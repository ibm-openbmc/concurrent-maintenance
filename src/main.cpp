#include "manager.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async/context.hpp>

int main() {
  sdbusplus::async::context ctx;

  ctx.request_name("com.ibm.ConcurrentMaintenance");

  concurrent_maintenance::Manager manager(ctx);

  lg2::info("Concurrent Maintenance service started");

  ctx.run();

  return 0;
}
