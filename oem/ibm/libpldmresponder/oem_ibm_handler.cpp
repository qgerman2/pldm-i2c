#include "oem_ibm_handler.hpp"

#include "file_io_type_lid.hpp"
#include "libpldmresponder/file_io.hpp"
#include "libpldmresponder/pdr_utils.hpp"

#include <libpldm/entity.h>
#include <libpldm/entity_oem_ibm.h>

#include <phosphor-logging/lg2.hpp>

PHOSPHOR_LOG2_USING;

using namespace pldm::pdr;
using namespace pldm::utils;

namespace pldm
{
namespace responder
{
namespace oem_ibm_platform
{
int pldm::responder::oem_ibm_platform::Handler::
    getOemStateSensorReadingsHandler(
        EntityType entityType, EntityInstance entityInstance,
        StateSetId stateSetId, CompositeCount compSensorCnt,
        std::vector<get_sensor_state_field>& stateField)
{
    int rc = PLDM_SUCCESS;
    stateField.clear();

    for (size_t i = 0; i < compSensorCnt; i++)
    {
        uint8_t sensorOpState{};
        if (entityType == PLDM_OEM_IBM_ENTITY_FIRMWARE_UPDATE &&
            stateSetId == PLDM_OEM_IBM_BOOT_STATE)
        {
            sensorOpState = fetchBootSide(entityInstance, codeUpdate);
        }
        else
        {
            rc = PLDM_PLATFORM_INVALID_STATE_VALUE;
            break;
        }
        stateField.push_back({PLDM_SENSOR_ENABLED, PLDM_SENSOR_UNKNOWN,
                              PLDM_SENSOR_UNKNOWN, sensorOpState});
    }
    return rc;
}

int pldm::responder::oem_ibm_platform::Handler::
    oemSetStateEffecterStatesHandler(
        uint16_t entityType, uint16_t entityInstance, uint16_t stateSetId,
        uint8_t compEffecterCnt,
        std::vector<set_effecter_state_field>& stateField,
        uint16_t /*effecterId*/)
{
    int rc = PLDM_SUCCESS;

    for (uint8_t currState = 0; currState < compEffecterCnt; ++currState)
    {
        if (stateField[currState].set_request == PLDM_REQUEST_SET)
        {
            if (entityType == PLDM_OEM_IBM_ENTITY_FIRMWARE_UPDATE &&
                stateSetId == PLDM_OEM_IBM_BOOT_STATE)
            {
                rc = setBootSide(entityInstance, currState, stateField,
                                 codeUpdate);
            }
            else if (entityType == PLDM_OEM_IBM_ENTITY_FIRMWARE_UPDATE &&
                     stateSetId == PLDM_OEM_IBM_FIRMWARE_UPDATE_STATE)
            {
                if (stateField[currState].effecter_state ==
                    uint8_t(CodeUpdateState::START))
                {
                    codeUpdate->setCodeUpdateProgress(true);
                    startUpdateEvent =
                        std::make_unique<sdeventplus::source::Defer>(
                            event,
                            std::bind(std::mem_fn(&oem_ibm_platform::Handler::
                                                      _processStartUpdate),
                                      this, std::placeholders::_1));
                }
                else if (stateField[currState].effecter_state ==
                         uint8_t(CodeUpdateState::END))
                {
                    rc = PLDM_SUCCESS;
                    assembleImageEvent = std::make_unique<
                        sdeventplus::source::Defer>(
                        event,
                        std::bind(
                            std::mem_fn(
                                &oem_ibm_platform::Handler::_processEndUpdate),
                            this, std::placeholders::_1));

                    // sendCodeUpdateEvent(effecterId, END, START);
                }
                else if (stateField[currState].effecter_state ==
                         uint8_t(CodeUpdateState::ABORT))
                {
                    codeUpdate->setCodeUpdateProgress(false);
                    codeUpdate->clearDirPath(LID_STAGING_DIR);
                    auto sensorId = codeUpdate->getFirmwareUpdateSensor();
                    sendStateSensorEvent(sensorId, PLDM_STATE_SENSOR_STATE, 0,
                                         uint8_t(CodeUpdateState::ABORT),
                                         uint8_t(CodeUpdateState::START));
                    // sendCodeUpdateEvent(effecterId, ABORT, END);
                }
                else if (stateField[currState].effecter_state ==
                         uint8_t(CodeUpdateState::ACCEPT))
                {
                    auto sensorId = codeUpdate->getFirmwareUpdateSensor();
                    sendStateSensorEvent(sensorId, PLDM_STATE_SENSOR_STATE, 0,
                                         uint8_t(CodeUpdateState::ACCEPT),
                                         uint8_t(CodeUpdateState::END));
                    // TODO Set new Dbus property provided by code update app
                    // sendCodeUpdateEvent(effecterId, ACCEPT, END);
                }
                else if (stateField[currState].effecter_state ==
                         uint8_t(CodeUpdateState::REJECT))
                {
                    auto sensorId = codeUpdate->getFirmwareUpdateSensor();
                    sendStateSensorEvent(sensorId, PLDM_STATE_SENSOR_STATE, 0,
                                         uint8_t(CodeUpdateState::REJECT),
                                         uint8_t(CodeUpdateState::END));
                    // TODO Set new Dbus property provided by code update app
                    // sendCodeUpdateEvent(effecterId, REJECT, END);
                }
            }
            else if (entityType == PLDM_ENTITY_SYSTEM_CHASSIS &&
                     stateSetId == PLDM_OEM_IBM_SYSTEM_POWER_STATE)
            {
                if (stateField[currState].effecter_state == POWER_CYCLE_HARD)
                {
                    systemRebootEvent =
                        std::make_unique<sdeventplus::source::Defer>(
                            event,
                            std::bind(std::mem_fn(&oem_ibm_platform::Handler::
                                                      _processSystemReboot),
                                      this, std::placeholders::_1));
                }
            }
            else
            {
                rc = PLDM_PLATFORM_SET_EFFECTER_UNSUPPORTED_SENSORSTATE;
            }
        }
        if (rc != PLDM_SUCCESS)
        {
            break;
        }
    }
    return rc;
}

void buildAllCodeUpdateEffecterPDR(oem_ibm_platform::Handler* platformHandler,
                                   uint16_t entityType, uint16_t entityInstance,
                                   uint16_t stateSetID, pdr_utils::Repo& repo)
{
    size_t pdrSize = 0;
    pdrSize = sizeof(pldm_state_effecter_pdr) +
              sizeof(state_effecter_possible_states);
    std::vector<uint8_t> entry{};
    entry.resize(pdrSize);
    pldm_state_effecter_pdr* pdr =
        reinterpret_cast<pldm_state_effecter_pdr*>(entry.data());
    if (!pdr)
    {
        error("Failed to get record by PDR type, ERROR:{ERR_CODE}", "ERR_CODE",
              lg2::hex,
              static_cast<unsigned>(PLDM_PLATFORM_INVALID_EFFECTER_ID));
        return;
    }
    pdr->hdr.record_handle = 0;
    pdr->hdr.version = 1;
    pdr->hdr.type = PLDM_STATE_EFFECTER_PDR;
    pdr->hdr.record_change_num = 0;
    pdr->hdr.length = sizeof(pldm_state_effecter_pdr) - sizeof(pldm_pdr_hdr);
    pdr->terminus_handle = TERMINUS_HANDLE;
    pdr->effecter_id = platformHandler->getNextEffecterId();
    pdr->entity_type = entityType;
    pdr->entity_instance = entityInstance;
    pdr->container_id = 1;
    pdr->effecter_semantic_id = 0;
    pdr->effecter_init = PLDM_NO_INIT;
    pdr->has_description_pdr = false;
    pdr->composite_effecter_count = 1;

    auto* possibleStatesPtr = pdr->possible_states;
    auto possibleStates =
        reinterpret_cast<state_effecter_possible_states*>(possibleStatesPtr);
    possibleStates->state_set_id = stateSetID;
    possibleStates->possible_states_size = 2;
    auto state =
        reinterpret_cast<state_effecter_possible_states*>(possibleStates);
    if (stateSetID == PLDM_OEM_IBM_BOOT_STATE)
        state->states[0].byte = 6;
    else if (stateSetID == PLDM_OEM_IBM_FIRMWARE_UPDATE_STATE)
        state->states[0].byte = 126;
    else if (stateSetID == PLDM_OEM_IBM_SYSTEM_POWER_STATE)
        state->states[0].byte = 2;
    pldm::responder::pdr_utils::PdrEntry pdrEntry{};
    pdrEntry.data = entry.data();
    pdrEntry.size = pdrSize;
    repo.addRecord(pdrEntry);
}

void buildAllCodeUpdateSensorPDR(oem_ibm_platform::Handler* platformHandler,
                                 uint16_t entityType, uint16_t entityInstance,
                                 uint16_t stateSetID, pdr_utils::Repo& repo)
{
    size_t pdrSize = 0;
    pdrSize = sizeof(pldm_state_sensor_pdr) +
              sizeof(state_sensor_possible_states);
    std::vector<uint8_t> entry{};
    entry.resize(pdrSize);
    pldm_state_sensor_pdr* pdr =
        reinterpret_cast<pldm_state_sensor_pdr*>(entry.data());
    if (!pdr)
    {
        error("Failed to get record by PDR type, ERROR:{ERR_CODE}", "ERR_CODE",
              lg2::hex, static_cast<unsigned>(PLDM_PLATFORM_INVALID_SENSOR_ID));
        return;
    }
    pdr->hdr.record_handle = 0;
    pdr->hdr.version = 1;
    pdr->hdr.type = PLDM_STATE_SENSOR_PDR;
    pdr->hdr.record_change_num = 0;
    pdr->hdr.length = sizeof(pldm_state_sensor_pdr) - sizeof(pldm_pdr_hdr);
    pdr->terminus_handle = TERMINUS_HANDLE;
    pdr->sensor_id = platformHandler->getNextSensorId();
    pdr->entity_type = entityType;
    pdr->entity_instance = entityInstance;
    pdr->container_id = 1;
    pdr->sensor_init = PLDM_NO_INIT;
    pdr->sensor_auxiliary_names_pdr = false;
    pdr->composite_sensor_count = 1;

    auto* possibleStatesPtr = pdr->possible_states;
    auto possibleStates =
        reinterpret_cast<state_sensor_possible_states*>(possibleStatesPtr);
    possibleStates->state_set_id = stateSetID;
    possibleStates->possible_states_size = 2;
    auto state =
        reinterpret_cast<state_sensor_possible_states*>(possibleStates);
    if ((stateSetID == PLDM_OEM_IBM_BOOT_STATE) ||
        (stateSetID == PLDM_OEM_IBM_VERIFICATION_STATE))
        state->states[0].byte = 6;
    else if (stateSetID == PLDM_OEM_IBM_FIRMWARE_UPDATE_STATE)
        state->states[0].byte = 126;
    pldm::responder::pdr_utils::PdrEntry pdrEntry{};
    pdrEntry.data = entry.data();
    pdrEntry.size = pdrSize;
    repo.addRecord(pdrEntry);
}

void pldm::responder::oem_ibm_platform::Handler::buildOEMPDR(
    pdr_utils::Repo& repo)
{
    buildAllCodeUpdateEffecterPDR(this, PLDM_OEM_IBM_ENTITY_FIRMWARE_UPDATE,
                                  ENTITY_INSTANCE_0, PLDM_OEM_IBM_BOOT_STATE,
                                  repo);
    buildAllCodeUpdateEffecterPDR(this, PLDM_OEM_IBM_ENTITY_FIRMWARE_UPDATE,
                                  ENTITY_INSTANCE_1, PLDM_OEM_IBM_BOOT_STATE,
                                  repo);
    buildAllCodeUpdateEffecterPDR(this, PLDM_OEM_IBM_ENTITY_FIRMWARE_UPDATE,
                                  ENTITY_INSTANCE_0,
                                  PLDM_OEM_IBM_FIRMWARE_UPDATE_STATE, repo);
    buildAllCodeUpdateEffecterPDR(this, PLDM_ENTITY_SYSTEM_CHASSIS,
                                  ENTITY_INSTANCE_1,
                                  PLDM_OEM_IBM_SYSTEM_POWER_STATE, repo);

    buildAllCodeUpdateSensorPDR(this, PLDM_OEM_IBM_ENTITY_FIRMWARE_UPDATE,
                                ENTITY_INSTANCE_0, PLDM_OEM_IBM_BOOT_STATE,
                                repo);
    buildAllCodeUpdateSensorPDR(this, PLDM_OEM_IBM_ENTITY_FIRMWARE_UPDATE,
                                ENTITY_INSTANCE_1, PLDM_OEM_IBM_BOOT_STATE,
                                repo);
    buildAllCodeUpdateSensorPDR(this, PLDM_OEM_IBM_ENTITY_FIRMWARE_UPDATE,
                                ENTITY_INSTANCE_0,
                                PLDM_OEM_IBM_FIRMWARE_UPDATE_STATE, repo);
    buildAllCodeUpdateSensorPDR(this, PLDM_OEM_IBM_ENTITY_FIRMWARE_UPDATE,
                                ENTITY_INSTANCE_0,
                                PLDM_OEM_IBM_VERIFICATION_STATE, repo);
    auto sensorId = findStateSensorId(
        repo.getPdr(), 0, PLDM_OEM_IBM_ENTITY_FIRMWARE_UPDATE,
        ENTITY_INSTANCE_0, 1, PLDM_OEM_IBM_VERIFICATION_STATE);
    codeUpdate->setMarkerLidSensor(sensorId);
    sensorId = findStateSensorId(
        repo.getPdr(), 0, PLDM_OEM_IBM_ENTITY_FIRMWARE_UPDATE,
        ENTITY_INSTANCE_0, 1, PLDM_OEM_IBM_FIRMWARE_UPDATE_STATE);
    codeUpdate->setFirmwareUpdateSensor(sensorId);
}

void pldm::responder::oem_ibm_platform::Handler::setPlatformHandler(
    pldm::responder::platform::Handler* handler)
{
    platformHandler = handler;
}

int pldm::responder::oem_ibm_platform::Handler::sendEventToHost(
    std::vector<uint8_t>& requestMsg, uint8_t instanceId)
{
    if (requestMsg.size())
    {
        std::ostringstream tempStream;
        for (int byte : requestMsg)
        {
            tempStream << std::setfill('0') << std::setw(2) << std::hex << byte
                       << " ";
        }
        std::cout << tempStream.str() << std::endl;
    }
    auto oemPlatformEventMessageResponseHandler =
        [](mctp_eid_t /*eid*/, const pldm_msg* response, size_t respMsgLen) {
        uint8_t completionCode{};
        uint8_t status{};
        auto rc = decode_platform_event_message_resp(response, respMsgLen,
                                                     &completionCode, &status);
        if (rc || completionCode)
        {
            error(
                "Failed to decode_platform_event_message_resp: for code update event rc={RC}, cc={CC}",
                "RC", rc, "CC", static_cast<unsigned>(completionCode));
        }
    };
    auto rc = handler->registerRequest(
        mctp_eid, instanceId, PLDM_PLATFORM, PLDM_PLATFORM_EVENT_MESSAGE,
        std::move(requestMsg),
        std::move(oemPlatformEventMessageResponseHandler));
    if (rc)
    {
        error("Failed to send BIOS attribute change event message ");
    }

    return rc;
}

int encodeEventMsg(uint8_t eventType, const std::vector<uint8_t>& eventDataVec,
                   std::vector<uint8_t>& requestMsg, uint8_t instanceId)
{
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    auto rc = encode_platform_event_message_req(
        instanceId, 1 /*formatVersion*/, TERMINUS_ID /*tId*/, eventType,
        eventDataVec.data(), eventDataVec.size(), request,
        eventDataVec.size() + PLDM_PLATFORM_EVENT_MESSAGE_MIN_REQ_BYTES);

    return rc;
}

void pldm::responder::oem_ibm_platform::Handler::sendStateSensorEvent(
    uint16_t sensorId, enum sensor_event_class_states sensorEventClass,
    uint8_t sensorOffset, uint8_t eventState, uint8_t prevEventState)
{
    std::vector<uint8_t> sensorEventDataVec{};
    size_t sensorEventSize = PLDM_SENSOR_EVENT_DATA_MIN_LENGTH + 1;
    sensorEventDataVec.resize(sensorEventSize);
    auto eventData = reinterpret_cast<struct pldm_sensor_event_data*>(
        sensorEventDataVec.data());
    eventData->sensor_id = sensorId;
    eventData->sensor_event_class_type = sensorEventClass;
    auto eventClassStart = eventData->event_class;
    auto eventClass =
        reinterpret_cast<struct pldm_sensor_event_state_sensor_state*>(
            eventClassStart);
    eventClass->sensor_offset = sensorOffset;
    eventClass->event_state = eventState;
    eventClass->previous_event_state = prevEventState;
    auto instanceId = instanceIdDb.next(mctp_eid);
    std::vector<uint8_t> requestMsg(sizeof(pldm_msg_hdr) +
                                    PLDM_PLATFORM_EVENT_MESSAGE_MIN_REQ_BYTES +
                                    sensorEventDataVec.size());
    auto rc = encodeEventMsg(PLDM_SENSOR_EVENT, sensorEventDataVec, requestMsg,
                             instanceId);
    if (rc != PLDM_SUCCESS)
    {
        error("Failed to encode state sensor event, rc = {RC}", "RC", rc);
        instanceIdDb.free(mctp_eid, instanceId);
        return;
    }
    rc = sendEventToHost(requestMsg, instanceId);
    if (rc != PLDM_SUCCESS)
    {
        error("Failed to send event to host: rc={RC}", "RC", rc);
    }
    return;
}

void pldm::responder::oem_ibm_platform::Handler::_processEndUpdate(
    sdeventplus::source::EventBase& /*source */)
{
    assembleImageEvent.reset();
    int retc = codeUpdate->assembleCodeUpdateImage();
    if (retc != PLDM_SUCCESS)
    {
        codeUpdate->setCodeUpdateProgress(false);
        auto sensorId = codeUpdate->getFirmwareUpdateSensor();
        sendStateSensorEvent(sensorId, PLDM_STATE_SENSOR_STATE, 0,
                             uint8_t(CodeUpdateState::FAIL),
                             uint8_t(CodeUpdateState::START));
    }
}

void pldm::responder::oem_ibm_platform::Handler::_processStartUpdate(
    sdeventplus::source::EventBase& /*source */)
{
    codeUpdate->deleteImage();
    CodeUpdateState state = CodeUpdateState::START;
    auto rc = codeUpdate->setRequestedApplyTime();
    if (rc != PLDM_SUCCESS)
    {
        error("setRequestedApplyTime failed");
        state = CodeUpdateState::FAIL;
    }
    auto sensorId = codeUpdate->getFirmwareUpdateSensor();
    sendStateSensorEvent(sensorId, PLDM_STATE_SENSOR_STATE, 0, uint8_t(state),
                         uint8_t(CodeUpdateState::END));
}

void pldm::responder::oem_ibm_platform::Handler::_processSystemReboot(
    sdeventplus::source::EventBase& /*source */)
{
    pldm::utils::PropertyValue value =
        "xyz.openbmc_project.State.Chassis.Transition.Off";
    pldm::utils::DBusMapping dbusMapping{"/xyz/openbmc_project/state/chassis0",
                                         "xyz.openbmc_project.State.Chassis",
                                         "RequestedPowerTransition", "string"};
    try
    {
        dBusIntf->setDbusProperty(dbusMapping, value);
    }
    catch (const std::exception& e)
    {
        error(
            "Chassis State transition to Off failed, unable to set property RequestedPowerTransition ERROR={ERR_EXCEP}",
            "ERR_EXCEP", e.what());
    }

    using namespace sdbusplus::bus::match::rules;
    chassisOffMatch = std::make_unique<sdbusplus::bus::match_t>(
        pldm::utils::DBusHandler::getBus(),
        propertiesChanged("/xyz/openbmc_project/state/chassis0",
                          "xyz.openbmc_project.State.Chassis"),
        [this](sdbusplus::message_t& msg) {
        DbusChangedProps props{};
        std::string intf;
        msg.read(intf, props);
        const auto itr = props.find("CurrentPowerState");
        if (itr != props.end())
        {
            PropertyValue value = itr->second;
            auto propVal = std::get<std::string>(value);
            if (propVal == "xyz.openbmc_project.State.Chassis.PowerState.Off")
            {
                pldm::utils::DBusMapping dbusMapping{
                    "/xyz/openbmc_project/control/host0/"
                    "power_restore_policy/one_time",
                    "xyz.openbmc_project.Control.Power.RestorePolicy",
                    "PowerRestorePolicy", "string"};
                value = "xyz.openbmc_project.Control.Power.RestorePolicy."
                        "Policy.AlwaysOn";
                try
                {
                    dBusIntf->setDbusProperty(dbusMapping, value);
                }
                catch (const std::exception& e)
                {
                    error(
                        "Setting one-time restore policy failed, unable to set property PowerRestorePolicy ERROR={ERR_EXCEP}",
                        "ERR_EXCEP", e.what());
                }
                dbusMapping = pldm::utils::DBusMapping{
                    "/xyz/openbmc_project/state/bmc0",
                    "xyz.openbmc_project.State.BMC", "RequestedBMCTransition",
                    "string"};
                value = "xyz.openbmc_project.State.BMC.Transition.Reboot";
                try
                {
                    dBusIntf->setDbusProperty(dbusMapping, value);
                }
                catch (const std::exception& e)
                {
                    error(
                        "BMC state transition to reboot failed, unable to set property RequestedBMCTransition ERROR={ERR_EXCEP}",
                        "ERR_EXCEP", e.what());
                }
            }
        }
    });
}

void pldm::responder::oem_ibm_platform::Handler::checkAndDisableWatchDog()
{
    if (!hostOff && setEventReceiverCnt == SET_EVENT_RECEIVER_SENT)
    {
        disableWatchDogTimer();
    }

    return;
}

bool pldm::responder::oem_ibm_platform::Handler::watchDogRunning()
{
    static constexpr auto watchDogObjectPath =
        "/xyz/openbmc_project/watchdog/host0";
    static constexpr auto watchDogEnablePropName = "Enabled";
    static constexpr auto watchDogInterface =
        "xyz.openbmc_project.State.Watchdog";
    bool isWatchDogRunning = false;
    try
    {
        isWatchDogRunning = pldm::utils::DBusHandler().getDbusProperty<bool>(
            watchDogObjectPath, watchDogEnablePropName, watchDogInterface);
    }
    catch (const std::exception& e)
    {
        return false;
    }
    return isWatchDogRunning;
}

void pldm::responder::oem_ibm_platform::Handler::resetWatchDogTimer()
{
    static constexpr auto watchDogService = "xyz.openbmc_project.Watchdog";
    static constexpr auto watchDogObjectPath =
        "/xyz/openbmc_project/watchdog/host0";
    static constexpr auto watchDogInterface =
        "xyz.openbmc_project.State.Watchdog";
    static constexpr auto watchDogResetPropName = "ResetTimeRemaining";

    bool wdStatus = watchDogRunning();
    if (wdStatus == false)
    {
        return;
    }
    try
    {
        auto& bus = pldm::utils::DBusHandler::getBus();
        auto resetMethod =
            bus.new_method_call(watchDogService, watchDogObjectPath,
                                watchDogInterface, watchDogResetPropName);
        resetMethod.append(true);
        bus.call_noreply(resetMethod, dbusTimeout);
    }
    catch (const std::exception& e)
    {
        error("Failed To reset watchdog timer ERROR={ERR_EXCEP}", "ERR_EXCEP",
              e.what());
        return;
    }
}

void pldm::responder::oem_ibm_platform::Handler::disableWatchDogTimer()
{
    setEventReceiverCnt = 0;
    pldm::utils::DBusMapping dbusMapping{"/xyz/openbmc_project/watchdog/host0",
                                         "xyz.openbmc_project.State.Watchdog",
                                         "Enabled", "bool"};
    bool wdStatus = watchDogRunning();

    if (!wdStatus)
    {
        return;
    }
    try
    {
        pldm::utils::DBusHandler().setDbusProperty(dbusMapping, false);
    }
    catch (const std::exception& e)
    {
        error("Failed To disable watchdog timer ERROR={ERR_EXCEP}", "ERR_EXCEP",
              e.what());
    }
}
int pldm::responder::oem_ibm_platform::Handler::checkBMCState()
{
    try
    {
        pldm::utils::PropertyValue propertyValue =
            pldm::utils::DBusHandler().getDbusPropertyVariant(
                "/xyz/openbmc_project/state/bmc0", "CurrentBMCState",
                "xyz.openbmc_project.State.BMC");

        if (std::get<std::string>(propertyValue) ==
            "xyz.openbmc_project.State.BMC.BMCState.NotReady")
        {
            error("GetPDR : PLDM stack is not ready for PDR exchange");
            return PLDM_ERROR_NOT_READY;
        }
    }
    catch (const std::exception& e)
    {
        error("Error getting the current BMC state");
        return PLDM_ERROR;
    }
    return PLDM_SUCCESS;
}

const pldm_pdr_record*
    pldm::responder::oem_ibm_platform::Handler::fetchLastBMCRecord(
        const pldm_pdr* repo)
{
    pldm_pdr_record* record{};
    return pldm_pdr_find_last_in_range(repo, BMC_PDR_START_RANGE,
                                       BMC_PDR_END_RANGE);
}

bool pldm::responder::oem_ibm_platform::Handler::checkRecordHandleInRange(
    uint32_t record_handle)
{
    return record_handle >= HOST_PDR_START_RANGE &&
           record_handle <= HOST_PDR_END_RANGE;
}

} // namespace oem_ibm_platform
} // namespace responder
} // namespace pldm
