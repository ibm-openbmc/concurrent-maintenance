// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright IBM Corp.

#pragma once

#include "cm_object.hpp"
#include "cm_parent_object.hpp"
#include "fru_identifier.hpp"

#include <sdbusplus/async/context.hpp>
#include <sdbusplus/async/task.hpp>

#include <memory>
#include <string>

namespace concurrent_maintenance
{

/**
 * @brief Top-level CM daemon manager.
 *
 * Responsibilities:
 *   1. Watch the ReadyToRemove property change signal on all inventory
 *      objects under /xyz/openbmc_project/inventory.
 *   2. Gate-keep concurrency: drop any new request while one is already
 *      in flight.
 *   3. Create CMObject and co_await its execute() to drive the operation.
 *   4. Drive the parent Progress interface at /com/ibm/ConcurrentMaintenance:
 *      NotStarted on start, InProgress when remove begins, Completed/Failed
 *      when the subsequent add finishes.
 */
class Manager
{
  public:
    explicit Manager(sdbusplus::async::context& ctx);

    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(Manager&&) = delete;

    ~Manager() = default;

  private:
    sdbusplus::async::context& ctx;

    /**
     * @brief Long-lived Progress object at /com/ibm/ConcurrentMaintenance.
     *
     * Tracks the overall remove to add cycle:
     *   NotStarted  — no operation started yet.
     *   InProgress  — remove operation underway (set when remove begins).
     *   Completed   — add operation finished successfully.
     *   Failed      — remove or add operation encountered an error.
     */
    CMParentObject parentObject;

    /* The in-flight CM object (nullptr when idle). */
    std::unique_ptr<CMObject> currentCMObject;

    /* Async coroutine: watches for ReadyToRemove property changes
     * using sdbusplus::async::match and co_awaits processCMRequest per signal,
     * serializing operations so Signal 2 is queued until Signal 1 completes. */
    sdbusplus::async::task<> watchReadyToRemove();

    /**
     * @brief Gate-keep concurrency and drive the full CM operation.
     *
     * Drops the request immediately if a CM operation is already
     * in flight. Otherwise co_awaits the mapper to resolve the FRU
     * interface list, identifies the handler, creates a CMObject
     * and co_awaits execute() to completion.
     *
     * Parent Progress transitions:
     *   remove start: InProgress
     *   add complete: Completed or Failed
     *
     * @param readyToRemove  true for removal, false for addition.
     * @param fruPath        Inventory D-Bus path of the FRU.
     */
    sdbusplus::async::task<> processCMRequest(bool readyToRemove,
                                              std::string fruPath);
};

} // namespace concurrent_maintenance
