#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>

constexpr auto HOST_SOFTOFF_MCTP_ID = 0;
constexpr auto HOST_SOFTOFF_EFFECTER_ID = 0;
constexpr auto HOST_SOFTOFF_STATE = 0;

namespace pldm
{

/** @class SoftPowerOff
 *  @brief Responsible for coordinating Host SoftPowerOff operation
 */
class PldmSoftPowerOff
{
  public:
    /** @brief Constructs SoftPowerOff object.
     *
     *  @param[in] bus       - system dbus handler
     *  @param[in] event     - sd_event handler
     */
    PldmSoftPowerOff();

    /** @brief Send PLDM Set State Effecter States command.
     */
    int setStateEffecterStates();

    /** @brief Parser the json file to get timeout seconds.
     */
    void parserJsonFile();

  private:
    /** @brief Timeout seconds */
    int timeOutSeconds = 2700;
};

} // namespace pldm
