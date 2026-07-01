#include "cm_object.hpp"

#include <phosphor-logging/lg2.hpp>

#include <chrono>

namespace concurrent_maintenance
{

CMObject::CMObject(sdbusplus::async::context& /*ctx*/,
                   const std::string& path) : objectPath(path)
{
    lg2::info("Creating CM object at path: {PATH}", "PATH", path);

    // TODO: When implementing progress interface, create the actual D-Bus
    // object here Example: auto& bus = ctx.get_bus(); dbusObject =
    // std::make_unique<sdbusplus::server::object_t<...>>(
    //     bus, path.c_str(),
    //     sdbusplus::server::object_t<...>::action::defer_emit);
    // dbusObject->emit_object_added();

    lg2::info("CM object created at path: {PATH}", "PATH", path);
}

} // namespace concurrent_maintenance
