#include "manager.hpp"

#include <gtest/gtest.h>
#include <sdbusplus/async/context.hpp>

namespace concurrent_maintenance {

TEST(ManagerTest, CanBeConstructed) {
  sdbusplus::async::context ctx;

  EXPECT_NO_THROW({ Manager manager(ctx); });
}

} // namespace concurrent_maintenance
