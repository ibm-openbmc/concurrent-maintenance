#include "manager.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>

namespace concurrent_maintenance
{

Manager::Manager(sdbusplus::async::context& ctx) :
    ctx(ctx), currentCMObject(nullptr)
{
    lg2::info("Concurrent Maintenance manager initialized");

    // Create property change signal match for ReadyToRemove property
    // TODO:
    // Currently, this will match all child objects under
    // /xyz/openbmc_project/inventory and checks for "Availabile" property
    // change until ReadyToRemove property is advertised. This should be changed
    // to "ReadyToRemove" property change under the right interface
    auto& bus = ctx.get_bus();

    readyToRemoveMatch = std::make_unique<sdbusplus::bus::match_t>(
        bus,
        sdbusplus::bus::match::rules::propertiesChangedNamespace(
            "/xyz/openbmc_project/inventory",
            "xyz.openbmc_project.State.Decorator.Availability"),
        [this](sdbusplus::message_t& msg) {
        this->handleReadyToRemoveChange(msg);
    });

    lg2::info(
        "ReadyToRemove property watcher registered for all inventory objects");
}

void Manager::handleReadyToRemoveChange(sdbusplus::message_t& msg)
{
    std::string interface;
    std::map<std::string, std::variant<bool>> changedProperties;

    try
    {
        msg.read(interface, changedProperties);

        // TODO: Change back to "ReadyToRemove" when backend is ready
        // auto it = changedProperties.find("ReadyToRemove");
        // Temporarily using "Available" property for testing
        auto it = changedProperties.find("Available");
        if (it != changedProperties.end())
        {
            bool readyToRemove = std::get<bool>(it->second);
            std::string objectPath = msg.get_path();
            lg2::info("ReadyToRemove property changed on {PATH}: {VALUE}",
                      "PATH", objectPath, "VALUE", readyToRemove);

            // Manage CM object based on property value
            manageCMObject(readyToRemove);
        }
    }
    catch (const std::exception& e)
    {
        lg2::error("Error handling ReadyToRemove property change: {ERROR}",
                   "ERROR", e.what());
    }
}

void Manager::manageCMObject(bool readyToRemove)
{
    try
    {
        std::string cmObjectPath;

        // When readyToRemove=true, the device is ready to be removed
        // (create "remove" object)
        // When readyToRemove=false, device has been added back
        // (create "add" object)
        if (readyToRemove)
        {
            cmObjectPath = "/com/ibm/concurrent_maintenance/remove";
        }
        else
        {
            cmObjectPath = "/com/ibm/concurrent_maintenance/add";
        }

        // Check if a CM object already exists
        if (currentCMObject)
        {
            lg2::error(
                "CM is already in progress. Object already exists at path: {PATH}.",
                "PATH", currentCMObject->getPath());
            return;
        }

        lg2::info("Creating CM object at path: {PATH}", "PATH", cmObjectPath);

        // Create the actual D-Bus object
        // The ObjectManager in main.cpp will automatically detect and manage it
        currentCMObject = std::make_unique<CMObject>(ctx, cmObjectPath);

        lg2::info("CM object created at {PATH}", "PATH", cmObjectPath);
    }
    catch (const std::exception& e)
    {
        lg2::error("Error managing CM object: {ERROR}", "ERROR", e.what());
    }
}

} // namespace concurrent_maintenance
