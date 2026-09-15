// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright IBM Corp.

#include "cm_object.hpp"

#include "fru_identifier.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async/context.hpp>
#include <sdbusplus/async/task.hpp>

#include <exception>
#include <functional>
#include <string>

namespace concurrent_maintenance
{

CMObject::CMObject(sdbusplus::async::context& ctx,
                   const std::string& objectPath, const std::string& fruPath) :
    sdbusplus::async::server_t<CMObject, AssocDefsAServer>(ctx,
                                                           objectPath.c_str()),
    ctx(ctx), objectPath(objectPath), fruPath(fruPath)
{
    // Association:
    // "inventory"  — from the CM object, the endpoint is an inventory item
    // "cm_object"  — from the inventory item, what points at it is a cm_object
    this->associations({{"inventory", "cm_object", fruPath}});

    this->emit_added();

    lg2::info(
        "CM object created at {PATH} with association to inventory {INV_PATH}",
        "PATH", objectPath, "INV_PATH", fruPath);
}

sdbusplus::async::task<> CMObject::execute(bool isRemove,
                                           const FRUOperations& ops)
{
    try
    {
        if (isRemove)
        {
            lg2::info("CM object: starting remove for {PATH}", "PATH", fruPath);
            co_await ops.remove(std::ref(ctx), fruPath, std::ref(*this));
        }
        else
        {
            lg2::info("CM object: starting add for {PATH}", "PATH", fruPath);
            co_await ops.add(std::ref(ctx), fruPath, std::ref(*this));
        }

        lg2::info("CM object: sequence completed for {PATH}", "PATH", fruPath);
    }
    catch (const std::exception& e)
    {
        lg2::error("CM object: sequence failed for {PATH}: {ERROR}", "PATH",
                   fruPath, "ERROR", e);
    }

    co_return;
}

} // namespace concurrent_maintenance
