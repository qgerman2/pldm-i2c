#include "config.h"

#include "mctp_endpoint_discovery.hpp"

#include "common/types.hpp"
#include "common/utils.hpp"

#include <linux/mctp.h>

#include <phosphor-logging/lg2.hpp>

#include <algorithm>
#include <expected>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <vector>

using namespace sdbusplus::bus::match::rules;

PHOSPHOR_LOG2_USING;

namespace pldm
{
MctpDiscovery::MctpDiscovery(
    sdbusplus::bus_t& bus,
    std::initializer_list<MctpDiscoveryHandlerIntf*> list) :
    bus(bus), mctpEndpointAddedSignal(
                  bus, interfacesAdded(MCTPPath),
                  std::bind_front(&MctpDiscovery::discoverEndpoints, this)),
    mctpEndpointRemovedSignal(
        bus, interfacesRemoved(MCTPPath),
        std::bind_front(&MctpDiscovery::removeEndpoints, this)),
    mctpEndpointPropChangedSignal(
        bus, propertiesChangedNamespace(MCTPPath, MCTPInterfaceCC),
        std::bind_front(&MctpDiscovery::propertiesChangedCb, this)),
    handlers(list)
{
    std::map<MctpInfo, Availability> currentMctpInfoMap;
    getMctpInfos(currentMctpInfoMap);
    for (const auto& mapIt : currentMctpInfoMap)
    {
        if (mapIt.second)
        {
            // Only add the available endpoints to the terminus
            // Let the propertiesChanged signal tells us when it comes back
            // to Available again
            addToExistingMctpInfos(MctpInfos(1, mapIt.first));
        }
    }
    handleMctpEndpoints(existingMctpInfos);
}

void MctpDiscovery::getMctpInfos(std::map<MctpInfo, Availability>& mctpInfoMap)
{
    // Find all implementations of the MCTP Endpoint interface
    pldm::utils::GetSubTreeResponse mapperResponse;
    try
    {
        mapperResponse = pldm::utils::DBusHandler().getSubtree(
            MCTPPath, 0, std::vector<std::string>({MCTPInterface}));
    }
    catch (const sdbusplus::exception_t& e)
    {
        error(
            "Failed to getSubtree call at path '{PATH}' and interface '{INTERFACE}', error - {ERROR} ",
            "ERROR", e, "PATH", MCTPPath, "INTERFACE", MCTPInterface);
        return;
    }

    for (const auto& [path, services] : mapperResponse)
    {
        for (const auto& serviceIter : services)
        {
            const std::string& service = serviceIter.first;
            const MctpEndpointProps& epProps =
                getMctpEndpointProps(service, path);
            const UUID& uuid = getEndpointUUIDProp(service, path);
            const Availability& availability =
                getEndpointConnectivityProp(path);
            auto types = std::get<MCTPMsgTypes>(epProps);
            if (std::find(types.begin(), types.end(), mctpTypePLDM) !=
                types.end())
            {
                auto mctpInfo =
                    MctpInfo(std::get<eid>(epProps), uuid, "",
                             std::get<NetworkId>(epProps), std::nullopt);
                searchConfigurationFor(mctpInfo);
                mctpInfoMap[std::move(mctpInfo)] = availability;
            }
        }
    }
}

MctpEndpointProps MctpDiscovery::getMctpEndpointProps(
    const std::string& service, const std::string& path)
{
    try
    {
        auto properties = pldm::utils::DBusHandler().getDbusPropertiesVariant(
            service.c_str(), path.c_str(), MCTPInterface);

        if (properties.contains("NetworkId") && properties.contains("EID") &&
            properties.contains("SupportedMessageTypes"))
        {
            auto networkId = std::get<NetworkId>(properties.at("NetworkId"));
            auto eid = std::get<mctp_eid_t>(properties.at("EID"));
            auto types = std::get<std::vector<uint8_t>>(
                properties.at("SupportedMessageTypes"));
            return MctpEndpointProps(networkId, eid, types);
        }
    }
    catch (const sdbusplus::exception_t& e)
    {
        error(
            "Error reading MCTP Endpoint property at path '{PATH}' and service '{SERVICE}', error - {ERROR}",
            "SERVICE", service, "PATH", path, "ERROR", e);
        return MctpEndpointProps(0, MCTP_ADDR_ANY, {});
    }

    return MctpEndpointProps(0, MCTP_ADDR_ANY, {});
}

UUID MctpDiscovery::getEndpointUUIDProp(const std::string& service,
                                        const std::string& path)
{
    try
    {
        auto properties = pldm::utils::DBusHandler().getDbusPropertiesVariant(
            service.c_str(), path.c_str(), EndpointUUID);

        if (properties.contains("UUID"))
        {
            return std::get<UUID>(properties.at("UUID"));
        }
    }
    catch (const sdbusplus::exception_t& e)
    {
        error(
            "Error reading Endpoint UUID property at path '{PATH}' and service '{SERVICE}', error - {ERROR}",
            "SERVICE", service, "PATH", path, "ERROR", e);
        return static_cast<UUID>(emptyUUID);
    }

    return static_cast<UUID>(emptyUUID);
}

Availability MctpDiscovery::getEndpointConnectivityProp(const std::string& path)
{
    Availability available = false;
    try
    {
        pldm::utils::PropertyValue propertyValue =
            pldm::utils::DBusHandler().getDbusPropertyVariant(
                path.c_str(), MCTPConnectivityProp, MCTPInterfaceCC);
        if (std::get<std::string>(propertyValue) == "Available")
        {
            available = true;
        }
    }
    catch (const sdbusplus::exception_t& e)
    {
        error(
            "Error reading Endpoint Connectivity property at path '{PATH}', error - {ERROR}",
            "PATH", path, "ERROR", e);
    }

    return available;
}

void MctpDiscovery::getAddedMctpInfos(sdbusplus::message_t& msg,
                                      MctpInfos& mctpInfos)
{
    using ObjectPath = sdbusplus::message::object_path;
    ObjectPath objPath;
    using Property = std::string;
    using PropertyMap = std::map<Property, dbus::Value>;
    std::map<std::string, PropertyMap> interfaces;
    std::string uuid = emptyUUID;

    try
    {
        msg.read(objPath, interfaces);
    }
    catch (const sdbusplus::exception_t& e)
    {
        error(
            "Error reading MCTP Endpoint added interface message, error - {ERROR}",
            "ERROR", e);
        return;
    }
    const Availability& availability = getEndpointConnectivityProp(objPath.str);

    /* Get UUID */
    try
    {
        auto service = pldm::utils::DBusHandler().getService(
            objPath.str.c_str(), EndpointUUID);
        uuid = getEndpointUUIDProp(service, objPath.str);
    }
    catch (const sdbusplus::exception_t& e)
    {
        error("Error getting Endpoint UUID D-Bus interface, error - {ERROR}",
              "ERROR", e);
    }

    for (const auto& [intfName, properties] : interfaces)
    {
        if (intfName == MCTPInterface)
        {
            if (properties.contains("NetworkId") &&
                properties.contains("EID") &&
                properties.contains("SupportedMessageTypes"))
            {
                auto networkId =
                    std::get<NetworkId>(properties.at("NetworkId"));
                auto eid = std::get<mctp_eid_t>(properties.at("EID"));
                auto types = std::get<std::vector<uint8_t>>(
                    properties.at("SupportedMessageTypes"));

                if (!availability)
                {
                    // Log an error message here, but still add it to the
                    // terminus
                    error(
                        "mctpd added a DEGRADED endpoint {EID} networkId {NET} to D-Bus",
                        "NET", networkId, "EID", static_cast<unsigned>(eid));
                }
                if (std::find(types.begin(), types.end(), mctpTypePLDM) !=
                    types.end())
                {
                    info(
                        "Adding Endpoint networkId '{NETWORK}' and EID '{EID}' UUID '{UUID}'",
                        "NETWORK", networkId, "EID", eid, "UUID", uuid);
                    auto mctpInfo =
                        MctpInfo(eid, uuid, "", networkId, std::nullopt);
                    searchConfigurationFor(mctpInfo);
                    mctpInfos.emplace_back(std::move(mctpInfo));
                }
            }
        }
    }
}

void MctpDiscovery::addToExistingMctpInfos(const MctpInfos& addedInfos)
{
    for (const auto& mctpInfo : addedInfos)
    {
        if (std::find(existingMctpInfos.begin(), existingMctpInfos.end(),
                      mctpInfo) == existingMctpInfos.end())
        {
            existingMctpInfos.emplace_back(mctpInfo);
        }
    }
}

void MctpDiscovery::removeFromExistingMctpInfos(MctpInfos& mctpInfos,
                                                MctpInfos& removedInfos)
{
    for (const auto& mctpInfo : existingMctpInfos)
    {
        if (std::find(mctpInfos.begin(), mctpInfos.end(), mctpInfo) ==
            mctpInfos.end())
        {
            removedInfos.emplace_back(mctpInfo);
        }
    }
    for (const auto& mctpInfo : removedInfos)
    {
        info("Removing Endpoint networkId '{NETWORK}' and  EID '{EID}'",
             "NETWORK", std::get<3>(mctpInfo), "EID", std::get<0>(mctpInfo));
        existingMctpInfos.erase(std::remove(existingMctpInfos.begin(),
                                            existingMctpInfos.end(), mctpInfo),
                                existingMctpInfos.end());
    }
}

void MctpDiscovery::propertiesChangedCb(sdbusplus::message_t& msg)
{
    using Interface = std::string;
    using Property = std::string;
    using Value = std::string;
    using Properties = std::map<Property, std::variant<Value>>;

    Interface interface;
    Properties properties;
    std::string objPath{};
    std::string service{};

    try
    {
        msg.read(interface, properties);
        objPath = msg.get_path();
    }
    catch (const sdbusplus::exception_t& e)
    {
        error(
            "Error handling Connectivity property changed message, error - {ERROR}",
            "ERROR", e);
        return;
    }

    for (const auto& [key, valueVariant] : properties)
    {
        Value propVal = std::get<std::string>(valueVariant);
        auto availability = (propVal == "Available") ? true : false;

        if (key == MCTPConnectivityProp)
        {
            service = pldm::utils::DBusHandler().getService(objPath.c_str(),
                                                            MCTPInterface);
            const MctpEndpointProps& epProps =
                getMctpEndpointProps(service, objPath);

            auto types = std::get<MCTPMsgTypes>(epProps);
            if (!std::ranges::contains(types, mctpTypePLDM))
            {
                return;
            }
            const UUID& uuid = getEndpointUUIDProp(service, objPath);

            MctpInfo mctpInfo(std::get<eid>(epProps), uuid, "",
                              std::get<NetworkId>(epProps), std::nullopt);
            searchConfigurationFor(mctpInfo);
            if (!std::ranges::contains(existingMctpInfos, mctpInfo))
            {
                if (availability)
                {
                    // The endpoint not in existingMctpInfos and is
                    // available Add it to existingMctpInfos
                    info(
                        "Adding Endpoint networkId {NETWORK} ID {EID} by propertiesChanged signal",
                        "NETWORK", std::get<3>(mctpInfo), "EID",
                        unsigned(std::get<0>(mctpInfo)));
                    addToExistingMctpInfos(MctpInfos(1, mctpInfo));
                    handleMctpEndpoints(MctpInfos(1, mctpInfo));
                }
            }
            else
            {
                // The endpoint already in existingMctpInfos
                updateMctpEndpointAvailability(mctpInfo, availability);
            }
        }
    }
}

void MctpDiscovery::discoverEndpoints(sdbusplus::message_t& msg)
{
    MctpInfos addedInfos;
    getAddedMctpInfos(msg, addedInfos);
    addToExistingMctpInfos(addedInfos);
    handleMctpEndpoints(addedInfos);
}

void MctpDiscovery::removeEndpoints(sdbusplus::message_t&)
{
    MctpInfos mctpInfos;
    MctpInfos removedInfos;
    std::map<MctpInfo, Availability> currentMctpInfoMap;
    getMctpInfos(currentMctpInfoMap);
    for (const auto& mapIt : currentMctpInfoMap)
    {
        mctpInfos.push_back(mapIt.first);
    }
    removeFromExistingMctpInfos(mctpInfos, removedInfos);
    handleRemovedMctpEndpoints(removedInfos);
    removeConfigs(removedInfos);
}

void MctpDiscovery::handleMctpEndpoints(const MctpInfos& mctpInfos)
{
    for (const auto& handler : handlers)
    {
        if (handler)
        {
            handler->handleConfigurations(configurations);
            handler->handleMctpEndpoints(mctpInfos);
        }
    }
}

void MctpDiscovery::handleRemovedMctpEndpoints(const MctpInfos& mctpInfos)
{
    for (const auto& handler : handlers)
    {
        if (handler)
        {
            handler->handleRemovedMctpEndpoints(mctpInfos);
        }
    }
}

void MctpDiscovery::updateMctpEndpointAvailability(const MctpInfo& mctpInfo,
                                                   Availability availability)
{
    for (const auto& handler : handlers)
    {
        if (handler)
        {
            handler->updateMctpEndpointAvailability(mctpInfo, availability);
        }
    }
}

/** @brief An internal helper function to get the name property from the
 * properties */
std::expected<std::string, std::string> getNameFromProperties(
    const utils::PropertyMap& properties)
{
    if (!properties.contains("Name"))
    {
        return std::unexpected("Missing name property");
    }
    return std::get<std::string>(properties.at("Name"));
}

void MctpDiscovery::searchConfigurationFor(MctpInfo& mctpInfo)
{
    const auto eidValue = std::get<eid>(mctpInfo);
    const auto networkId = std::get<NetworkId>(mctpInfo);
    const auto mctpReactorObjectPath =
        std::string{"/au/com/codeconstruct/mctp1/networks/"} +
        std::to_string(networkId) + "/endpoints/" + std::to_string(eidValue) +
        "/configured_by";
    std::string associatedObjPath;
    std::string associatedService;
    std::string associatedInterface;
    std::vector<std::string> interfaceFilter = {
        "xyz.openbmc_project.Configuration.MCTPI2CTarget",
        "xyz.openbmc_project.Configuration.MCTPI3CTarget"};
    std::string subtreePath = "/xyz/openbmc_project/inventory/system";
    try
    {
        //"/{board or chassis type}/{board or chassis}/{device}"
        auto constexpr subTreeDepth = 3;
        auto response = pldm::utils::DBusHandler().getAssociatedSubTree(
            mctpReactorObjectPath, subtreePath, subTreeDepth, interfaceFilter);
        if (response.empty())
        {
            error("No associated subtree found for path {PATH}", "PATH",
                  mctpReactorObjectPath);
            return;
        }
        associatedObjPath = response.begin()->first;
        auto associatedServiceProp = response.begin()->second;
        if (associatedServiceProp.empty())
        {
            error("No associated service found for path {PATH}", "PATH",
                  mctpReactorObjectPath);
            return;
        }
        associatedService = associatedServiceProp.begin()->first;
        auto dBusIntfList = associatedServiceProp.begin()->second;
        auto associatedInterfaceItr = std::find_if(
            dBusIntfList.begin(), dBusIntfList.end(),
            [&interfaceFilter](const auto& intf) {
                return std::find(interfaceFilter.begin(), interfaceFilter.end(),
                                 intf) != interfaceFilter.end();
            });
        if (associatedInterfaceItr == dBusIntfList.end())
        {
            error("No associated interface found for path {PATH}", "PATH",
                  mctpReactorObjectPath);
            return;
        }
        associatedInterface = *associatedInterfaceItr;
    }
    catch (const std::exception& e)
    {
        error("Failed to get associated subtree for path {PATH}: {ERROR}",
              "PATH", mctpReactorObjectPath, "ERROR", e);
    }
    try
    {
        auto mctpTargetProperties =
            pldm::utils::DBusHandler().getDbusPropertiesVariant(
                associatedService.c_str(), associatedObjPath.c_str(),
                associatedInterface.c_str());
        auto name = getNameFromProperties(mctpTargetProperties);
        if (!name)
        {
            error("Failed to get name property for path {PATH}: {ERROR}",
                  "PATH", associatedObjPath, "ERROR", name.error());
            return;
        }
        std::get<std::optional<std::string>>(mctpInfo) = *name;
        configurations.emplace(associatedObjPath, mctpInfo);
    }
    catch (const std::exception& e)
    {
        error("Failed to get PLDM device properties at path {PATH}: {ERROR}",
              "PATH", associatedObjPath, "ERROR", e);
    }
}

void MctpDiscovery::removeConfigs(const MctpInfos& removedInfos)
{
    for (const auto& mctpInfo : removedInfos)
    {
        auto eidToRemove = std::get<eid>(mctpInfo);
        std::erase_if(configurations, [eidToRemove](const auto& config) {
            auto& [__, mctpInfo] = config;
            auto eidValue = std::get<eid>(mctpInfo);
            return eidValue == eidToRemove;
        });
    }
}

} // namespace pldm
