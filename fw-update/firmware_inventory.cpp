#include "firmware_inventory.hpp"

#include <iostream>

namespace pldm::fw_update::fw_inventory
{

Entry::Entry(sdbusplus::bus::bus& bus, const std::string& objPath,
             const std::string& versionStr) :
    Ifaces(bus, objPath.c_str(), true)
{
    Ifaces::version(versionStr, true);
    Ifaces::purpose(VersionPurpose::Other, true);
    Ifaces::emit_object_added();
}

void Entry::createInventoryAssociation(const std::string& invPath)
{
    auto assocs = associations();
    assocs.emplace_back(std::make_tuple("inventory", "activation", invPath));
    associations(assocs);
}

Manager::Manager(sdbusplus::bus::bus& bus,
                 const FirmwareInventoryInfo& firmwareInventoryInfo,
                 const ComponentInfoMap& componentInfoMap) :
    bus(bus),
    firmwareInventoryInfo(firmwareInventoryInfo),
    componentInfoMap(componentInfoMap)
{}

void Manager::createEntry(pldm::EID eid, const pldm::UUID& uuid,
                          ObjectPath /*objectPath*/)
{
    if (firmwareInventoryInfo.contains(uuid) && componentInfoMap.contains(eid))
    {
        auto fwInfoSearch = firmwareInventoryInfo.find(uuid);
        auto compInfoSearch = componentInfoMap.find(eid);

        for (const auto& [compKey, compInfo] : compInfoSearch->second)
        {
            if (fwInfoSearch->second.contains(compKey.second))
            {
                auto componentName = fwInfoSearch->second.find(compKey.second);
                std::string objPath =
                    softwareBasePath + "/" + componentName->second;
                auto entry = std::make_unique<Entry>(bus, objPath,
                                                     std::get<1>(compInfo));
                firmwareInventoryMap.emplace(
                    std::make_pair(eid, compKey.second), std::move(entry));
            }
        }
    }
    else
    {
        std::cerr << "device_inventory::createEntry::<undetected_uuid>"
                  << "\n";
    }
}

} // namespace pldm::fw_update::fw_inventory