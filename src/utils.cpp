// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright IBM Corp.

#include "utils.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async/context.hpp>
#include <sdbusplus/async/task.hpp>
#include <xyz/openbmc_project/ObjectMapper/client.hpp>
#include <xyz/openbmc_project/State/Boot/Progress/client.hpp>

#include <exception>

namespace concurrent_maintenance
{

using ObjectMapper = sdbusplus::client::xyz::openbmc_project::ObjectMapper<>;
using BootProgress =
    sdbusplus::client::xyz::openbmc_project::state::boot::Progress<>;
using ProgressStages = BootProgress::ProgressStages;

constexpr auto hostStatePath = "/xyz/openbmc_project/state/host0";

// ctx is owned by the caller and outlives the task.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
sdbusplus::async::task<bool> isHostRunning(sdbusplus::async::context& ctx)
{
    try
    {
        /* co_await the mapper to find the service hosting BootProgress. */
        const auto result = co_await ObjectMapper(ctx)
                                .service(ObjectMapper::default_service)
                                .path(ObjectMapper::instance_path)
                                .get_object(hostStatePath,
                                            {BootProgress::interface});

        if (result.empty())
        {
            lg2::error("No service found hosting BootProgress at {PATH}",
                       "PATH", hostStatePath);
            co_return false;
        }

        const auto stage = co_await BootProgress(ctx)
                               .service(result.begin()->first)
                               .path(hostStatePath)
                               .boot_progress();

        co_return stage == ProgressStages::SystemInitComplete ||
            stage == ProgressStages::OSRunning;
    }
    catch (const std::exception& e)
    {
        lg2::error("Failed to read BootProgress at {PATH}: {ERROR}", "PATH",
                   hostStatePath, "ERROR", e);
        co_return false;
    }
}

} // namespace concurrent_maintenance
