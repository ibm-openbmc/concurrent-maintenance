// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright IBM Corp.

#pragma once

#include <sdbusplus/async/context.hpp>
#include <sdbusplus/async/task.hpp>

#include <chrono>
#include <cstdint>
#include <functional>

namespace concurrent_maintenance
{

/** @brief Returns the current wall-clock time as microseconds since the
 *         Unix epoch. */
inline uint64_t currentTimeMicroseconds()
{
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
}

/**
 * @brief Check whether the host is running using the BootProgress property.
 *
 * The host is considered running once BootProgress is SystemInitComplete,
 * OSStart or OSRunning. Any earlier stage (including Unspecified) is not
 * running. Returns false if BootProgress cannot be read.
 *
 * @param ctx  Reference wrapper to the async D-Bus context (never null).
 *             Must outlive the task.
 * @return true if the host is running, false otherwise.
 */
sdbusplus::async::task<bool>
    isHostRunning(std::reference_wrapper<sdbusplus::async::context> ctx);

} // namespace concurrent_maintenance
