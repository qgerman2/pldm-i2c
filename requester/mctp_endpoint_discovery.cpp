#include "mctp_endpoint_discovery.hpp"

#include "common/types.hpp"
#include "common/utils.hpp"

#include <algorithm>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace pldm
{
MctpDiscovery::MctpDiscovery(sdbusplus::bus_t& bus,
                             fw_update::Manager* fwManager) :
    bus(bus),
    fwManager(fwManager),
    mctpEndpointSignal(bus,
                       sdbusplus::bus::match::rules::interfacesAdded(
                           "/xyz/openbmc_project/mctp"),
                       std::bind_front(&MctpDiscovery::dicoverEndpoints, this))
{
    pldm::utils::ObjectValueTree objects;

    try
    {
        objects = pldm::utils::DBusHandler::getManagedObj(MCTPService,
                                                          MCTPPath);
    }
    catch (const std::exception& e)
    {
        error("Failed to call the D-Bus Method: {ERROR}", "ERROR", e);
        return;
    }

    std::vector<mctp_eid_t> eids;

    for (const auto& [objectPath, interfaces] : objects)
    {
        for (const auto& [intfName, properties] : interfaces)
        {
            if (intfName == mctpEndpointIntfName)
            {
                if (properties.contains("EID") &&
                    properties.contains("SupportedMessageTypes"))
                {
                    auto eid = std::get<mctp_eid_t>(properties.at("EID"));
                    auto types = std::get<std::vector<uint8_t>>(
                        properties.at("SupportedMessageTypes"));
                    if (std::find(types.begin(), types.end(), mctpTypePLDM) !=
                        types.end())
                    {
                        eids.emplace_back(eid);
                    }
                }
            }
        }
    }

    if (eids.size() && fwManager)
    {
        fwManager->handleMCTPEndpoints(eids);
    }
}

void MctpDiscovery::dicoverEndpoints(sdbusplus::message_t& msg)
{
    constexpr std::string_view mctpEndpointIntfName{
        "xyz.openbmc_project.MCTP.Endpoint"};
    std::vector<mctp_eid_t> eids;

    sdbusplus::message::object_path objPath;
    std::map<std::string, std::map<std::string, dbus::Value>> interfaces;
    msg.read(objPath, interfaces);

    for (const auto& [intfName, properties] : interfaces)
    {
        if (intfName == mctpEndpointIntfName)
        {
            if (properties.contains("EID") &&
                properties.contains("SupportedMessageTypes"))
            {
                auto eid = std::get<mctp_eid_t>(properties.at("EID"));
                auto types = std::get<std::vector<uint8_t>>(
                    properties.at("SupportedMessageTypes"));
                if (std::find(types.begin(), types.end(), mctpTypePLDM) !=
                    types.end())
                {
                    eids.emplace_back(eid);
                }
            }
        }
    }

    if (eids.size() && fwManager)
    {
        fwManager->handleMCTPEndpoints(eids);
    }
}

} // namespace pldm
