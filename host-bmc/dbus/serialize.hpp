#pragma once

#include "type.hpp"

#include <libpldm/pdr.h>

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>
#include <sdbusplus/server/object.hpp>

#include <filesystem>
#include <fstream>

namespace pldm
{
namespace serialize
{
namespace fs = std::filesystem;

using ObjectPath = fs::path;
using ObjectPathMaps = std::map<ObjectPath, pldm_entity_node*>;
using namespace pldm::dbus;

/** @class Serialize
 *  @brief This class is used for Store and restore the data
 */
class Serialize
{
  private:
    Serialize()
    {
        deserialize();
    }

  public:
    /** @brief Single object is required to handle store/retrieve data
     *       so, restricting creation/copying of the class object
     *
     */
    Serialize(const Serialize&) = delete;
    Serialize(Serialize&&) = delete;
    Serialize& operator=(const Serialize&) = delete;
    Serialize& operator=(Serialize&&) = delete;
    ~Serialize() = default;

    /** @brief Managing single static object throughout the process
     *
     * @returns singletone object to manage the data
     */
    static Serialize& getSerialize()
    {
        static Serialize serialize;
        return serialize;
    }

    /** @brief Storing data using cereal data storing mechanism.
     *
     *  @param[in] path - Dbus path
     *  @param[in] intf - Interface path over dbus
     *  @param[in] name - Property Name over interface
     *  @param[in] value - Property Value over interface
     */
    void serialize(const std::string& path, const std::string& intf,
                   const std::string& name = "",
                   dbus::PropertyValue value = {});

    /** @brief Store the data as Key and property value pair
     *
     *  @param[in] key - Key to store property value
     *  @param[in] Property - property value will be stored in map
     */
    void serializeKeyVal(const std::string& key, dbus::PropertyValue value);

    /** @brief This function is to Read all values from binary file.
     *  @return true is success else false.
     */
    bool deserialize();

    /** @brief This function is to get stored structured data object.
     *  @return returning structured data object.
     */
    dbus::SavedObjs getSavedObjs()
    {
        return savedObjs;
    }

    /** @brief This function is to get all saved key pair values
     *   @return returning map of property and values
     */
    std::map<std::string, pldm::dbus::PropertyValue> getSavedKeyVals()
    {
        return savedKeyVal;
    }

    /** @brief Removing  entity data from file, and storing back
     *
     *  @param[in] types - entity types to remove data from file
     */
    void reSerialize(const std::vector<uint16_t> types);

    /** @brief Storing types of entity
     *
     *  @param[in] storeEntities - Map of Dbus objects mappings
     */
    void setEntityTypes(const std::set<uint16_t>& storeEntities);

    /** @brief This function to store entity information.
     */
    void setObjectPathMaps(const ObjectPathMaps& maps);

    /** @brief This function added to change file location for unit test only.
     *  Do not use for other purpose.
     */
    void setfilePathForUnitTest(fs::path path)
    {
        filePath = path;
    }

  private:
    dbus::SavedObjs savedObjs;
    fs::path filePath{PERSISTENT_FILE};
    std::set<uint16_t> storeEntityTypes;
    std::map<ObjectPath, pldm_entity> entityPathMaps;
    std::map<std::string, pldm::dbus::PropertyValue> savedKeyVal;
};

} // namespace serialize
} // namespace pldm
