// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright IBM Corp.

#include "cm_handlers.hpp"

#include "cm_object.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async/context.hpp>
#include <sdbusplus/async/task.hpp>
#include <xyz/openbmc_project/Control/SideBandBus/client.hpp>
#include <xyz/openbmc_project/ObjectMapper/client.hpp>

#include <exception>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>
namespace concurrent_maintenance
{

namespace
{
using ObjectMapper = sdbusplus::client::xyz::openbmc_project::ObjectMapper<>;
using SideBandBus =
    sdbusplus::client::xyz::openbmc_project::control::SideBandBus<>;
constexpr auto sideBandBusService = "xyz.openbmc_project.Control.SideBandBus";
constexpr auto chassisInterface = "xyz.openbmc_project.Inventory.Item.Chassis";

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
} // namespace

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
}

} // namespace concurrent_maintenance
