#pragma once

#include "xyz/openbmc_project/Common/error.hpp"

#include <stdint.h>
#include <systemd/sd-bus.h>
#include <unistd.h>

#include <exception>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/server.hpp>
#include <string>
#include <variant>
#include <vector>

#include "libpldm/base.h"

namespace pldm
{
namespace responder
{
namespace utils
{

/** @struct CustomFD
 *
 *  RAII wrapper for file descriptor.
 */
struct CustomFD
{
    CustomFD(const CustomFD&) = delete;
    CustomFD& operator=(const CustomFD&) = delete;
    CustomFD(CustomFD&&) = delete;
    CustomFD& operator=(CustomFD&&) = delete;

    CustomFD(int fd) : fd(fd)
    {
    }

    ~CustomFD()
    {
        if (fd >= 0)
        {
            close(fd);
        }
    }

    int operator()() const
    {
        return fd;
    }

  private:
    int fd = -1;
};

/** @brief Calculate the pad for PLDM data
 *
 *  @param[in] data - Length of the data
 *  @return - uint8_t - number of pad bytes
 */
uint8_t getNumPadBytes(uint32_t data);

} // namespace utils

/**
 *  @brief Get the DBUS Service name for the input dbus path
 *  @param[in] bus - DBUS Bus Object
 *  @param[in] path - DBUS object path
 *  @param[in] interface - DBUS Interface
 *  @return std::string - the dbus service name
 */
std::string getService(sdbusplus::bus::bus& bus, const std::string& path,
                       const std::string& interface);

/** @brief Convert any Decimal number to BCD
 *
 *  @tparam[in] decimal - Decimal number
 *  @return Corresponding BCD number
 */
template <typename T>
T decimalToBcd(T decimal)
{
    T bcd = 0;
    T rem = 0;
    auto cnt = 0;

    while (decimal)
    {
        rem = decimal % 10;
        bcd = bcd + (rem << cnt);
        decimal = decimal / 10;
        cnt += 4;
    }

    return bcd;
}

constexpr auto dbusProperties = "org.freedesktop.DBus.Properties";

/**
 *  @class DBusHandler
 *
 *  Expose API to handle the D-Bus calls
 *
 *  This class contains the APIs to handle the D-Bus calls
 *  to cater the request from pldm requester.
 *  A class is created to mock the apis in the test cases
 */
class DBusHandler
{
  public:
    /** @brief API to set a D-Bus property
     *
     *  @param[in] objPath - Object path for the D-Bus object
     *  @param[in] dbusProp - The D-Bus property
     *  @param[in] dbusInterface - The D-Bus interface
     *  @param[in] value - The value to be set
     *  @return - int - success when the property is set. Throws exception on
     * failure
     *  */
    int setDbusProperty(const char* objPath, const char* dbusProp,
                        const std::string& dbusInterface,
                        const std::variant<std::string>& value) const
    {
        using namespace phosphor::logging;
        auto bus = sdbusplus::bus::new_default();
        auto service = getService(bus, objPath, dbusInterface);
        auto method = bus.new_method_call(service.c_str(), objPath,
                                          dbusProperties, "Set");
        method.append(dbusInterface, dbusProp, value);
        bus.call_noreply(method);
        return PLDM_SUCCESS;
    }
};

} // namespace responder
} // namespace pldm
