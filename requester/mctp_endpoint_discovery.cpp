#include "mctp_endpoint_discovery.hpp"

#include "libpldm/requester/pldm.h"

#include "common/types.hpp"
#include "common/utils.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace pldm
{
const std::string EMPTY_UUID = "00000000-0000-0000-0000-000000000000";

MctpDiscovery::MctpDiscovery(
    sdbusplus::bus::bus& bus,
    std::initializer_list<IMctpDiscoveryHandlerIntf*> list) :
    bus(bus),
    mctpEndpointSignal(bus,
                       sdbusplus::bus::match::rules::interfacesAdded(
                           "/xyz/openbmc_project/mctp"),
                       std::bind_front(&MctpDiscovery::dicoverEndpoints, this)),
    handlers(list)
{
    dbus::ObjectValueTree objects;

    try
    {
        auto method = bus.new_method_call(
            "xyz.openbmc_project.MCTP.Control", "/xyz/openbmc_project/mctp",
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
        auto reply = bus.call(method);
        reply.read(objects);
    }
    catch (const std::exception& e)
    {
        return;
    }

    MctpInfos mctpInfos;

    for (const auto& [objectPath, interfaces] : objects)
    {
        for (const auto& [intfName, properties] : interfaces)
        {
            if (intfName == mctpEndpointIntfName)
            {
                if (properties.contains("EID") &&
                    properties.contains("SupportedMessageTypes"))
                {
                    auto eid = std::get<size_t>(properties.at("EID"));
                    auto types = std::get<std::vector<uint8_t>>(
                        properties.at("SupportedMessageTypes"));
                    if (std::find(types.begin(), types.end(), mctpTypePLDM) !=
                        types.end())
                    {
                        mctpInfos.emplace_back(MctpInfo(eid, EMPTY_UUID));
                    }
                }
            }
        }
    }

    handleMCTPEndpoints(mctpInfos);
}

void MctpDiscovery::dicoverEndpoints(sdbusplus::message::message& msg)
{
    constexpr std::string_view mctpEndpointIntfName{
        "xyz.openbmc_project.MCTP.Endpoint"};
    MctpInfos mctpInfos;

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
                auto eid = std::get<size_t>(properties.at("EID"));
                auto types = std::get<std::vector<uint8_t>>(
                    properties.at("SupportedMessageTypes"));
                if (std::find(types.begin(), types.end(), mctpTypePLDM) !=
                    types.end())
                {
                    mctpInfos.emplace_back(MctpInfo(eid, EMPTY_UUID));
                }
            }
        }
    }

    handleMCTPEndpoints(mctpInfos);
}

void MctpDiscovery::loadStaticEndpoints(const std::filesystem::path& jsonPath)
{
    if (!std::filesystem::exists(jsonPath))
    {
        std::cerr << "Static EIDs json file does not exist, PATH=" << jsonPath
                  << "\n";
        return;
    }

    std::ifstream jsonFile(jsonPath);
    auto data = nlohmann::json::parse(jsonFile, nullptr, false);
    if (data.is_discarded())
    {
        std::cerr << "Parsing json file failed, FILE=" << jsonPath << "\n";
        return;
    }

    MctpInfos mctpInfos;
    const nlohmann::json emptyJson{};
    const std::vector<nlohmann::json> emptyJsonArray{};
    auto endpoints = data.value("endpoints", emptyJsonArray);
    for (const auto& endpoint : endpoints)
    {
        const std::vector<uint8_t> emptyUnit8Array;
        auto eid = endpoint.value("EID", 0xFF);
        auto types = endpoint.value("SupportedMessageTypes", emptyUnit8Array);
        if (std::find(types.begin(), types.end(), mctpTypePLDM) != types.end())
        {
            mctpInfos.emplace_back(MctpInfo(eid, EMPTY_UUID));
        }
    }

    handleMCTPEndpoints(mctpInfos);
}

void MctpDiscovery::handleMCTPEndpoints(const MctpInfos& mctpInfos)
{
    if (mctpInfos.size() && handlers.size())
    {
        for (IMctpDiscoveryHandlerIntf* handler : handlers)
        {
            if (handler)
            {
                handler->handleMCTPEndpoints(mctpInfos);
            }
        }
    }
}

} // namespace pldm