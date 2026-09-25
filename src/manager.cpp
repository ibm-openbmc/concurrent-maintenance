// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright IBM Corp.

#include "manager.hpp"

#include "cm_object.hpp"
#include "fru_identifier.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async/match.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>
#include <xyz/openbmc_project/ObjectMapper/client.hpp>

#include <map>
#include <string>
#include <variant>
#include <vector>

namespace concurrent_maintenance
{

using ObjectMapper = sdbusplus::client::xyz::openbmc_project::ObjectMapper<>;

constexpr auto readyToRemoveProperty = "ReadyToRemove";
constexpr auto cmRemoveObjectPath = "/com/ibm/ConcurrentMaintenance/remove";
constexpr auto cmAddObjectPath = "/com/ibm/ConcurrentMaintenance/add";

Manager::Manager(sdbusplus::async::context& ctx) :
    ctx(ctx), currentCMObject(nullptr)
{
    lg2::info("Concurrent Maintenance manager initialized");
    ctx.spawn(watchReadyToRemove());
}

sdbusplus::async::task<> Manager::watchReadyToRemove()
{
    using PropertiesVariant = std::variant<bool>;
    using ChangedProperties = std::map<std::string, PropertiesVariant>;

    /* Watch for property changes on all child objects under
     * /xyz/openbmc_project/inventory
     */
    sdbusplus::async::match matcher(
        ctx, sdbusplus::bus::match::rules::propertiesChangedNamespace(
                 "/xyz/openbmc_project/inventory",
                 "xyz.openbmc_project.State.ReadyToRemove"));

    lg2::info(
        "ReadyToRemove property watcher registered for all inventory objects");

    while (!ctx.stop_requested())
    {
        auto msg = co_await matcher.next();

        try
        {
            auto [interface, changedProperties] =
                msg.unpack<std::string, ChangedProperties>();

            const auto it = changedProperties.find(readyToRemoveProperty);
            if (it == changedProperties.end())
            {
                continue;
            }

            bool readyToRemove = std::get<bool>(it->second);
            std::string fruPath = msg.get_path();

            lg2::info("ReadyToRemove property changed on {PATH}: {VALUE}",
                      "PATH", fruPath, "VALUE", readyToRemove);

            co_await processCMRequest(readyToRemove, std::move(fruPath));
        }
        catch (const std::exception& e)
        {
            lg2::error("Error handling ReadyToRemove property change: {ERROR}",
                       "ERROR", e);
        }
    }
}

sdbusplus::async::task<> Manager::processCMRequest(bool readyToRemove,
                                                   std::string fruPath)
{
    /* If a CM operation is actively InProgress, drop concurrent requests */
    if (currentCMObject &&
        currentCMObject->getStatus() == OperationStatus::InProgress)
    {
        lg2::error("CM is already in progress. Object already exists at path:"
                   " {PATH}. Dropping request for {FRUPATH}.",
                   "PATH", currentCMObject->getPath(), "FRUPATH", fruPath);
        co_return;
    }

    /* co_await the mapper to obtain the interfaces implemented by this FRU.
     * This is also a natural yield point that serializes any back-to-back
     * signals arriving before the first coroutine completes.
     */
    std::vector<std::string> interfaces;
    try
    {
        auto result = co_await ObjectMapper(ctx)
                          .service(ObjectMapper::default_service)
                          .path(ObjectMapper::instance_path)
                          .get_object(fruPath, {});

        for (const auto& [service, ifaces] : result)
        {
            interfaces.insert(interfaces.end(), ifaces.begin(), ifaces.end());
        }
    }
    catch (const std::exception& e)
    {
        /* Mapper lookup failed; identifyType will be called with an empty
         * interface list and will return nullptr for unrecognized FRUs. */
        lg2::warning("Mapper lookup failed for {PATH}: {ERROR}.", "PATH",
                     fruPath, "ERROR", e);
    }

    const FRUOperations* ops = FRUIdentifier::identifyType(interfaces, fruPath);
    if (ops == nullptr)
    {
        lg2::error("Unrecognized FRU type for {PATH} — dropping request.",
                   "PATH", fruPath);
        co_return;
    }

    const std::string cmPath = readyToRemove ? cmRemoveObjectPath
                                             : cmAddObjectPath;

    currentCMObject.reset();

    currentCMObject = std::make_unique<CMObject>(ctx, cmPath, fruPath);

    try
    {
        co_await currentCMObject->execute(readyToRemove, *ops);
        lg2::info("CM operation completed for {PATH}", "PATH", fruPath);
    }
    catch (const std::exception& e)
    {
        lg2::error("CM operation failed for {PATH}: {ERROR}", "PATH", fruPath,
                   "ERROR", e);
    }

    /* Keep currentCMObject on D-Bus until the next event arrives */
}

} // namespace concurrent_maintenance
