#pragma once

#include <sdbusplus/async/context.hpp>
#include <sdbusplus/bus.hpp>

#include <string>

namespace concurrent_maintenance
{

class CMObject
{
  public:
    CMObject(sdbusplus::async::context& ctx, const std::string& path);

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

    // TODO: Add progress interface when implementing progress tracking
    // When adding the progress interface, use sdbusplus::server::object_t:
    // Example:
    // sdbusplus::server::object_t<xyz::openbmc_project::Common::Progress::server::Progress>
    //     dbusObject(bus, path.c_str(), action::defer_emit);
    // Then call: dbusObject.emit_object_added();
};

} // namespace concurrent_maintenance
