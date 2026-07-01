#include "manager.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async/context.hpp>
#include <sdbusplus/server/manager.hpp>

int main()
{
    sdbusplus::async::context ctx;

    // Create ObjectManager for concurrent maintenance
    constexpr auto objManagerPath = "/com/ibm/concurrent_maintenance";
    sdbusplus::server::manager_t objManager(ctx, objManagerPath);

    ctx.request_name("com.ibm.ConcurrentMaintenance");

    concurrent_maintenance::Manager manager(ctx);

    lg2::info("Concurrent Maintenance service started");

    ctx.run();

    return 0;
}
