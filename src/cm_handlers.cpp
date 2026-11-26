// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright IBM Corp.

#include "cm_handlers.hpp"

#include "cm_object.hpp"
#include "utils.hpp"

#include <com/ibm/VPD/Manager/client.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async/context.hpp>
#include <sdbusplus/async/task.hpp>
#include <sdbusplus/async/timer.hpp>
#include <xyz/openbmc_project/Common/Progress/client.hpp>
#include <xyz/openbmc_project/Control/SideBandBus/client.hpp>
#include <xyz/openbmc_project/ObjectMapper/client.hpp>

#include <chrono>
#include <exception>
#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace concurrent_maintenance
{

namespace
{
using namespace std::chrono_literals;
using ObjectMapper = sdbusplus::client::xyz::openbmc_project::ObjectMapper<>;
using SideBandBus =
    sdbusplus::client::xyz::openbmc_project::control::SideBandBus<>;
using VpdManager = sdbusplus::client::com::ibm::vpd::Manager<>;
using ProgressClient =
    sdbusplus::client::xyz::openbmc_project::common::Progress<>;

constexpr auto sideBandBusService = "xyz.openbmc_project.Control.SideBandBus";
constexpr auto chassisInterface = "xyz.openbmc_project.Inventory.Item.Chassis";
constexpr auto progressInterface = "xyz.openbmc_project.Common.Progress";
constexpr auto vpdManagerService = "com.ibm.VPD.Manager";
constexpr auto vpdManagerPath = "/com/ibm/VPD/Manager";
constexpr auto inventoryService = "xyz.openbmc_project.Inventory.Manager";
/**
 * @brief Resolve the chassis inventory path that owns a FRU.
 *
 * @param ctx      Async context used for the mapper call.
 * @param fruPath  Inventory path of the FRU.
 * @return Chassis inventory path.
 * @throws std::runtime_error if the mapper returns no chassis ancestor, or
 *         an sdbusplus exception if the mapper call fails.
 */

sdbusplus::async::task<std::string>
    getChassisPath(std::reference_wrapper<sdbusplus::async::context> ctx,
                   std::string fruPath)
{
    auto ancestors =
        co_await ObjectMapper(ctx.get())
            .service(ObjectMapper::default_service)
            .path(ObjectMapper::instance_path)
            .get_ancestors(fruPath, std::vector<std::string>{chassisInterface});

    if (ancestors.empty())
    {
        throw std::runtime_error("no Inventory.Item.Chassis ancestor found");
    }

    co_return ancestors.begin()->first;
}

/**
 * @brief Poll Common.Progress.Status on a path until it reaches the expected
 *        value, or the timeout expires.
 *
 * Every wait is a co_await, so the event loop is never blocked.
 *
 * @throws std::runtime_error on timeout or if the status becomes
 *         Failed/Aborted, or an sdbusplus exception if the read fails.
 */
sdbusplus::async::task<>
    waitForProgressStatus(std::reference_wrapper<sdbusplus::async::context> ctx,
                          const std::string& path,
                          ProgressClient::OperationStatus expected,
                          std::chrono::seconds timeout = 120s)
{
    constexpr auto pollInterval = 500ms;
    const auto deadline = std::chrono::steady_clock::now() + timeout;

    while (std::chrono::steady_clock::now() < deadline)
    {
        try
        {
            const auto status = co_await ProgressClient(ctx.get())
                                    .service(inventoryService)
                                    .path(path)
                                    .status();

            if (status == expected)
            {
                lg2::info("Progress.Status reached expected state on {PATH}",
                          "PATH", path);
                co_return;
            }

            if (status == ProgressClient::OperationStatus::Failed ||
                status == ProgressClient::OperationStatus::Aborted)
            {
                throw std::runtime_error("VPD operation failed on target path");
            }
        }
        catch (const std::exception& e)
        {
            lg2::error("Progress.Status wait failed on {PATH}: {ERROR}", "PATH",
                       path, "ERROR", e);
            throw;
        }

        co_await sdbusplus::async::sleep_for(ctx.get(), pollInterval);
    }

    lg2::error(
        "TIMEOUT ({SECS}s) waiting for expected Progress.Status on {PATH}",
        "SECS", timeout.count(), "PATH", path);
    throw std::runtime_error("Timed out waiting for VPD Progress.Status");
}
} //namespace

sdbusplus::async::task<>
    fsiCardRemove(std::reference_wrapper<sdbusplus::async::context> ctx,
                  std::string fruPath,
                  std::reference_wrapper<CMObject> /*cmObj*/)
{
    lg2::info("FSI remove: starting for {PATH}", "PATH", fruPath);

    std::string chassisPath;

    try
    {
        chassisPath = co_await getChassisPath(ctx, fruPath);
    }
    catch (const std::exception& e)
    {
        lg2::error("FSI remove: failed to resolve chassis for {PATH}: {ERROR}",
                   "PATH", fruPath, "ERROR", e);
        throw;
    }

    try
    {
        co_await SideBandBus(ctx.get())
            .service(sideBandBusService)
            .path(SideBandBus::instance_path)
            .set_access(sdbusplus::object_path{chassisPath}, false);
    }
    catch (const std::exception& e)
    {
        lg2::error("FSI remove: failed to disable sideband bus for "
                   "{CHASSIS}: {ERROR}",
                   "CHASSIS", chassisPath, "ERROR", e);
        throw;
    }

    lg2::info("FSI remove: sequence complete for {PATH}", "PATH", fruPath);

    try
    {
        const bool hostRunning = co_await isHostRunning(ctx);
        lg2::info("Host running state: {STATE}", "STATE",
                  hostRunning ? "Running" : "Not Running");

        if (hostRunning)
        {
            co_await VpdManager(ctx.get())
                .service(vpdManagerService)
                .path(vpdManagerPath)
                .delete_fruvpd(sdbusplus::object_path(fruPath), true);

            co_await waitForProgressStatus(
                ctx, fruPath, ProgressClient::OperationStatus::NotStarted);
        }
        else
        {
            // TODO: host not running needs a different VPD API, which is
            // not ready yet. Placeholder.
            lg2::info("FSI remove: host not running, VPD removal not "
                      "implemented yet for {PATH}",
                      "PATH", fruPath);
        }
    }
    catch (const std::exception& e)
    {
        lg2::error("FSI remove: VPD removal failed for {PATH}: {ERROR}", "PATH",
                   fruPath, "ERROR", e);
        throw;
    }
}

sdbusplus::async::task<>
    fsiCardAdd(std::reference_wrapper<sdbusplus::async::context> ctx,
               std::string fruPath, std::reference_wrapper<CMObject> /*cmObj*/)
{
    lg2::info("FSI add: starting for {PATH}", "PATH", fruPath);

    std::string chassisPath;

    try
    {
        chassisPath = co_await getChassisPath(ctx, fruPath);
    }
    catch (const std::exception& e)
    {
        lg2::error("FSI add: failed to resolve chassis for {PATH}: {ERROR}",
                   "PATH", fruPath, "ERROR", e);
        throw;
    }

    try
    {
        co_await SideBandBus(ctx.get())
            .service(sideBandBusService)
            .path(SideBandBus::instance_path)
            .set_access(sdbusplus::object_path{chassisPath}, true);
    }
    catch (const std::exception& e)
    {
        lg2::error("FSI add: failed to enable sideband bus for "
                   "{CHASSIS}: {ERROR}",
                   "CHASSIS", chassisPath, "ERROR", e);
        throw;
    }

    lg2::info("FSI add: sequence complete for {PATH}", "PATH", fruPath);

    try
    {
        const bool hostRunning = co_await isHostRunning(ctx);
        lg2::info("Host running state: {STATE}", "STATE",
                  hostRunning ? "Running" : "Not Running");

        if (hostRunning)
        {
            co_await VpdManager(ctx.get())
                .service(vpdManagerService)
                .path(vpdManagerPath)
                .collect_fruvpd(sdbusplus::object_path(fruPath), true);

            co_await waitForProgressStatus(
                ctx, fruPath, ProgressClient::OperationStatus::Completed);
        }
        else
        {
            // TODO: host not running needs a different VPD API, which is
            // not ready yet. Placeholder.
            lg2::info("FSI add: host not running, VPD collection not "
                      "implemented yet for {PATH}",
                      "PATH", fruPath);
        }
    }
    catch (const std::exception& e)
    {
        lg2::error("FSI add: VPD collection failed for {PATH}: {ERROR}", "PATH",
                   fruPath, "ERROR", e);
        throw;
    }
}

} // namespace concurrent_maintenance
