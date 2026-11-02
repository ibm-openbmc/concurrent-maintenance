#include "gpio_utils.hpp"

#include <phosphor-logging/lg2.hpp>

#include <stdexcept>
#include <string>

SiblingBmcReset::SiblingBmcReset(const std::string& lineName) :
    lineName(lineName)
{}

void SiblingBmcReset::assertReset()
{
    line = gpiod::find_line(lineName);

    if (!line)
    {
        lg2::error("Could not find sibling BMC reset GPIO: {LINE}", "LINE",
                   lineName);
        throw std::runtime_error("Could not find line: " + lineName);
    }

    line.request({"ibm-concurrent-maintenance",
                  gpiod::line_request::DIRECTION_OUTPUT, 0},
                 0);

    lg2::info("Driving sibling BMC reset line low: {LINE}", "LINE", lineName);
}

void SiblingBmcReset::deassertReset()
{
    line.set_value(1);
    line.set_direction_input();
    lg2::info("Driving sibling BMC reset line high: {LINE}", "LINE", lineName);

    line.release();
}

SiblingBmcReset::~SiblingBmcReset()
{
    if (line)
    {
        line.release();
    }
}
