// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright IBM Corp.

#include "manager.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async/match.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>

#include <map>
#include <string>
#include <variant>

namespace concurrent_maintenance
{

constexpr auto readyToRemoveProperty = "ReadyToRemove";
constexpr auto cmRemoveObjectPath = "/com/ibm/ConcurrentMaintenance/remove";
constexpr auto cmAddObjectPath = "/com/ibm/ConcurrentMaintenance/add";

Manager::Manager(sdbusplus::async::context& ctx) : ctx(ctx)
{
    lg2::info("Concurrent Maintenance manager initialized");
}

// NOLINTBEGIN(clang-analyzer-core.uninitialized.Branch)
void Manager::start()
{
    // Spawn coroutine to watch for ReadyToRemove property changes
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

    while (true)
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
            const auto& objectPath = msg.get_path();
            lg2::info("ReadyToRemove property changed on {PATH}: {VALUE}",
                      "PATH", objectPath, "VALUE", readyToRemove);

            manageCMObject(readyToRemove);
        }
        catch (const std::exception& e)
        {
            lg2::error("Error handling ReadyToRemove property change: {ERROR}",
                       "ERROR", e);
        }
    }
}
// NOLINTEND(clang-analyzer-core.uninitialized.Branch)

void Manager::manageCMObject(bool readyToRemove)
{
    // Only one CM at a time, reject if a CM is already in progress
    if (currentCMObject)
    {
        lg2::error(
            "CM is already in progress. Object already exists at path: {PATH}.",
            "PATH", currentCMObject->getPath());
        return;
    }

    const std::string path = readyToRemove ? cmRemoveObjectPath
                                           : cmAddObjectPath;


    lg2::info("Creating CM object at {PATH}", "PATH", path);
    currentCMObject = std::make_unique<CMObject>(ctx, path);
    lg2::info("CM object created at {PATH}", "PATH",
              currentCMObject->getPath());

    currentCMObject = std::make_unique<CMObject>(
        ctx, path, [this]() { ctx.spawn(deleteCMObject()); });
    currentCMObject->updateStatus(OperationStatus::InProgress);

}

} // namespace concurrent_maintenance
