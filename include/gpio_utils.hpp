// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright IBM Corp.

#pragma once

#include <gpiod.hpp>

#include <string>

namespace concurrent_maintenance
{

class SiblingBMCReset
{
  public:
    /**
     * @brief Constructor
     *
     * Finds the sibling BMC reset GPIO line by name. The line is not
     * requested or driven until assertReset() is called.
     *
     * @param[in] lineName - The GPIO line name of the sibling BMC reset
     *
     * @throws std::runtime_error if the GPIO line cannot be found
     */
    explicit SiblingBMCReset(const std::string& lineName);

    SiblingBMCReset(const SiblingBMCReset&) = delete;
    SiblingBMCReset& operator=(const SiblingBMCReset&) = delete;
    SiblingBMCReset(SiblingBMCReset&&) = delete;
    SiblingBMCReset& operator=(SiblingBMCReset&&) = delete;
    /**
     * @brief Asserts the sibling BMC reset.
     *
     * Requests the GPIO line as an output and drives it low, which places
     * the sibling BMC in reset. Does nothing if the reset is already
     * asserted.
     */
    void assertReset();

    /**
     * @brief Deasserts the sibling BMC reset.
     *
     * Drives the GPIO line high to take the sibling BMC out of reset, then
     * switches the line to an input and releases it. Does nothing if the
     * reset is not currently asserted.
     */
    void deassertReset();

    ~SiblingBMCReset();

  private:
    std::string lineName;
    gpiod::line line;
    bool asserted = false;
};

} // namespace concurrent_maintenance
