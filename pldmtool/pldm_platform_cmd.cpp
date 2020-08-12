#include "libpldm/entity.h"
#include "libpldm/state_set.h"

#include "common/types.hpp"
#include "pldm_cmd_helper.hpp"

namespace pldmtool
{

namespace platform
{

namespace
{

using namespace pldmtool::helper;
std::vector<std::unique_ptr<CommandInterface>> commands;

} // namespace

class GetPDR : public CommandInterface
{
  public:
    ~GetPDR() = default;
    GetPDR() = delete;
    GetPDR(const GetPDR&) = delete;
    GetPDR(GetPDR&&) = default;
    GetPDR& operator=(const GetPDR&) = delete;
    GetPDR& operator=(GetPDR&&) = default;

    using CommandInterface::CommandInterface;

    explicit GetPDR(const char* type, const char* name, CLI::App* app) :
        CommandInterface(type, name, app)
    {
        app->add_option(
               "-d,--data", recordHandle,
               "retrieve individual PDRs from a PDR Repository\n"
               "eg: The recordHandle value for the PDR to be retrieved and 0 "
               "means get first PDR in the repository.")
            ->required();
    }

    std::pair<int, std::vector<uint8_t>> createRequestMsg() override
    {
        std::vector<uint8_t> requestMsg(sizeof(pldm_msg_hdr) +
                                        PLDM_GET_PDR_REQ_BYTES);
        auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

        auto rc =
            encode_get_pdr_req(instanceId, recordHandle, 0, PLDM_GET_FIRSTPART,
                               UINT16_MAX, 0, request, PLDM_GET_PDR_REQ_BYTES);
        return {rc, requestMsg};
    }

    void parseResponseMsg(pldm_msg* responsePtr, size_t payloadLength) override
    {
        uint8_t completionCode = 0;
        uint8_t recordData[UINT16_MAX] = {0};
        uint32_t nextRecordHndl = 0;
        uint32_t nextDataTransferHndl = 0;
        uint8_t transferFlag = 0;
        uint16_t respCnt = 0;
        uint8_t transferCRC = 0;

        auto rc = decode_get_pdr_resp(
            responsePtr, payloadLength, &completionCode, &nextRecordHndl,
            &nextDataTransferHndl, &transferFlag, &respCnt, recordData,
            sizeof(recordData), &transferCRC);

        if (rc != PLDM_SUCCESS || completionCode != PLDM_SUCCESS)
        {
            std::cerr << "Response Message Error: "
                      << "rc=" << rc << ",cc=" << (int)completionCode
                      << std::endl;
            return;
        }

        printPDRMsg(nextRecordHndl, respCnt, recordData);
    }

  private:
    const std::map<pldm::pdr::EntityType, std::string> entityType = {
        {PLDM_ENTITY_COMM_CHANNEL, "Communication Channel"},
        {PLDM_ENTITY_SYS_FIRMWARE, "System Firmware"},
        {PLDM_ENTITY_VIRTUAL_MACHINE_MANAGER, "Virtual Machine Manager"},
        {PLDM_ENTITY_SYSTEM_CHASSIS, "System chassis (main enclosure)"},
        {PLDM_ENTITY_SYS_BOARD, "System Board"},
        {PLDM_ENTITY_MEMORY_MODULE, "Memory Module"},
        {PLDM_ENTITY_PROC_MODULE, "Processor Module"},
        {PLDM_ENTITY_CHASSIS_FRONT_PANEL_BOARD,
         "Chassis front panel board (control panel)"},
        {PLDM_ENTITY_POWER_CONVERTER, "Power converter"},
        {PLDM_ENTITY_PROC, "Processor"},
        {PLDM_ENTITY_MGMT_CONTROLLER, "Management Controller"},
        {PLDM_ENTITY_CONNECTOR, "Connector"},
        {PLDM_ENTITY_POWER_SUPPLY, "Power Supply"},
        {11521, "System (logical)"},
    };

    const std::map<uint16_t, std::string> stateSet = {
        {PLDM_STATE_SET_HEALTH_STATE, "Health State"},
        {PLDM_STATE_SET_AVAILABILITY, "Availability"},
        {PLDM_STATE_SET_OPERATIONAL_STATUS, "Operational Status"},
        {PLDM_STATE_SET_OPERATIONAL_RUNNING_STATUS,
         "Operational Running Status"},
        {PLDM_STATE_SET_PRESENCE, "Presence"},
        {PLDM_STATE_SET_CONFIGURATION_STATE, "Configuration State"},
        {PLDM_STATE_SET_LINK_STATE, "Link State"},
        {PLDM_STATE_SET_SW_TERMINATION_STATUS, "Software Termination Status"},
        {PLDM_STATE_SET_BOOT_RESTART_CAUSE, "Boot/Restart Cause"},
        {PLDM_STATE_SET_BOOT_PROGRESS, "Boot Progress"},
        {PLDM_STATE_SET_SYSTEM_POWER_STATE, "System Power State"},
    };

    const std::array<std::string_view, 4> sensorInit = {
        "noInit", "useInitPDR", "enableSensor", "disableSensor"};

    const std::array<std::string_view, 4> effecterInit = {
        "noInit", "useInitPDR", "enableEffecter", "disableEffecter"};

    const std::map<uint8_t, std::string> pdrType = {
        {PLDM_TERMINUS_LOCATOR_PDR, "Terminus Locator PDR"},
        {PLDM_NUMERIC_SENSOR_PDR, "Numeric Sensor PDR"},
        {PLDM_NUMERIC_SENSOR_INITIALIZATION_PDR,
         "Numeric Sensor Initialization PDR"},
        {PLDM_STATE_SENSOR_PDR, "State Sensor PDR"},
        {PLDM_STATE_SENSOR_INITIALIZATION_PDR,
         "State Sensor Initialization PDR"},
        {PLDM_SENSOR_AUXILIARY_NAMES_PDR, "Sensor Auxiliary Names PDR"},
        {PLDM_OEM_UNIT_PDR, "OEM Unit PDR"},
        {PLDM_OEM_STATE_SET_PDR, "OEM State Set PDR"},
        {PLDM_NUMERIC_EFFECTER_PDR, "Numeric Effecter PDR"},
        {PLDM_NUMERIC_EFFECTER_INITIALIZATION_PDR,
         "Numeric Effecter Initialization PDR"},
        {PLDM_STATE_EFFECTER_PDR, "State Effecter PDR"},
        {PLDM_STATE_EFFECTER_INITIALIZATION_PDR,
         "State Effecter Initialization PDR"},
        {PLDM_EFFECTER_AUXILIARY_NAMES_PDR, "Effecter Auxiliary Names PDR"},
        {PLDM_EFFECTER_OEM_SEMANTIC_PDR, "Effecter OEM Semantic PDR"},
        {PLDM_PDR_ENTITY_ASSOCIATION, "Entity Association PDR"},
        {PLDM_ENTITY_AUXILIARY_NAMES_PDR, "Entity Auxiliary Names PDR"},
        {PLDM_OEM_ENTITY_ID_PDR, "OEM Entity ID PDR"},
        {PLDM_INTERRUPT_ASSOCIATION_PDR, "Interrupt Association PDR"},
        {PLDM_EVENT_LOG_PDR, "PLDM Event Log PDR"},
        {PLDM_PDR_FRU_RECORD_SET, "FRU Record Set PDR"},
        {PLDM_OEM_DEVICE_PDR, "OEM Device PDR"},
        {PLDM_OEM_PDR, "OEM PDR"},
    };

    std::string getEntityName(pldm::pdr::EntityType type)
    {
        try
        {
            return entityType.at(type);
        }
        catch (const std::out_of_range& e)
        {
            return std::to_string(static_cast<unsigned>(type)) + "(OEM)";
        }
    }

    std::string getStateSetName(uint16_t id)
    {
        auto typeString = std::to_string(id);
        try
        {
            return stateSet.at(id) + "(" + typeString + ")";
        }
        catch (const std::out_of_range& e)
        {
            return typeString;
        }
    }

    std::string getPDRType(uint8_t type)
    {
        auto typeString = std::to_string(type);
        try
        {
            return pdrType.at(type) + "(" + typeString + ")";
        }
        catch (const std::out_of_range& e)
        {
            return typeString;
        }
    }

    json printCommonPDRHeader(const pldm_pdr_hdr* hdr)
    {
        json header;
        header["recordHandle"] = hdr->record_handle;
        header["PDRHeaderVersion"] = unsigned(hdr->version);
        header["PDRType"] = getPDRType(hdr->type);
        header["recordChangeNumber"] = hdr->record_change_num;
        header["dataLength"] = hdr->length;

        return header;
    }

    std::string printPossibleStates(uint8_t possibleStatesSize,
                                    const bitfield8_t* states)
    {
        uint8_t possibleStatesPos{};
        std::string data;
        auto printStates = [&possibleStatesPos, &data](const bitfield8_t& val) {
            std::stringstream pstates;
            for (int i = 0; i < CHAR_BIT; i++)
            {
                if (val.byte & (1 << i))
                {
                    pstates << " " << (possibleStatesPos * CHAR_BIT + i);
                    data.append(pstates.str());
                }
            }
            possibleStatesPos++;
        };
        std::for_each(states, states + possibleStatesSize, printStates);
        return data;
    }

    json printStateSensorPDR(const uint8_t* data)
    {
        auto pdr = reinterpret_cast<const pldm_state_sensor_pdr*>(data);
        json ssdata;
        ssdata["PLDMTerminusHandle"] = pdr->terminus_handle;
        ssdata["sensorID"] = pdr->sensor_id;
        ssdata["entityType"] = getEntityName(pdr->entity_type);
        ssdata["entityInstanceNumber"] = pdr->entity_instance;
        ssdata["containerID"] = pdr->container_id;
        ssdata["sensorInit"] = sensorInit[pdr->sensor_init];
        ssdata["sensorAuxiliaryNamesPDR"] =
            (pdr->sensor_auxiliary_names_pdr ? true : false);
        ssdata["compositeSensorCount"] = unsigned(pdr->composite_sensor_count);

        auto statesPtr = pdr->possible_states;
        auto compositeSensorCount = pdr->composite_sensor_count;

        while (compositeSensorCount--)
        {
            auto state = reinterpret_cast<const state_sensor_possible_states*>(
                statesPtr);
            ssdata["stateSetID"] = getStateSetName(state->state_set_id);
            ssdata["possibleStatesSize"] = getStateSetName(state->state_set_id);
            ssdata["possibleStates"] =
                printPossibleStates(state->possible_states_size, state->states);

            if (compositeSensorCount)
            {
                statesPtr += sizeof(state_sensor_possible_states) +
                             state->possible_states_size - 1;
            }
        }
        return ssdata;
    }

    json printPDRFruRecordSet(uint8_t* data)
    {
        if (data == NULL)
        {
            return {};
        }

        data += sizeof(pldm_pdr_hdr);
        pldm_pdr_fru_record_set* pdr =
            reinterpret_cast<pldm_pdr_fru_record_set*>(data);

        json frudata;

        frudata["PLDMTerminusHandle"] = unsigned(pdr->terminus_handle);
        frudata["FRURecordSetIdentifier"] = unsigned(pdr->fru_rsi);
        frudata["entityType"] = getEntityName(pdr->entity_type);
        frudata["entityInstanceNumber"] = unsigned(pdr->entity_instance_num);
        frudata["containerID"] = unsigned(pdr->container_id);

        return frudata;
    }

    json printPDREntityAssociation(uint8_t* data)
    {
        const std::map<uint8_t, const char*> assocationType = {
            {PLDM_ENTITY_ASSOCIAION_PHYSICAL, "Physical"},
            {PLDM_ENTITY_ASSOCIAION_LOGICAL, "Logical"},
        };

        if (data == NULL)
        {
            return {};
        }

        data += sizeof(pldm_pdr_hdr);
        pldm_pdr_entity_association* pdr =
            reinterpret_cast<pldm_pdr_entity_association*>(data);

        json peadata = json::object({});
        peadata["containerID"] = int(pdr->container_id);
        peadata["associationType"] = assocationType.at(pdr->association_type);
        peadata["containerEntityType"] =
            getEntityName(pdr->container.entity_type);
        peadata["containerEntityInstanceNumber"] =
            int(pdr->container.entity_instance_num);
        peadata["containerEntityContainerID"] =
            int(pdr->container.entity_container_id);
        peadata["containedEntityCount"] =
            static_cast<unsigned>(pdr->num_children);

        auto child = reinterpret_cast<pldm_entity*>(&pdr->children[0]);
        for (int i = 0; i < pdr->num_children; ++i)
        {
            peadata.emplace("containedEntityType[" + std::to_string(i + 1) +
                                "]",
                            getEntityName(child->entity_type));
            peadata.emplace("containedEntityInstanceNumber[" +
                                std::to_string(i + 1) + "]",
                            unsigned(child->entity_instance_num));
            peadata.emplace("containedEntityContainerID[" +
                                std::to_string(i + 1) + "]",
                            unsigned(child->entity_container_id));

            ++child;
        }
        return peadata;
    }

    json printNumericEffecterPDR(uint8_t* data)
    {
        struct pldm_numeric_effecter_value_pdr* pdr =
            (struct pldm_numeric_effecter_value_pdr*)data;

        json nedata;
        nedata["PLDMTerminusHandle"] = int(pdr->terminus_handle);
        nedata["effecterID"] = int(pdr->effecter_id);
        nedata["entityType"] = int(pdr->entity_type);
        nedata["entityInstanceNumber"] = int(pdr->entity_instance);
        nedata["containerID"] = int(pdr->container_id);
        nedata["effecterSemanticID"] = int(pdr->effecter_semantic_id);
        nedata["effecterInit"] = unsigned(pdr->effecter_init);
        nedata["effecterAuxiliaryNames"] =
            (unsigned(pdr->effecter_auxiliary_names) ? true : false);
        nedata["baseUnit"] = unsigned(pdr->base_unit);
        nedata["unitModifier"] = unsigned(pdr->unit_modifier);
        nedata["rateUnit"] = unsigned(pdr->rate_unit);
        nedata["baseOEMUnitHandle"] = unsigned(pdr->base_oem_unit_handle);
        nedata["auxUnit"] = unsigned(pdr->aux_unit);
        nedata["auxUnitModifier"] = unsigned(pdr->aux_unit_modifier);
        nedata["auxrateUnit"] = unsigned(pdr->aux_rate_unit);
        nedata["auxOEMUnitHandle"] = unsigned(pdr->aux_oem_unit_handle);
        nedata["isLinear"] = (unsigned(pdr->is_linear) ? true : false);
        nedata["effecterDataSize"] = unsigned(pdr->effecter_data_size);
        nedata["resolution"] = unsigned(pdr->resolution);
        nedata["offset"] = unsigned(pdr->offset);
        nedata["accuracy"] = unsigned(pdr->accuracy);
        nedata["plusTolerance"] = unsigned(pdr->plus_tolerance);
        nedata["minusTolerance"] = unsigned(pdr->minus_tolerance);
        nedata["stateTransitionInterval"] =
            unsigned(pdr->state_transition_interval);
        nedata["TransitionInterval"] = unsigned(pdr->transition_interval);

        switch (pdr->effecter_data_size)
        {
            case PLDM_EFFECTER_DATA_SIZE_UINT8:
                nedata["maxSettable"] = unsigned(pdr->max_set_table.value_u8);
                nedata["minSettable"] = unsigned(pdr->min_set_table.value_u8);
                break;
            case PLDM_EFFECTER_DATA_SIZE_SINT8:
                nedata["maxSettable"] = unsigned(pdr->max_set_table.value_s8);
                nedata["minSettable"] = unsigned(pdr->min_set_table.value_s8);
                break;
            case PLDM_EFFECTER_DATA_SIZE_UINT16:
                nedata["maxSettable"] = unsigned(pdr->max_set_table.value_u16);
                nedata["minSettable"] = unsigned(pdr->min_set_table.value_u16);
                break;
            case PLDM_EFFECTER_DATA_SIZE_SINT16:
                nedata["maxSettable"] = unsigned(pdr->max_set_table.value_s16);
                nedata["minSettable"] = unsigned(pdr->min_set_table.value_s16);
                break;
            case PLDM_EFFECTER_DATA_SIZE_UINT32:
                nedata["maxSettable"] = unsigned(pdr->max_set_table.value_u32);
                nedata["minSettable"] = unsigned(pdr->min_set_table.value_u32);
                break;
            case PLDM_EFFECTER_DATA_SIZE_SINT32:
                nedata["maxSettable"] = unsigned(pdr->max_set_table.value_s32);
                nedata["minSettable"] = unsigned(pdr->min_set_table.value_s32);
                break;
            default:
                break;
        }

        nedata["rangeFieldFormat"] = unsigned(pdr->range_field_format);
        nedata["rangeFieldSupport"] = unsigned(pdr->range_field_support.byte);

        switch (pdr->range_field_format)
        {
            case PLDM_RANGE_FIELD_FORMAT_UINT8:
                nedata["nominalValue"] = unsigned(pdr->nominal_value.value_u8);
                nedata["normalMax"] = unsigned(pdr->normal_max.value_u8);
                nedata["normalMin"] = unsigned(pdr->normal_min.value_u8);
                nedata["ratedMax"] = unsigned(pdr->rated_max.value_u8);
                nedata["ratedMin"] = unsigned(pdr->rated_min.value_u8);
                break;
            case PLDM_RANGE_FIELD_FORMAT_SINT8:
                nedata["nominalValue"] = unsigned(pdr->nominal_value.value_s8);
                nedata["normalMax"] = unsigned(pdr->normal_max.value_s8);
                nedata["normalMin"] = unsigned(pdr->normal_min.value_s8);
                nedata["ratedMax"] = unsigned(pdr->rated_max.value_s8);
                nedata["ratedMin"] = unsigned(pdr->rated_min.value_s8);
                break;
            case PLDM_RANGE_FIELD_FORMAT_UINT16:
                nedata["nominalValue"] = unsigned(pdr->nominal_value.value_u16);
                nedata["normalMax"] = unsigned(pdr->normal_max.value_u16);
                nedata["normalMin"] = unsigned(pdr->normal_min.value_u16);
                nedata["ratedMax"] = unsigned(pdr->rated_max.value_u16);
                nedata["ratedMin"] = unsigned(pdr->rated_min.value_u16);
                break;
            case PLDM_RANGE_FIELD_FORMAT_SINT16:
                nedata["nominalValue"] = unsigned(pdr->nominal_value.value_s16);
                nedata["normalMax"] = unsigned(pdr->normal_max.value_s16);
                nedata["normalMin"] = unsigned(pdr->normal_min.value_s16);
                nedata["ratedMax"] = unsigned(pdr->rated_max.value_s16);
                nedata["ratedMin"] = unsigned(pdr->rated_min.value_s16);
                break;
            case PLDM_RANGE_FIELD_FORMAT_UINT32:
                nedata["nominalValue"] = unsigned(pdr->nominal_value.value_u32);
                nedata["normalMax"] = unsigned(pdr->normal_max.value_u32);
                nedata["normalMin"] = unsigned(pdr->normal_min.value_u32);
                nedata["ratedMax"] = unsigned(pdr->rated_max.value_u32);
                nedata["ratedMin"] = unsigned(pdr->rated_min.value_u32);
                break;
            case PLDM_RANGE_FIELD_FORMAT_SINT32:
                nedata["nominalValue"] = unsigned(pdr->nominal_value.value_s32);
                nedata["normalMax"] = unsigned(pdr->normal_max.value_s32);
                nedata["normalMin"] = unsigned(pdr->normal_min.value_s32);
                nedata["ratedMax"] = unsigned(pdr->rated_max.value_s32);
                nedata["ratedMin"] = unsigned(pdr->rated_min.value_s32);
                break;
            case PLDM_RANGE_FIELD_FORMAT_REAL32:
                nedata["nominalValue"] = unsigned(pdr->nominal_value.value_f32);
                nedata["normalMax"] = unsigned(pdr->normal_max.value_f32);
                nedata["normalMin"] = unsigned(pdr->normal_min.value_f32);
                nedata["ratedMax"] = unsigned(pdr->rated_max.value_f32);
                nedata["ratedMin"] = unsigned(pdr->rated_min.value_f32);
                break;
            default:
                break;
        }
        return nedata;
    }

    json printStateEffecterPDR(const uint8_t* data)
    {
        auto pdr = reinterpret_cast<const pldm_state_effecter_pdr*>(data);
        json sedata;

        sedata["PLDMTerminusHandle"] = pdr->terminus_handle;
        sedata["effecterID"] = pdr->effecter_id;
        sedata["entityType"] = getEntityName(pdr->entity_type);
        sedata["entityInstanceNumber"] = pdr->entity_instance;
        sedata["containerID"] = pdr->container_id;
        sedata["effecterSemanticID"] = pdr->effecter_semantic_id;
        sedata["effecterInit"] = effecterInit[pdr->effecter_init];
        sedata["effecterDescriptionPDR"] =
            (pdr->has_description_pdr ? true : false);
        sedata["compositeEffecterCount"] =
            unsigned(pdr->composite_effecter_count);

        auto statesPtr = pdr->possible_states;
        auto compositeEffecterCount = pdr->composite_effecter_count;

        while (compositeEffecterCount--)
        {
            auto state =
                reinterpret_cast<const state_effecter_possible_states*>(
                    statesPtr);

            sedata["stateSetID"] = getStateSetName(state->state_set_id);
            sedata["possibleStatesSize"] = (state->possible_states_size);
            sedata["possibleStates"] =
                printPossibleStates(state->possible_states_size, state->states);

            if (compositeEffecterCount)
            {
                statesPtr += sizeof(state_effecter_possible_states) +
                             state->possible_states_size - 1;
            }
        }
        return sedata;
    }

    json printTerminusLocatorPDR(const uint8_t* data)
    {
        const std::array<std::string_view, 4> terminusLocatorType = {
            "UID", "MCTP_EID", "SMBusRelative", "systemSoftware"};

        auto pdr = reinterpret_cast<const pldm_terminus_locator_pdr*>(data);
        json tdata;

        tdata["PLDMTerminusHandle"] = pdr->terminus_handle;
        tdata["validity"] = (pdr->validity ? "valid" : "notValid");
        tdata["TID"] = unsigned(pdr->tid);
        tdata["containerID"] = pdr->container_id;
        tdata["terminusLocatorType"] =
            terminusLocatorType[pdr->terminus_locator_type];
        tdata["terminusLocatorValueSize"] =
            unsigned(pdr->terminus_locator_value_size);

        if (pdr->terminus_locator_type == PLDM_TERMINUS_LOCATOR_TYPE_MCTP_EID)
        {
            auto locatorValue =
                reinterpret_cast<const pldm_terminus_locator_type_mctp_eid*>(
                    pdr->terminus_locator_value);
            tdata["EID"] = unsigned(locatorValue->eid);
        }
        return tdata;
    }

    void printPDRMsg(const uint32_t nextRecordHndl, const uint16_t respCnt,
                     uint8_t* data)
    {
        if (data == NULL)
        {
            return;
        }

        json output = json::object({});
        output.emplace("nextRecordHandle", nextRecordHndl);
        output.emplace("responseCount", respCnt);

        struct pldm_pdr_hdr* pdr = (struct pldm_pdr_hdr*)data;
        json header = printCommonPDRHeader(pdr);
        output.insert(header.begin(), header.end());

        json pdrtypeinfo;
        switch (pdr->type)
        {
            case PLDM_TERMINUS_LOCATOR_PDR:
                pdrtypeinfo = printTerminusLocatorPDR(data);
                break;
            case PLDM_STATE_SENSOR_PDR:
                pdrtypeinfo = printStateSensorPDR(data);
                break;
            case PLDM_NUMERIC_EFFECTER_PDR:
                pdrtypeinfo = printNumericEffecterPDR(data);
                break;
            case PLDM_STATE_EFFECTER_PDR:
                pdrtypeinfo = printStateEffecterPDR(data);
                break;
            case PLDM_PDR_ENTITY_ASSOCIATION:
                pdrtypeinfo = printPDREntityAssociation(data);
                break;
            case PLDM_PDR_FRU_RECORD_SET:
                pdrtypeinfo = printPDRFruRecordSet(data);
                break;
            default:
                break;
        }
        output.insert(pdrtypeinfo.begin(), pdrtypeinfo.end());
        pldmtool::helper::DisplayInJson(output);
    }

  private:
    uint32_t recordHandle;
};

class SetStateEffecter : public CommandInterface
{
  public:
    ~SetStateEffecter() = default;
    SetStateEffecter() = delete;
    SetStateEffecter(const SetStateEffecter&) = delete;
    SetStateEffecter(SetStateEffecter&&) = default;
    SetStateEffecter& operator=(const SetStateEffecter&) = delete;
    SetStateEffecter& operator=(SetStateEffecter&&) = default;

    // compositeEffecterCount(value: 0x01 to 0x08) * stateField(2)
    static constexpr auto maxEffecterDataSize = 16;

    // compositeEffecterCount(value: 0x01 to 0x08)
    static constexpr auto minEffecterCount = 1;
    static constexpr auto maxEffecterCount = 8;
    explicit SetStateEffecter(const char* type, const char* name,
                              CLI::App* app) :
        CommandInterface(type, name, app)
    {
        app->add_option(
               "-i, --id", effecterId,
               "A handle that is used to identify and access the effecter")
            ->required();
        app->add_option("-c, --count", effecterCount,
                        "The number of individual sets of effecter information")
            ->required();
        app->add_option(
               "-d,--data", effecterData,
               "Set effecter state data\n"
               "eg: requestSet0 effecterState0 noChange1 dummyState1 ...")
            ->required();
    }

    std::pair<int, std::vector<uint8_t>> createRequestMsg() override
    {
        std::vector<uint8_t> requestMsg(
            sizeof(pldm_msg_hdr) + PLDM_SET_STATE_EFFECTER_STATES_REQ_BYTES);
        auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

        if (effecterCount > maxEffecterCount ||
            effecterCount < minEffecterCount)
        {
            std::cerr << "Request Message Error: effecterCount size "
                      << effecterCount << "is invalid\n";
            auto rc = PLDM_ERROR_INVALID_DATA;
            return {rc, requestMsg};
        }

        if (effecterData.size() > maxEffecterDataSize)
        {
            std::cerr << "Request Message Error: effecterData size "
                      << effecterData.size() << "is invalid\n";
            auto rc = PLDM_ERROR_INVALID_DATA;
            return {rc, requestMsg};
        }

        auto stateField = parseEffecterData(effecterData, effecterCount);
        if (!stateField)
        {
            std::cerr << "Failed to parse effecter data, effecterCount size "
                      << effecterCount << "\n";
            auto rc = PLDM_ERROR_INVALID_DATA;
            return {rc, requestMsg};
        }

        auto rc = encode_set_state_effecter_states_req(
            instanceId, effecterId, effecterCount, stateField->data(), request);
        return {rc, requestMsg};
    }

    void parseResponseMsg(pldm_msg* responsePtr, size_t payloadLength) override
    {
        uint8_t completionCode = 0;
        auto rc = decode_set_state_effecter_states_resp(
            responsePtr, payloadLength, &completionCode);

        if (rc != PLDM_SUCCESS || completionCode != PLDM_SUCCESS)
        {
            std::cerr << "Response Message Error: "
                      << "rc=" << rc << ",cc=" << (int)completionCode << "\n";
            return;
        }

        json data;
        data = {{"status", "SUCCESS"}};
        pldmtool::helper::DisplayInJson(data);
    }

  private:
    uint16_t effecterId;
    uint8_t effecterCount;
    std::vector<uint8_t> effecterData;
};

class SetNumericEffecterValue : public CommandInterface
{
  public:
    ~SetNumericEffecterValue() = default;
    SetNumericEffecterValue() = delete;
    SetNumericEffecterValue(const SetNumericEffecterValue&) = delete;
    SetNumericEffecterValue(SetNumericEffecterValue&&) = default;
    SetNumericEffecterValue& operator=(const SetNumericEffecterValue&) = delete;
    SetNumericEffecterValue& operator=(SetNumericEffecterValue&&) = default;

    explicit SetNumericEffecterValue(const char* type, const char* name,
                                     CLI::App* app) :
        CommandInterface(type, name, app)
    {
        app->add_option(
               "-i, --id", effecterId,
               "A handle that is used to identify and access the effecter")
            ->required();
        app->add_option("-s, --size", effecterDataSize,
                        "The bit width and format of the setting value for the "
                        "effecter. enum value: {uint8, sint8, uint16, sint16, "
                        "uint32, sint32}\n")
            ->required();
        app->add_option("-d,--data", maxEffecterValue,
                        "The setting value of numeric effecter being "
                        "requested\n")
            ->required();
    }

    std::pair<int, std::vector<uint8_t>> createRequestMsg() override
    {
        std::vector<uint8_t> requestMsg(
            sizeof(pldm_msg_hdr) +
            PLDM_SET_NUMERIC_EFFECTER_VALUE_MIN_REQ_BYTES + 3);

        uint8_t* effecterValue = (uint8_t*)&maxEffecterValue;

        auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
        size_t payload_length = PLDM_SET_NUMERIC_EFFECTER_VALUE_MIN_REQ_BYTES;

        if (effecterDataSize == PLDM_EFFECTER_DATA_SIZE_UINT16 ||
            effecterDataSize == PLDM_EFFECTER_DATA_SIZE_SINT16)
        {
            payload_length = PLDM_SET_NUMERIC_EFFECTER_VALUE_MIN_REQ_BYTES + 1;
        }
        if (effecterDataSize == PLDM_EFFECTER_DATA_SIZE_UINT32 ||
            effecterDataSize == PLDM_EFFECTER_DATA_SIZE_SINT32)
        {
            payload_length = PLDM_SET_NUMERIC_EFFECTER_VALUE_MIN_REQ_BYTES + 3;
        }
        auto rc = encode_set_numeric_effecter_value_req(
            0, effecterId, effecterDataSize, effecterValue, request,
            payload_length);

        return {rc, requestMsg};
    }

    void parseResponseMsg(pldm_msg* responsePtr, size_t payloadLength) override
    {
        uint8_t completionCode = 0;
        auto rc = decode_set_numeric_effecter_value_resp(
            responsePtr, payloadLength, &completionCode);

        if (rc != PLDM_SUCCESS || completionCode != PLDM_SUCCESS)
        {
            std::cerr << "Response Message Error: "
                      << "rc=" << rc << ",cc=" << (int)completionCode
                      << std::endl;
            return;
        }

        json data;
        data = {{"status", "SUCCESS"}};
        pldmtool::helper::DisplayInJson(data);
    }

  private:
    uint16_t effecterId;
    uint8_t effecterDataSize;
    uint64_t maxEffecterValue;
};

void registerCommand(CLI::App& app)
{
    auto platform = app.add_subcommand("platform", "platform type command");
    platform->require_subcommand(1);

    auto getPDR =
        platform->add_subcommand("GetPDR", "get platform descriptor records");
    commands.push_back(std::make_unique<GetPDR>("platform", "getPDR", getPDR));

    auto setStateEffecterStates = platform->add_subcommand(
        "SetStateEffecterStates", "set effecter states");
    commands.push_back(std::make_unique<SetStateEffecter>(
        "platform", "setStateEffecterStates", setStateEffecterStates));

    auto setNumericEffecterValue = platform->add_subcommand(
        "SetNumericEffecterValue", "set the value for a PLDM Numeric Effecter");
    commands.push_back(std::make_unique<SetNumericEffecterValue>(
        "platform", "setNumericEffecterValue", setNumericEffecterValue));
}

} // namespace platform
} // namespace pldmtool
