// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright IBM Corp.

#include "cm_parent_object.hpp"

#include "utils.hpp"

#include <phosphor-logging/lg2.hpp>

namespace concurrent_maintenance
{

CMParentObject::CMParentObject(sdbusplus::async::context& ctx,
                               const std::string& objectPath) :
    sdbusplus::async::server_t<CMParentObject, ProgressAServer>(
        ctx, objectPath.c_str()),
    objectPath(objectPath)
{
    lg2::info("CM parent object created at {PATH}", "PATH", objectPath);

    this->start_time(currentTimeMicroseconds());
    this->status(OperationStatus::NotStarted);
    this->emit_added();
}

void CMParentObject::updateStatus(OperationStatus newStatus)
{
    if (newStatus == OperationStatus::InProgress)
    {
        /* Stamp start_time when the remove phase begins. */
        this->start_time(currentTimeMicroseconds());
    }
    else if (newStatus == OperationStatus::Completed ||
             newStatus == OperationStatus::Failed ||
             newStatus == OperationStatus::Aborted)
    {
        this->completed_time(currentTimeMicroseconds());
    }

    this->status(newStatus);
    lg2::info("CM parent object status at {PATH} updated to {STATUS}", "PATH",
              objectPath, "STATUS", convertForMessage(newStatus));
}

} // namespace concurrent_maintenance
