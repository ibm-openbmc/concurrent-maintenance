// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright IBM Corp.

#include "gpio_utils.hpp"

#include <gpiod.hpp>
#include <phosphor-logging/lg2.hpp>

#include <exception>
#include <stdexcept>
#include <string>

namespace concurrent_maintenance
{

SiblingBMCReset::SiblingBMCReset(const std::string& lineName) :
    lineName(lineName), line(gpiod::find_line(lineName))
{
    if (!line)
    {
        lg2::error("Could not find sibling BMC reset GPIO: {LINE}", "LINE",
                   lineName);
        throw std::runtime_error("Could not find line: " + lineName);
    }
}

void SiblingBMCReset::assertReset()
{
    if (asserted)
    {
        lg2::info("Sibling BMC reset already asserted: {LINE}", "LINE",
                  lineName);
        return;
    }

    line.request({"ibm-concurrent-maintenance",
                  gpiod::line_request::DIRECTION_OUTPUT, 0},
                 0);

    asserted = true;

    lg2::info("Driving sibling BMC reset line low: {LINE}", "LINE", lineName);
}

void SiblingBMCReset::deassertReset()
{
    if (!asserted)
    {
        lg2::info("Sibling BMC reset not asserted, nothing to do: {LINE}",
                  "LINE", lineName);
        return;
    }
    line.set_value(1);
    line.set_direction_input();
    line.release();
    asserted = false;
    lg2::info("Driving sibling BMC reset line high: {LINE}", "LINE", lineName);
}

SiblingBMCReset::~SiblingBMCReset()
{
    try
    {
        if (asserted)
        {
            line.release();
        }
    }
    catch (const std::exception& e)
    {
        lg2::error("Failed to release sibling BMC reset GPIO {LINE}: {ERROR}",
                   "LINE", lineName, "ERROR", e);
    }
}

} // namespace concurrent_maintenance
