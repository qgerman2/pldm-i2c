#pragma once
#include "libpldm/platform.h"

#include "inband_code_update.hpp"
#include "libpldmresponder/oem_handler.hpp"
#include "libpldmresponder/pdr_utils.hpp"
#include "libpldmresponder/platform.hpp"

#include <memory>
#include <vector>
namespace pldm
{

namespace responder
{

namespace oem_ibm_platform
{

static constexpr auto PLDM_OEM_IBM_BOOT_STATE = 32769;
static constexpr auto PLDM_OEM_IBM_FIRMWARE_UPDATE_STATE = 32768;
static constexpr auto PLDM_OEM_IBM_ENTITY_FIRMWARE_UPDATE = 24577;
static constexpr auto PLDM_OEM_IBM_VERIFICATION_STATE = 32770;
constexpr uint16_t ENTITY_INSTANCE_0 = 0;
constexpr uint16_t ENTITY_INSTANCE_1 = 1;

enum codeUpdateStateValues
{
    START = 0x1,
    END = 0x2,
    FAIL = 0x3,
    ABORT = 0x4,
    ACCEPT = 0x5,
    REJECT = 0x6,
};

enum VerificationStateValues
{
    VALID = 0x0,
    ENTITLEMENT_FAIL = 0x1,
    BANNED_PLATFORM_FAIL = 0x2,
    MIN_MIF_FAIL = 0x4,
};

class Handler : public oem_platform::Handler
{
  public:
    Handler(const pldm::utils::DBusHandler* dBusIntf,
            pldm::responder::CodeUpdate* codeUpdate, int mctp_fd,
            uint8_t mctp_eid, Requester& requester, sdeventplus::Event& event) :
        oem_platform::Handler(dBusIntf),
        codeUpdate(codeUpdate), platformHandler(nullptr), mctp_fd(mctp_fd),
        mctp_eid(mctp_eid), requester(requester), event(event)
    {
        codeUpdate->setVersions();
    }

    int getOemStateSensorReadingsHandler(
        uint16_t entityType, uint16_t entityInstance, uint16_t stateSetId,
        uint8_t compSensorCnt, std::vector<get_sensor_state_field>& stateField);

    int OemSetStateEffecterStatesHandler(
        uint16_t entityType, uint16_t entityInstance, uint16_t stateSetId,
        uint8_t compEffecterCnt,
        std::vector<set_effecter_state_field>& stateField, uint16_t effecterId);

    /** @brief Method to set the platform handler in the
     *         oem_ibm_handler class
     *  @param[in] handler - pointer to PLDM platform handler
     */
    void setPlatformHandler(pldm::responder::platform::Handler* handler);

    //  bool isCodeUpdateInProgress();

    //   std::string fetchCurrentBootSide();

    uint16_t getNextEffecterId()
    {
        return platformHandler->getNextEffecterId();
    }

    uint16_t getNextSensorId()
    {
        return platformHandler->getNextSensorId();
    }

    /** @brief Method to Generate the OEM PDRs if OEM_IBM is defined
     *
     * @param[in] repo - instance of concrete implementation of Repo
     */
    void buildOEMPDR(pdr_utils::Repo& repo);

    /** @brief Method to send code update event to host
     *  @param[in] sensorId - sendor ID
     *  @param[in] opState - new code update operational state
     *  @param[in] previousOpState - previous code update operational state
     *  @return void
     */
    void sendCodeUpdateEvent(uint16_t effecterId, codeUpdateStateValues opState,
                             codeUpdateStateValues previousOpState);
    void sendStateSensorEvent(uint16_t sensorId,
                              enum sensor_event_class_states sensorEventClass,
                              uint8_t sensorOffset, uint8_t eventState,
                              uint8_t prevEventState);

    /** @brief Method to send encoded request msg of code update event to host
     *  @param[in] requestMsg - encoded request msg
     *  @return PLDM status code
     */
    int sendEventToHost(std::vector<uint8_t>& requestMsg);

    void _processEndUpdate(sdeventplus::source::EventBase& source);

    ~Handler()
    {}

    pldm::responder::CodeUpdate* codeUpdate; //!< pointer to CodeUpdate object
    pldm::responder::platform::Handler*
        platformHandler; //!< pointer to PLDM platform handler

    /** @brief fd of MCTP communications socket */
    int mctp_fd;

    /** @brief MCTP EID of host firmware */
    uint8_t mctp_eid;

    /** @brief reference to Requester object, primarily used to access API to
     *  obtain PLDM instance id.
     */
    Requester& requester;
    /** @brief sdeventplus event source */
    std::unique_ptr<sdeventplus::source::Defer> assembleImageEvent;
    sdeventplus::Event& event;
};

/** @brief Method to encode code update event msg
 *  @param[in] eventType - type of event
 *  @param[in] eventDataVec - vector of event data to be sent to host
 *  @param[in] instanceId - instance ID
 *  @param[in/out] requestMsg - request msg to be encoded
 *  @return PLDM status code
 */
int encodeEventMsg(uint8_t eventType, const std::vector<uint8_t>& eventDataVec,
                   std::vector<uint8_t>& requestMsg, uint8_t instanceId);

} // namespace oem_ibm_platform

} // namespace responder

} // namespace pldm
