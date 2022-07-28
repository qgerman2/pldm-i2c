#pragma once

#include <filesystem>
#include <iostream>

namespace pldm
{
namespace responder
{
static constexpr auto compatibleInterface =
    "xyz.openbmc_project.Configuration.IBMCompatibleSystem";
static constexpr auto namesProperty = "Names";
namespace fs = std::filesystem;
class SystemConfig
{
  public:
    SystemConfig() = delete;
    SystemConfig(const SystemConfig&) = delete;
    SystemConfig(SystemConfig&&) = delete;
    SystemConfig& operator=(const SystemConfig&) = delete;
    SystemConfig& operator=(SystemConfig&&) = delete;
    ~SystemConfig() = default;

    SystemConfig(const pldm::utils::DBusHandler* dBusIntf) : dBusIntf(dBusIntf)
    {
        ibmCompatibleMatchConfig =
            std::make_unique<sdbusplus::bus::match::match>(
                dBusIntf->getBus(),
                sdbusplus::bus::match::rules::interfacesAdded() +
                    sdbusplus::bus::match::rules::sender(
                        "xyz.openbmc_project.EntityManager"),
                std::bind_front(&SystemConfig::ibmCompatibleAddedCallback,
                                this));
    }

    /** @brief Method to get the system type information
     *
     *  @return[std::filesystem::path] - the system type information
     */
    std::filesystem::path getConfigDir()
    {
        if (!systemType.empty())
        {
            return fs::path{systemType};
        }

        static constexpr auto orgFreeDesktopInterface =
            "org.freedesktop.DBus.Properties";
        static constexpr auto getMethod = "Get";

        static constexpr auto searchpath = "/xyz/openbmc_project/";
        int depth = 0;
        std::vector<std::string> ibmCompatible = {compatibleInterface};
        pldm::utils::GetSubTreeResponse response;
        try
        {
            response = pldm::utils::DBusHandler().getSubtree(searchpath, depth,
                                                             ibmCompatible);
        }
        catch (const std::exception& e)
        {
            std::cerr << "failed to make a call to object mapper, ERROR="
                      << e.what() << "\n";
            return fs::path();
        }

        auto& bus = pldm::utils::DBusHandler::getBus();
        std::variant<std::vector<std::string>> value;

        for (const auto& [objectPath, serviceMap] : response)
        {
            try
            {
                auto method = bus.new_method_call(
                    serviceMap[0].first.c_str(), objectPath.c_str(),
                    orgFreeDesktopInterface, getMethod);
                method.append(ibmCompatible[0].c_str(), namesProperty);
                auto reply = bus.call(method);
                reply.read(value);
                return fs::path{std::get<std::vector<std::string>>(value)[0]};
            }
            catch (const std::exception& e)
            {
                std::cerr << "Error getting Names property , PATH="
                          << objectPath
                          << " Compatible interface =" << ibmCompatible[0]
                          << "\n";
            }
        }
        return fs::path();
    }

  private:
    /**@ brief system type/model */
    std::string systemType;

    /** @brief D-Bus Interfaced added signal match for Entity Manager */
    std::unique_ptr<sdbusplus::bus::match::match> ibmCompatibleMatchConfig;

    /** @brief D-Bus Interface object*/
    const pldm::utils::DBusHandler* dBusIntf;

    /** @brief callback function invoked when interfaces get added from
     *     Entity manager
     *
     *  @param[in] msg - Data associated with subscribed signal
     */
    void ibmCompatibleAddedCallback(sdbusplus::message::message& msg)
    {
        sdbusplus::message::object_path path;
        std::map<std::string,
                 std::map<std::string, std::variant<std::vector<std::string>>>>
            interfaces;

        msg.read(path, interfaces);

        if (!interfaces.contains(compatibleInterface))
        {
            return;
        }
        // Get the "Name" property value of the
        // "xyz.openbmc_project.Configuration.IBMCompatibleSystem" interface
        const auto& properties = interfaces.at(compatibleInterface);

        if (!properties.contains(namesProperty))
        {
            return;
        }
        auto names =
            std::get<std::vector<std::string>>(properties.at(namesProperty));

        // get only the first system type
        if (!names.empty())
        {
            systemType = names.front();
        }

        ibmCompatibleMatchConfig.reset();
    }
};

} // namespace responder
} // namespace pldm
