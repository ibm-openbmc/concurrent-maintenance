// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright IBM Corp.

#pragma once

#include <sdbusplus/async/context.hpp>

#include <functional>
#include <memory>

#include <string>

namespace concurrent_maintenance
{

class CMObject
{
  public:
    CMObject(sdbusplus::async::context& ctx, const std::string& path,
             std::function<void()> onOperationComplete);

    CMObject(const CMObject&) = delete;
    CMObject& operator=(const CMObject&) = delete;
    CMObject(CMObject&&) = delete;
    CMObject& operator=(CMObject&&) = delete;

    ~CMObject() = default;

    // Get the object path
    const std::string& getPath() const
    {
        return objectPath;
    }

  private:
    std::string objectPath;

    std::unique_ptr<ProgressIntf> progressIntf;
    std::function<void()> onOperationComplete;
};

} // namespace concurrent_maintenance
