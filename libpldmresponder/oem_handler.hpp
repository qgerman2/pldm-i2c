#pragma once

#include "common/utils.hpp"
#include "pldmd/handler.hpp"

namespace pldm
{

namespace responder
{

namespace oem_platform
{

class Handler : public CmdHandler
{
  public:
    Handler(const pldm::utils::DBusHandler* dBusIntf) : dBusIntf(dBusIntf)
    {}

    /** @brief Interface to get the state sensor readings requested by pldm
     *  requester for OEM types. Each specific type should implement a handler
     *  of it's own
     *
     *  @param[in] entityType - entity type corresponding to the sensor
     *  @param[in] entityInstance - entity instance number
     *  @param[in] stateSetId - state set id
     *  @param[in] compSensorCnt - composite sensor count
     *  @param[out] stateField - The state field data for each of the states,
     *                           equal to composite sensor count in number
     *  @return - Success or failure in getting the states. Returns failure in
     *            terms of PLDM completion codes if fetching atleast one state
     *            fails
     */
    virtual int getOemStateSensorReadingsHandler(
        uint16_t entityType, uint16_t entityInstance, uint16_t stateSetId,
        uint8_t compSensorCnt,
        std::vector<get_sensor_state_field>& stateField) = 0;

    /** @brief Interface to set the effecter requested by pldm requester
     *         for OEM types. Each individual oem type should implement
     *         it's own handler.
     *  @param[in] entityType - entity type corresponding to the effecter id
     *  @param[in] entityInstance - entity instance
     *  @param[in] stateSetId - state set id
     *  @param[in] compEffecterCnt - composite effecter count
     *  param[in] stateField - The state field data for each of the states,
     *                         equal to compEffecterCnt in number
     *  @return - Success or failure in setting the states.Returns failure in
     *            terms of PLDM completion codes if atleast one state fails to
     *            be set
     */

    virtual int OemSetStateEffecterStatesHandler(
        uint16_t entityType, uint16_t entityInstance, uint16_t stateSetId,
        uint8_t compEffecterCnt,
        std::vector<set_effecter_state_field>& stateField) = 0;

    /** @brief Interface to generate the OEM PDRs if OEM_IBM is defined
     *
     * @param[in] repo - instance of concrete implementation of Repo
     */
    virtual void buildOEMPDR(pdr_utils::Repo& repo) = 0;
    virtual ~Handler()
    {}

  protected:
    const pldm::utils::DBusHandler* dBusIntf;
};

} // namespace oem_platform

} // namespace responder

} // namespace pldm
