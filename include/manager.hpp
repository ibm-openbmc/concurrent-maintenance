// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright IBM Corp.

#pragma once

#include "cm_object.hpp"
#include "fru_identifier.hpp"

#include <sdbusplus/async/context.hpp>
#include <sdbusplus/async/server.hpp>
#include <sdbusplus/async/task.hpp>

#include <cstdint>
#include <memory>
#include <string>

namespace concurrent_maintenance
{

class Manager;

using ManagerIface = sdbusplus::async::server_t<Manager, ProgressAServer>;

/**
 * @brief Top-level CM daemon manager.
 *
 * Responsibilities:
 *   1. Watch the ReadyToRemove property change signal on all inventory
 *      objects under /xyz/openbmc_project/inventory.
 *   2. Gate-keep concurrency: drop any new request while one is already
 *      in flight.
 *   3. Create CMObject and co_await its execute() to drive the operation.
 *   4. Implement the Progress interface at /com/ibm/ConcurrentMaintenance:
 *      NotStarted on start (start_time = 0), InProgress when remove begins,
 *      Completed/Failed when the subsequent add finishes.
 */
class Manager : public ManagerIface
{
  public:
    explicit Manager(sdbusplus::async::context& ctx);

    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(Manager&&) = delete;

    ~Manager() = default;

    /**
     * @brief Start watching ReadyToRemove property changes.
     *
     * Spawns the watcher on the context. This is kept out of the constructor
     * because the watcher waits on D-Bus signals indefinitely, which would
     * leave a pending task on the context and abort it on destruction in
     * unit tests that never run the context.
     */
    void start();

    /**
     * @brief Update the Progress interface status of parent object on D-Bus.
     *
     * @param newStatus      New status.
     * @param completedTime  completed_time (microseconds) applied for terminal
     *                       states (Completed/Failed/Aborted), taken from the
     *                       CM object. Ignored for other states.
     */
    void updateStatus(OperationStatus newStatus, uint64_t completedTime = 0);

    /** @brief Return the current Progress status of the parent object. */
    OperationStatus getStatus() const
    {
        return this->status();
    }

    /** @brief Return the parent Progress start_time in microseconds. */
    uint64_t getStartTime() const
    {
        return this->start_time();
    }

    /** @brief Return the parent Progress completed_time in microseconds. */
    uint64_t getCompletedTime() const
    {
        return this->completed_time();
    }

  private:
    sdbusplus::async::context& ctx;

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
     *   remove failed: Failed
     *   add complete: Completed or Failed
     *   add succeeded: back to NotStarted (start/completed times cleared)
     * Terminal transitions reuse the CM object's completed_time so the parent
     * and child report the same instant.
     *
     * @param readyToRemove  true for removal, false for addition.
     * @param fruPath        Inventory D-Bus path of the FRU.
     */
    sdbusplus::async::task<> processCMRequest(bool readyToRemove,
                                              std::string fruPath);
};

} // namespace concurrent_maintenance
