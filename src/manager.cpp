#include "manager.hpp"

#include <phosphor-logging/lg2.hpp>

namespace concurrent_maintenance {

Manager::Manager(sdbusplus::async::context &ctx) : ctx(ctx) {
  lg2::info("Concurrent Maintenance manager initialized");
}

} // namespace concurrent_maintenance
