// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright IBM Corp.

#include "fru_identifier.hpp"

#include "cm_handlers.hpp"

#include <algorithm>
#include <array>
#include <string_view>
#include <vector>

namespace concurrent_maintenance
{

namespace
{

/** Returns true if 'interfaces' contains 'iface'. */
bool hasInterface(const std::vector<std::string>& interfaces,
                  std::string_view iface)
{
    return std::ranges::find(interfaces, iface) != interfaces.end();
}

bool matchBmcCard(const std::vector<std::string>& interfaces,
                  const std::string& /*fruPath*/)
{
    return hasInterface(interfaces,
                        "xyz.openbmc_project.Inventory.Item.Board") &&
           hasInterface(interfaces,
                        "xyz.openbmc_project.Common.PhysicalContext");
}

bool matchSwitchboard(const std::vector<std::string>& interfaces,
                      const std::string& /*fruPath*/)
{
    return hasInterface(interfaces,
                        "xyz.openbmc_project.Inventory.Item.Board.Motherboard");
}

bool matchFsiCard(const std::vector<std::string>& interfaces,
                  const std::string& /*fruPath*/)
{
    return hasInterface(interfaces, "xyz.openbmc_project.Inventory.Item.Board");
}

constexpr std::array<FRUEntry, 3> handlerTable = {{
    {matchBmcCard, {bmcRemove, bmcAdd}},
    {matchSwitchboard, {switchboardRemove, switchboardAdd}},
    {matchFsiCard, {fsiCardRemove, fsiCardAdd}},
}};

} // namespace

const FRUOperations*
    FRUIdentifier::identifyType(const std::vector<std::string>& interfaces,
                                const std::string& fruPath)
{
    for (const auto& entry : handlerTable)
    {
        if (entry.match(interfaces, fruPath))
        {
            return &entry.ops;
        }
    }
    return nullptr;
}

} // namespace concurrent_maintenance
