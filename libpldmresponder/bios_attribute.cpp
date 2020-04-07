#include "config.h"

#include "bios_attribute.hpp"

#include "bios_config.hpp"
#include "utils.hpp"

#include <iostream>
#include <variant>

namespace pldm
{
namespace responder
{
namespace bios
{

BIOSAttribute::BIOSAttribute(const Json& entry,
                             const BIOSStringTable& stringTable,
                             DBusHandler* const dbusHandler) :
    name(entry.at("attribute_name")),
    attrNameHandle(stringTable.findHandle(name)),
    readOnly(!entry.contains("dbus")),
    attrHandle(pldm_bios_table_get_attr_handle()), dbusHandler(dbusHandler)
{
    if (!readOnly)
    {
        std::string objectPath = entry.at("dbus").at("object_path");
        std::string interface = entry.at("dbus").at("interface");
        std::string propertyName = entry.at("dbus").at("property_name");
        std::string propertyType = entry.at("dbus").at("property_type");

        dBusMap = {objectPath, interface, propertyName, propertyType};
    }
}

std::optional<DBusMapping> BIOSAttribute::getDBusMap()
{
    return dBusMap;
}

} // namespace bios
} // namespace responder
} // namespace pldm
