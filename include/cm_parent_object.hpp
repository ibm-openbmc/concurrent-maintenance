// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright IBM Corp.

#pragma once

#include "cm_object.hpp"

#include <sdbusplus/async/context.hpp>
#include <sdbusplus/async/server.hpp>
#include <xyz/openbmc_project/Common/Progress/aserver.hpp>

#include <string>

namespace concurrent_maintenance
{

/**
 * @brief Long-lived D-Bus object at /com/ibm/ConcurrentMaintenance.
 *
 * Publishes the xyz.openbmc_project.Common.Progress interface for the
 * entire concurrent maintenance progress of remove to add cycle:
 *
 *   NotStarted  — service has started, no operation yet (or last cycle
 *                 already finished and a new one has not begun).
 *   InProgress  — remove phase has started (stamped at remove start).
 *   Completed   — add phase finished successfully.
 *   Failed      — remove or add phase encountered an error.
 *
 * This object is created once during Manager construction and lives for
 * the full daemon lifetime.
 */
class CMParentObject :
    public sdbusplus::async::server_t<CMParentObject, ProgressAServer>
{
  public:
    CMParentObject(sdbusplus::async::context& ctx,
                   const std::string& objectPath);

    CMParentObject(const CMParentObject&) = delete;
    CMParentObject& operator=(const CMParentObject&) = delete;

    ~CMParentObject() = default;

    /** @brief Update the Progress interface status on D-Bus. */
    void updateStatus(OperationStatus newStatus);

    /** @brief Return the current Progress status. */
    OperationStatus getStatus() const
    {
        return this->status();
    }

  private:
    const std::string objectPath;
};

} // namespace concurrent_maintenance
