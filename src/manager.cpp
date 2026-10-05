// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright IBM Corp.

#include "manager.hpp"

#include "cm_object.hpp"
#include "fru_identifier.hpp"
#include "utils.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async/context.hpp>
#include <sdbusplus/async/match.hpp>
#include <sdbusplus/async/task.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>
#include <xyz/openbmc_project/Common/Progress/common.hpp>
#include <xyz/openbmc_project/ObjectMapper/client.hpp>

#include <cstdint>
#include <exception>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace concurrent_maintenance
{

using ObjectMapper = sdbusplus::client::xyz::openbmc_project::ObjectMapper<>;

constexpr auto readyToRemoveProperty = "ReadyToRemove";
constexpr auto cmParentObjectPath = "/com/ibm/ConcurrentMaintenance";
constexpr auto cmRemoveObjectPath = "/com/ibm/ConcurrentMaintenance/remove";
constexpr auto cmAddObjectPath = "/com/ibm/ConcurrentMaintenance/add";

Manager::Manager(sdbusplus::async::context& ctx) :
    ManagerIface(ctx, cmParentObjectPath), ctx(ctx), currentCMObject(nullptr)
{
    lg2::info("Concurrent Maintenance manager initialized");

    /* start_time is 0 until the remove phase begins (no current time stamp). */
    this->start_time(0);
    this->status(OperationStatus::NotStarted);
    this->emit_added();
}

void Manager::start()
{
    ctx.spawn(watchReadyToRemove());
}

void Manager::updateStatus(OperationStatus newStatus, uint64_t completedTime)
{
    if (newStatus == OperationStatus::NotStarted)
    {
        /* Back to idle: clear both timestamps */
        this->start_time(0);
        this->completed_time(0);
    }
    else if (newStatus == OperationStatus::InProgress)
    {
        /* Stamp start_time when the remove phase begins. */
        this->start_time(currentTimeMicroseconds());
    }
    else if (newStatus == OperationStatus::Completed ||
             newStatus == OperationStatus::Failed ||
             newStatus == OperationStatus::Aborted)
    {
        this->completed_time(completedTime);
    }

    this->status(newStatus);
    lg2::info("CM parent object status at {PATH} updated to {STATUS}", "PATH",
              cmParentObjectPath, "STATUS", convertForMessage(newStatus));
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

    currentCMObject = std::make_unique<CMObject>(ctx, cmPath, fruPath);

    /* Drive the parent Progress transitions. */
    if (readyToRemove)
    {
        /* Remove operation is starting */
        updateStatus(OperationStatus::InProgress);
    }

    try
    {
        co_await currentCMObject->execute(readyToRemove, *ops);

        /* execute() swallows handler failures and records them on the CM
         * object, so derive the parent state from the object's status. The
         * object's completed_time is reused so both report the same instant. */
        const auto cmStatus = currentCMObject->getStatus();
        const auto cmCompletedTime = currentCMObject->getCompletedTime();

        if (cmStatus == OperationStatus::Failed)
        {
            lg2::error("CM operation failed for {PATH}", "PATH", fruPath);
            updateStatus(OperationStatus::Failed, cmCompletedTime);
        }
        else
        {
            lg2::info("CM operation completed for {PATH}", "PATH", fruPath);

            /* Add finished successfully: the parent progress is done. */
            if (!readyToRemove)
            {
                updateStatus(OperationStatus::Completed, cmCompletedTime);
            }
        }
    }
    catch (const std::exception& e)
    {
        lg2::error("CM operation failed for {PATH}: {ERROR}", "PATH", fruPath,
                   "ERROR", e);

        /* Unexpected error outside the object's own handling; there is no
         * object timestamp to reuse, so stamp the current time. */
        updateStatus(OperationStatus::Failed, currentTimeMicroseconds());
    }

    /* The add completed successfully, so the remove/add cycle is over: return
     * the parent to NotStarted and clear its start/completed times. A failed
     * add is left as Failed so the failure stays visible. */
    if (!readyToRemove && getStatus() == OperationStatus::Completed)
    {
        updateStatus(OperationStatus::NotStarted);
    }

    currentCMObject.reset();
}

} // namespace concurrent_maintenance
