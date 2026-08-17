// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright IBM Corp.

#pragma once

#include "fru_identifier.hpp"

#include <sdbusplus/async/server.hpp>
#include <sdbusplus/async/task.hpp>
#include <xyz/openbmc_project/Association/Definitions/aserver.hpp>

#include <string>

namespace concurrent_maintenance
{

// Template alias required by server_t
template <typename Instance, typename Server>
using AssocDefsAServer =
    sdbusplus::aserver::xyz::openbmc_project::association::Definitions<Instance,
                                                                       Server>;

/**
 * @brief Tracks one in-flight CM operation and drives its execution.
 *
 * Responsibilities:
 *   1. Own the D-Bus tracking object path for the operation.
 *   2. Expose Association.Definitions linking this object back to the
 *      inventory FRU for which CM was triggered.
 *   3. Drive the remove or add sequence via execute().
 *   4. Update Progress state (setStep/setCompleted/setFailed).
 *
 * Handler functions are stateless inline free functions declared in
 * cm_handlers.hpp. CMObject invokes them — no circular ownership.
 *
 * Manager owns CMObject for the full operation lifetime.
 */
class CMObject : public sdbusplus::async::server_t<CMObject, AssocDefsAServer>
{
  public:
    CMObject(sdbusplus::async::context& ctx, const std::string& objectPath,
             const std::string& fruPath);

    CMObject(const CMObject&) = delete;
    CMObject& operator=(const CMObject&) = delete;

    ~CMObject()
    {
        this->emit_removed();
    }

    /** @brief D-Bus object path for this operation. */
    const std::string& getPath() const
    {
        return objectPath;
    }

    /** @brief Inventory path of the FRU under maintenance. */
    const std::string& getFruPath() const
    {
        return fruPath;
    }

    /**
     * @brief Drive the full remove or add sequence asynchronously.
     *
     * @param isRemove  true -> removal; false -> addition.
     * @param ops       Handler pair resolved by Manager via the mapper.
     *                  Never null — Manager validates before constructing
     *                  CMObject.
     */
    sdbusplus::async::task<> execute(bool isRemove, const FRUOperations& ops);

  private:
    sdbusplus::async::context& ctx;
    const std::string objectPath;
    const std::string fruPath;
};

} // namespace concurrent_maintenance
