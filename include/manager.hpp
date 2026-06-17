#pragma once

#include <sdbusplus/async/context.hpp>

namespace concurrent_maintenance
{

class Manager
{
  public:
    explicit Manager(sdbusplus::async::context& ctx);

    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(Manager&&) = delete;

    ~Manager() = default;

  private:
    sdbusplus::async::context& ctx;
};

} // namespace concurrent_maintenance
