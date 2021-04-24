#include "common/utils.hpp"

#include "libpldm/entity.h"

#include "utils.hpp"

#include <iostream>

namespace fs = std::filesystem;
namespace pldm
{
namespace hostbmc
{
namespace utils
{

Entities getParentEntites(const EntityAssociations& entityAssoc)
{
    Entities parents{};
    for (const auto& et : entityAssoc)
    {
        parents.push_back(et[0]);
    }

    for (auto it = parents.begin(); it != parents.end();)
    {
        bool find = false;
        for (const auto& evs : entityAssoc)
        {
            for (size_t i = 1; i < evs.size(); i++)
            {
                if (evs[i].entity_type == it->entity_type &&
                    evs[i].entity_instance_num == it->entity_instance_num &&
                    evs[i].entity_container_id == it->entity_container_id)
                {
                    find = true;
                    break;
                }
            }
            if (find)
            {
                break;
            }
        }
        if (find)
        {
            it = parents.erase(it);
        }
        else
        {
            it++;
        }
    }

    return parents;
}

void addObjectPathEntityAssociations(const EntityAssociations& entityAssoc,
                                     const pldm_entity& entity,
                                     const ObjectPath& path,
                                     ObjectPathMaps& objPathMap)
{
    bool find = false;
    if (!entityMaps.contains(entity.entity_type))
    {
        std::cerr
            << "No found entity type from the std::map of entityMaps...\n";
        return;
    }

    std::string entityName = entityMaps.at(entity.entity_type);
    std::string entityNum = std::to_string(
        entity.entity_instance_num > 0 ? entity.entity_instance_num - 1 : 0);

    for (auto& ev : entityAssoc)
    {
        if (ev[0].entity_instance_num == entity.entity_instance_num &&
            ev[0].entity_type == entity.entity_type)
        {
            ObjectPath p = path / fs::path{entityName + entityNum};

            for (size_t i = 1; i < ev.size(); i++)
            {
                addObjectPathEntityAssociations(entityAssoc, ev[i], p,
                                                objPathMap);
            }
            find = true;
        }
    }

    if (!find)
    {
        objPathMap.emplace(path / fs::path{entityName + entityNum}, entity);
    }
}
void setCoreCount(const EntityAssociations& Associations)
{
    static constexpr auto searchpath = "/xyz/openbmc_project/";
    int depth = 0;
    std::vector<std::string> cpuInterface = {
        "xyz.openbmc_project.Inventory.Item.Cpu"};
    pldm::utils::MapperGetSubTreeResponse response =
        pldm::utils::DBusHandler().getSubtree(searchpath, depth, cpuInterface);

    // get the CPU pldm entities
    for (const auto& entries : Associations)
    {
        // entries[0] would be the parent in the entity association map
        if (entries[0].entity_type == PLDM_ENTITY_PROC)
        {
            int corecount = 0;
            for (const auto& entry : entries)
            {
                if (entry.entity_type == (PLDM_ENTITY_PROC | 0x8000))
                {
                    // got a core child
                    ++corecount;
                }
            }

            std::string grepWord =
                entityMaps.at(entries[0].entity_type) +
                std::to_string(entries[0].entity_instance_num);
            for (const auto& [objectPath, serviceMap] : response)
            {
                // find the object path with first occurance of coreX
                if (objectPath.find(grepWord) != std::string::npos)
                {
                    pldm::utils::DBusMapping dbusMapping{
                        objectPath, cpuInterface[0], "CoreCount", "uint16_t"};
                    pldm::utils::PropertyValue value =
                        static_cast<uint16_t>(corecount);
                    try
                    {
                        pldm::utils::DBusHandler().setDbusProperty(dbusMapping,
                                                                   value);
                    }
                    catch (const std::exception& e)
                    {
                        std::cerr << "failed to set the core count property "
                                  << objPath.c_str() << " ERROR=" << e.what()
                                  << "\n";
                    }
                }
            }
        }
    }
}
} // namespace utils
} // namespace hostbmc
} // namespace pldm
