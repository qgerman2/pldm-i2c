#include "config.h"

#include "ampere_event_manager.hpp"

#include "libpldm/pldm.h"

#include "common/utils.hpp"
#include "cper.hpp"

#include <assert.h>
#include <systemd/sd-journal.h>

#include <nlohmann/json.hpp>
#include <sdeventplus/clock.hpp>
#include <sdeventplus/exception.hpp>
#include <sdeventplus/source/io.hpp>
#include <sdeventplus/source/time.hpp>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <thread>

#define OEM_EVENT 0xFA
#define SENSOR_TYPE_OEM 0xF0

using namespace pldm::utils;

namespace pldm
{

namespace platform_mc
{
static std::unique_ptr<sdbusplus::bus::match_t> pldmNumericSensorEventSignal;

OemEventManager::OemEventManager(
    sdeventplus::Event& event, TerminusManager& terminusManager,
    std::map<tid_t, std::shared_ptr<Terminus>>& termini) :
    EventManager(event, terminusManager, termini)
{
    if (!std::filesystem::is_directory(CPER_LOG_PATH))
        std::filesystem::create_directories(CPER_LOG_PATH);
    if (!std::filesystem::is_directory(CRASHDUMP_LOG_PATH))
        std::filesystem::create_directories(CRASHDUMP_LOG_PATH);

    // register event class handler
    registerEventHandler(PLDM_MESSAGE_POLL_EVENT,
                         [&](uint8_t TID, uint8_t eventClass, uint16_t eventID,
                             const std::vector<uint8_t> data) {
                             return pldmPollForEventMessage(TID, eventClass,
                                                            eventID, data);
                         });
    // TODO: update OEM class handler
    registerEventHandler(OEM_EVENT, [&](uint8_t TID, uint8_t eventClass,
                                        uint16_t eventID,
                                        const std::vector<uint8_t> data) {
        return pldmPollForEventMessage(TID, eventClass, eventID, data);
    });

    handleNumericSensorEventSignal();
}

static void addSELLog(uint8_t TID, uint16_t eventID, AmpereSpecData* p)
{
    std::vector<uint8_t> evtData;
    std::string message = "PLDM RAS SEL Event";
    uint8_t recordType;
    uint8_t evtData1, evtData2, evtData3, evtData4, evtData5, evtData6;
    uint8_t socket;

    /*
     * OEM IPMI SEL Recode Format for RAS event:
     * evtData1:
     *    Bit [7:4]: eventClass
     *        0xF: oemEvent for RAS
     *    Bit [3:1]: Reserved
     *    Bit 0: SocketID
     *        0x0: Socket 0 0x1: Socket 1
     * evtData2:
     *    Event ID, indicates RAS PLDM sensor ID.
     * evtData3:
     *     Bit [7:4]: Payload Type
     *     Bit [3:0]: Error Type ID - Bit [11:8]
     * evtData4:
     *     Error Type ID - Bit [7:0]
     * evtData5:
     *     Error Sub Type ID high byte
     * evtData6:
     *     Error Sub Type ID low byte
     */
    socket = (TID == 1) ? 0 : 1;
    recordType = 0xD0;
    evtData1 = SENSOR_TYPE_OEM | socket;
    evtData2 = eventID;
    evtData3 = ((p->typeId.member.payloadType << 4) & 0xF0) |
               ((p->typeId.member.ipType >> 8) & 0xF);
    evtData4 = p->typeId.member.ipType;
    evtData5 = p->subTypeId >> 8;
    evtData6 = p->subTypeId;
    /*
     * OEM data bytes
     *    Ampere IANA: 3 bytes [0x3a 0xcd 0x00]
     *    event data: 9 bytes [evtData1 evtData2 evtData3
     *                         evtData4 evtData5 evtData6
     *                         0x00     0x00     0x00 ]
     *    sel type: 1 byte [0xC0]
     */
    evtData.push_back(0x3a);
    evtData.push_back(0xcd);
    evtData.push_back(0);
    evtData.push_back(evtData1);
    evtData.push_back(evtData2);
    evtData.push_back(evtData3);
    evtData.push_back(evtData4);
    evtData.push_back(evtData5);
    evtData.push_back(evtData6);
    evtData.push_back(0);
    evtData.push_back(0);
    evtData.push_back(0);

    auto& bus = pldm::utils::DBusHandler::getBus();
    try
    {
        auto method = bus.new_method_call("xyz.openbmc_project.Logging.IPMI",
                                          "/xyz/openbmc_project/Logging/IPMI",
                                          "xyz.openbmc_project.Logging.IPMI",
                                          "IpmiSelAddOem");
        method.append(message, evtData, recordType);

        auto selReply = bus.call(method);
        if (selReply.is_method_error())
        {
            std::cerr << "add SEL log error\n";
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "call SEL log error: " << e.what() << "\n";
    }
}

int OemEventManager::pldmPollForEventMessage(uint8_t TID,
                                             uint8_t /*eventClass*/,
                                             uint16_t eventID,
                                             std::vector<uint8_t> data)
{
#ifdef OEM_DEBUG
    std::cout << "\nOUTPUT DATA\n";
    for (auto it = data.begin(); it != data.end(); it++)
    {
        std::cout << std::hex << (unsigned)*it << " ";
    }
    std::cout << "\n";
#endif
    long pos;
    AmpereSpecData ampHdr;
    CommonEventData evtData;
    pos = 0;

    std::memcpy((void*)&evtData, (void*)&data[pos], sizeof(CommonEventData));
    pos += sizeof(CommonEventData);

    if (!std::filesystem::is_directory(CPER_LOG_DIR))
        std::filesystem::create_directories(CPER_LOG_DIR);

    std::string cperFile = std::string(CPER_LOG_DIR) + "cper.dump";
    std::ofstream out(cperFile.c_str(), std::ofstream::binary);
    if (!out.is_open())
    {
        std::cerr << "Can not open ofstream for CPER binary file\n";
        return -1;
    }
    decodeCperRecord(data, pos, &ampHdr, out);
    out.close();

    std::string prefix = "RAS_CPER_";
    std::string primaryLogId = pldm::utils::getUniqueEntryID(prefix);
    std::string type = "CPER";
    std::string faultLogFilePath = std::string(CPER_LOG_PATH) + primaryLogId;
    std::filesystem::copy(cperFile.c_str(), faultLogFilePath.c_str());
    std::filesystem::remove(cperFile.c_str());

    addSELLog(TID, eventID, &ampHdr);
    pldm::utils::addFaultLogToRedfish(primaryLogId, type);

    return data.size();
}

int OemEventManager::enqueueOverflowEvent(uint8_t tid, uint16_t eventId)
{
    if (overflowEventQueue.size() > MAX_QUEUE_SIZE)
        return -1;

    for (auto& i : overflowEventQueue)
    {
        if (i.first == tid && i.second == eventId)
        {
            return QUEUE_ITEM_EXIST;
        }
    }

#ifdef OEM_DEBUG
    std::cout << "\nQUEUING OVERFLOW EVENT_ID " << unsigned(tid) << ":"
              << eventId << " at " << pldm::utils::getCurrentSystemTime()
              << "\n";
#endif
    // insert to event queue
    overflowEventQueue.push_back(std::make_pair(tid, eventId));

    return 0;
}

void OemEventManager::handleNumericSensorEventSignal()
{
    pldmNumericSensorEventSignal = std::make_unique<sdbusplus::bus::match_t>(
        pldm::utils::DBusHandler::getBus(),
        sdbusplus::bus::match::rules::type::signal() +
            sdbusplus::bus::match::rules::member("NumericSensorEvent") +
            sdbusplus::bus::match::rules::path("/xyz/openbmc_project/pldm") +
            sdbusplus::bus::match::rules::interface(
                "xyz.openbmc_project.PLDM.Event"),
        [&](sdbusplus::message::message& msg) {
            try
            {
                uint8_t tid{};
                uint16_t sensorId{};
                uint8_t eventState{};
                uint8_t preEventState{};
                uint8_t sensorDataSize{};
                uint32_t presentReading{};
                /*
                 * Read the information of event
                 */
                msg.read(tid, sensorId, eventState, preEventState,
                         sensorDataSize, presentReading);

                if ((sensorId >= 191) && (sensorId <= 198))
                {
                    // add the priority
                    std::cerr << "Overflow: " << sensorId << "\n";
                    this->enqueueOverflowEvent(tid, sensorId);
                }
            }
            catch (const std::exception& e)
            {
                std::cerr << "handleDbusEventSignalMatch failed\n"
                          << e.what() << std::endl;
            }
        });
}

int OemEventManager::feedCriticalEventCb()
{
    std::pair<uint8_t, uint16_t> eventItem = std::make_pair(0, 0);

    if (isProcessPolling)
        return PLDM_ERROR;

    if (critEventQueue.empty() && overflowEventQueue.empty())
    {
        isCritical = false;
        return PLDM_ERROR;
    }

    if (!overflowEventQueue.empty())
    {
        eventItem = overflowEventQueue.front();
        overflowEventQueue.pop_front();
    }
    else
    {
        if (!critEventQueue.empty())
        {
            eventItem = critEventQueue.front();
            critEventQueue.pop_front();
        }
    }

    /* Has Critical Event */
    isCritical = true;
    /* prepare request data */
    reqData.tid = eventItem.first;
    reqData.operationFlag = PLDM_GET_FIRSTPART;
    reqData.dataTransferHandle = eventItem.second;
    reqData.eventIdToAck = eventItem.second;
#ifdef OEM_DEBUG
    std::cout << "\nFeed EVENT_ID " << unsigned(eventItem.first) << ":"
              << eventItem.second << " at "
              << pldm::utils::getCurrentSystemTime() << "\n";
#endif
    if (!pollTimer->isRunning())
    {
        pollTimer->start(
            duration_cast<std::chrono::milliseconds>(milliseconds(pollingTime)),
            true);
    }
    return PLDM_SUCCESS;
}

} // namespace platform_mc

} // namespace pldm
