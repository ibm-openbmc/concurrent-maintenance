// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright IBM Corp.

#include "cm_object.hpp"
#include "fru_identifier.hpp"
#include "manager.hpp"

#include <sdbusplus/async/context.hpp>
#include <sdbusplus/async/stdexec/__detail/__sync_wait.hpp>

#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace concurrent_maintenance
{

static const std::string fsiFruPath =
    "/xyz/openbmc_project/inventory/system/chassis3/motherboard/fsi_card";
static const std::string bmcFruPath =
    "/xyz/openbmc_project/inventory/system/chassis1/motherboard/ebmc_card";
static const std::string switchboardFruPath =
    "/xyz/openbmc_project/inventory/system/chassis0/motherboard/switchboard0";
static const std::string unknownFruPath =
    "/xyz/openbmc_project/inventory/system/chassis1/motherboard/mcm";

static const std::string cmRemovePath = "/com/ibm/ConcurrentMaintenance/remove";
static const std::string cmAddPath = "/com/ibm/ConcurrentMaintenance/add";

/* Interface lists reflect what the real mapper returns for each FRU type.
 * Injected directly so tests exercise the predicate logic without D-Bus. */
static const std::vector<std::string> fsiInterfaces = {
    "xyz.openbmc_project.Inventory.Item",
    "xyz.openbmc_project.Inventory.Item.Board",
    "xyz.openbmc_project.State.ReadyToRemove",
};
static const std::vector<std::string> bmcInterfaces = {
    "xyz.openbmc_project.Inventory.Item",
    "xyz.openbmc_project.Inventory.Item.Board",
    "xyz.openbmc_project.Common.PhysicalContext",
    "xyz.openbmc_project.State.ReadyToRemove",
};
static const std::vector<std::string> switchboardInterfaces = {
    "xyz.openbmc_project.Inventory.Item",
    "xyz.openbmc_project.Inventory.Item.Board",
    "xyz.openbmc_project.Inventory.Item.Board.Motherboard",
    "xyz.openbmc_project.State.ReadyToRemove",
};
static const std::vector<std::string> unknownInterfaces = {
    "xyz.openbmc_project.Inventory.Item",
    "xyz.openbmc_project.State.ReadyToRemove",
};

TEST(CMObjectTest, RemovePathIsCorrect)
{
    sdbusplus::async::context ctx;
    CMObject obj(ctx, cmRemovePath, fsiFruPath);
    EXPECT_EQ(obj.getPath(), cmRemovePath);
}

TEST(CMObjectTest, AddPathIsCorrect)
{
    sdbusplus::async::context ctx;
    CMObject obj(ctx, cmAddPath, fsiFruPath);
    EXPECT_EQ(obj.getPath(), cmAddPath);
}

// Progress: initial status is NotStarted

TEST(CMObjectTest, InitialStatusIsNotStarted)
{
    sdbusplus::async::context ctx;
    CMObject obj(ctx, cmRemovePath, fsiFruPath);
    EXPECT_EQ(obj.getStatus(), OperationStatus::NotStarted);
}

// Progress: execute() drives NotStarted -> InProgress -> Completed

TEST(CMObjectTest, ExecuteRemoveFSISetsCompleted)
{
    sdbusplus::async::context ctx;
    CMObject obj(ctx, cmRemovePath, fsiFruPath);
    const FRUOperations* ops = FRUIdentifier::identifyType(fsiInterfaces,
                                                           fsiFruPath);
    ASSERT_NE(ops, nullptr);
    EXPECT_NO_THROW(stdexec::sync_wait(obj.execute(true, *ops)));
    EXPECT_EQ(obj.getStatus(), OperationStatus::Completed);
}

TEST(CMObjectTest, ExecuteAddFSISetsCompleted)
{
    sdbusplus::async::context ctx;
    CMObject obj(ctx, cmAddPath, fsiFruPath);
    const FRUOperations* ops = FRUIdentifier::identifyType(fsiInterfaces,
                                                           fsiFruPath);
    ASSERT_NE(ops, nullptr);
    EXPECT_NO_THROW(stdexec::sync_wait(obj.execute(false, *ops)));
    EXPECT_EQ(obj.getStatus(), OperationStatus::Completed);
}

TEST(CMObjectTest, ExecuteRemoveBMCSetsCompleted)
{
    sdbusplus::async::context ctx;
    CMObject obj(ctx, cmRemovePath, bmcFruPath);
    const FRUOperations* ops = FRUIdentifier::identifyType(bmcInterfaces,
                                                           bmcFruPath);
    ASSERT_NE(ops, nullptr);
    EXPECT_NO_THROW(stdexec::sync_wait(obj.execute(true, *ops)));
    EXPECT_EQ(obj.getStatus(), OperationStatus::Completed);
}

TEST(CMObjectTest, ExecuteAddBMCSetsCompleted)
{
    sdbusplus::async::context ctx;
    CMObject obj(ctx, cmAddPath, bmcFruPath);
    const FRUOperations* ops = FRUIdentifier::identifyType(bmcInterfaces,
                                                           bmcFruPath);
    ASSERT_NE(ops, nullptr);
    EXPECT_NO_THROW(stdexec::sync_wait(obj.execute(false, *ops)));
    EXPECT_EQ(obj.getStatus(), OperationStatus::Completed);
}

TEST(CMObjectTest, ExecuteRemoveSwitchboardSetsCompleted)
{
    sdbusplus::async::context ctx;
    CMObject obj(ctx, cmRemovePath, switchboardFruPath);
    const FRUOperations* ops =
        FRUIdentifier::identifyType(switchboardInterfaces, switchboardFruPath);
    ASSERT_NE(ops, nullptr);
    EXPECT_NO_THROW(stdexec::sync_wait(obj.execute(true, *ops)));
    EXPECT_EQ(obj.getStatus(), OperationStatus::Completed);
}

TEST(CMObjectTest, ExecuteAddSwitchboardSetsCompleted)
{
    sdbusplus::async::context ctx;
    CMObject obj(ctx, cmAddPath, switchboardFruPath);
    const FRUOperations* ops =
        FRUIdentifier::identifyType(switchboardInterfaces, switchboardFruPath);
    ASSERT_NE(ops, nullptr);
    EXPECT_NO_THROW(stdexec::sync_wait(obj.execute(false, *ops)));
    EXPECT_EQ(obj.getStatus(), OperationStatus::Completed);
}

// Progress: updateStatus transitions

TEST(CMObjectTest, UpdateStatusToInProgress)
{
    sdbusplus::async::context ctx;
    CMObject obj(ctx, cmRemovePath, fsiFruPath);
    obj.updateStatus(OperationStatus::InProgress);
    EXPECT_EQ(obj.getStatus(), OperationStatus::InProgress);
}

TEST(CMObjectTest, UpdateStatusToFailed)
{
    sdbusplus::async::context ctx;
    CMObject obj(ctx, cmRemovePath, fsiFruPath);
    obj.updateStatus(OperationStatus::Failed);
    EXPECT_EQ(obj.getStatus(), OperationStatus::Failed);
}

TEST(CMObjectTest, IdentifyTypeUnknownFRUReturnsNull)
{
    const FRUOperations* ops = FRUIdentifier::identifyType(unknownInterfaces,
                                                           unknownFruPath);
    EXPECT_EQ(ops, nullptr);
}

// Manager (parent Progress): initial status is NotStarted

TEST(ManagerProgressTest, InitialStatusIsNotStarted)
{
    sdbusplus::async::context ctx;
    Manager manager(ctx);
    EXPECT_EQ(manager.getStatus(), OperationStatus::NotStarted);
}

// Manager (parent Progress): updateStatus transitions

TEST(ManagerProgressTest, UpdateStatusToInProgress)
{
    sdbusplus::async::context ctx;
    Manager manager(ctx);
    manager.updateStatus(OperationStatus::InProgress);
    EXPECT_EQ(manager.getStatus(), OperationStatus::InProgress);
}

TEST(ManagerProgressTest, UpdateStatusToCompleted)
{
    sdbusplus::async::context ctx;
    Manager manager(ctx);
    manager.updateStatus(OperationStatus::InProgress);
    manager.updateStatus(OperationStatus::Completed, 1234);
    EXPECT_EQ(manager.getStatus(), OperationStatus::Completed);
}

TEST(ManagerProgressTest, UpdateStatusToFailed)
{
    sdbusplus::async::context ctx;
    Manager manager(ctx);
    manager.updateStatus(OperationStatus::InProgress);
    manager.updateStatus(OperationStatus::Failed, 1234);
    EXPECT_EQ(manager.getStatus(), OperationStatus::Failed);
}

// Manager (parent Progress): full remove->add cycle drives Completed

TEST(ManagerProgressTest, RemoveStartsInProgressAddCompletesCompleted)
{
    sdbusplus::async::context ctx;
    Manager manager(ctx);

    /* Simulate remove starting */
    manager.updateStatus(OperationStatus::InProgress);
    EXPECT_EQ(manager.getStatus(), OperationStatus::InProgress);

    /* Simulate add finishing successfully, reusing the add object's time */
    CMObject addObj(ctx, cmAddPath, fsiFruPath);
    const FRUOperations* ops = FRUIdentifier::identifyType(fsiInterfaces,
                                                           fsiFruPath);
    ASSERT_NE(ops, nullptr);
    stdexec::sync_wait(addObj.execute(false, *ops));

    manager.updateStatus(OperationStatus::Completed, addObj.getCompletedTime());
    EXPECT_EQ(manager.getStatus(), OperationStatus::Completed);
    EXPECT_NE(addObj.getCompletedTime(), 0U);
    EXPECT_EQ(manager.getCompletedTime(), addObj.getCompletedTime());
}

// Manager (parent Progress): NotStarted clears start/completed times

TEST(ManagerProgressTest, NotStartedResetsTimes)
{
    sdbusplus::async::context ctx;
    Manager manager(ctx);

    manager.updateStatus(OperationStatus::InProgress);
    manager.updateStatus(OperationStatus::Completed, 1234);
    EXPECT_EQ(manager.getCompletedTime(), 1234U);

    manager.updateStatus(OperationStatus::NotStarted);
    EXPECT_EQ(manager.getStatus(), OperationStatus::NotStarted);
    EXPECT_EQ(manager.getStartTime(), 0U);
    EXPECT_EQ(manager.getCompletedTime(), 0U);
}

// Manager (parent Progress): failed remove drives Failed

TEST(ManagerProgressTest, RemoveFailureDrivesParentToFailed)
{
    sdbusplus::async::context ctx;
    Manager manager(ctx);

    manager.updateStatus(OperationStatus::InProgress);
    manager.updateStatus(OperationStatus::Failed, 1234);
    EXPECT_EQ(manager.getStatus(), OperationStatus::Failed);
    EXPECT_EQ(manager.getCompletedTime(), 1234U);
}

} // namespace concurrent_maintenance
