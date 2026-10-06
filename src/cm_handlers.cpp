// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright IBM Corp.

#include "cm_handlers.hpp"

#include <xyz/openbmc_project/Control/SideBandBus/client.hpp>
#include <xyz/openbmc_project/ObjectMapper/client.hpp>

namespace concurrent_maintenance
{

using ObjectMapper = sdbusplus::client::xyz::openbmc_project::ObjectMapper<>;
using SideBandBus =
    sdbusplus::client::xyz::openbmc_project::control::SideBandBus<>;

sdbusplus::async::task<>
    fsiCardRemove(std::reference_wrapper<sdbusplus::async::context> ctx,
                  std::string fruPath,
                  std::reference_wrapper<CMObject> /*cmObj*/)
{
    lg2::info("FSI remove: starting for {PATH}", "PATH", fruPath);

    std::string chassisPath;

    try
    {
        auto ancestors =
            co_await ObjectMapper(ctx.get())
                .service(ObjectMapper::default_service)
                .path(ObjectMapper::instance_path)
                .get_ancestors(
                    fruPath, std::vector<std::string>{
                                 "xyz.openbmc_project.Inventory.Item.Chassis"});

        chassisPath = ancestors.begin()->first;
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
            .service("xyz.openbmc_project.Control.SideBandBus")
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
        auto ancestors =
            co_await ObjectMapper(ctx.get())
                .service(ObjectMapper::default_service)
                .path(ObjectMapper::instance_path)
                .get_ancestors(
                    fruPath, std::vector<std::string>{
                                 "xyz.openbmc_project.Inventory.Item.Chassis"});

        chassisPath = ancestors.begin()->first;
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
            .service("xyz.openbmc_project.Control.SideBandBus")
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
