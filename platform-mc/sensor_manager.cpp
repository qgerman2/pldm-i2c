#include "sensor_manager.hpp"

#include "terminus_manager.hpp"

#include <phosphor-logging/lg2.hpp>

namespace pldm
{
namespace platform_mc
{

using namespace std::chrono;

SensorManager::SensorManager(
    sdeventplus::Event& event, TerminusManager& terminusManager,
    std::map<tid_t, std::shared_ptr<Terminus>>& termini, bool verbose,
    const std::filesystem::path& configJson) :
    event(event),
    terminusManager(terminusManager), termini(termini),
    pollingTime(SENSOR_POLLING_TIME), verbose(verbose)
{
    // default priority sensor name spaces
    prioritySensorNameSpaces.emplace_back(
        "/xyz/openbmc_project/sensors/temperature/");
    prioritySensorNameSpaces.emplace_back(
        "/xyz/openbmc_project/sensors/power/");
    prioritySensorNameSpaces.emplace_back(
        "/xyz/openbmc_project/sensors/energy/");

    if (!std::filesystem::exists(configJson))
    {
        return;
    }

    std::ifstream jsonFile(configJson);
    auto data = nlohmann::json::parse(jsonFile, nullptr, false);
    if (data.is_discarded())
    {
        lg2::error("Parsing json file failed. FilePath={FILE_PATH}",
                   "FILE_PATH", std::string(configJson));
        return;
    }

    // load priority sensor name spaces
    const std::vector<std::string> emptyStringArray{};
    auto nameSpaces = data.value("PrioritySensorNameSpaces", emptyStringArray);
    if (nameSpaces.size() > 0)
    {
        prioritySensorNameSpaces.clear();
        for (const auto& nameSpace : nameSpaces)
        {
            prioritySensorNameSpaces.emplace_back(nameSpace);
        }
    }
}

bool SensorManager::isPriority(std::shared_ptr<NumericSensor> sensor)
{
    return (std::find(prioritySensorNameSpaces.begin(),
                      prioritySensorNameSpaces.end(),
                      sensor->sensorNameSpace) !=
            prioritySensorNameSpaces.end());
}
void SensorManager::startPolling(tid_t tid)
{
    // initialize prioritySensors and roundRobinSensors list
    for (const auto& [tId, terminus] : termini)
    {
        if (tid != tId)
        {
            continue;
        }
        // numeric sensor
        for (auto& sensor : terminus->numericSensors)
        {
            if (isPriority(sensor))
            {
                sensor->isPriority = true;
                prioritySensors[tid].emplace_back(sensor);
            }
            else
            {
                sensor->isPriority = false;
                roundRobinSensors[tid].push(sensor);
            }
        }

        if (sensorPollTimers.find(tid) == sensorPollTimers.end())
        {
            sensorPollTimers[tid] = std::make_unique<phosphor::Timer>(
                event.get(),
                std::bind_front(&SensorManager::doSensorPolling, this, tid));
        }

        if (!sensorPollTimers[tid]->isRunning())
        {
            sensorPollTimers[tid]->start(
                duration_cast<std::chrono::milliseconds>(
                    milliseconds(pollingTime)),
                true);
        }
    }
}

void SensorManager::stopPolling(tid_t tid)
{
    if (prioritySensors.find(tid) != prioritySensors.end())
    {
        prioritySensors[tid].clear();
    }
    if (roundRobinSensors.find(tid) != roundRobinSensors.end())
    {
        while (!roundRobinSensors[tid].empty())
        {
            roundRobinSensors[tid].pop();
        }
    }

    for (const auto& [tId, terminus] : termini)
    {
        if (tid != tId)
        {
            continue;
        }

        if (sensorPollTimers[tid])
        {
            sensorPollTimers[tid]->stop();
        }
    }
}

void SensorManager::doSensorPolling(tid_t tid)
{
    if (doSensorPollingTaskHandles[tid])
    {
        if (doSensorPollingTaskHandles[tid].done())
        {
            doSensorPollingTaskHandles[tid].destroy();
            auto co = doSensorPollingTask(tid);
            doSensorPollingTaskHandles[tid] = co.handle;
            if (doSensorPollingTaskHandles[tid].done())
            {
                doSensorPollingTaskHandles[tid] = nullptr;
            }
        }
    }
    else
    {
        auto co = doSensorPollingTask(tid);
        doSensorPollingTaskHandles[tid] = co.handle;
        if (doSensorPollingTaskHandles[tid].done())
        {
            doSensorPollingTaskHandles[tid] = nullptr;
        }
    }
}

requester::Coroutine SensorManager::doSensorPollingTask(tid_t tid)
{
    uint64_t t0 = 0;
    uint64_t t1 = 0;
    uint64_t elapsed = 0;
    uint64_t pollingTimeInUsec = pollingTime * 1000;

    do
    {
        if (sensorPollTimers[tid] && !sensorPollTimers[tid]->isRunning())
        {
            co_return PLDM_ERROR;
        }

        sd_event_now(event.get(), CLOCK_MONOTONIC, &t0);
        if (verbose)
        {
            lg2::info("TID:{TID} start sensor polling at {NOW}.", "TID", tid,
                      "NOW", t0);
        }

        if (termini.find(tid) == termini.end())
        {
            co_return PLDM_SUCCESS;
        }

        // poll priority Sensors
        for (auto& sensor : prioritySensors[tid])
        {
            if (sensor->updateTime == std::numeric_limits<uint64_t>::max())
            {
                continue;
            }

            sd_event_now(event.get(), CLOCK_MONOTONIC, &t1);
            elapsed = t1 - sensor->timeStamp;
            if (sensor->updateTime <= elapsed)
            {
                co_await getSensorReading(sensor);
                if (sensorPollTimers[tid] &&
                    !sensorPollTimers[tid]->isRunning())
                {
                    co_return PLDM_ERROR;
                }
                sd_event_now(event.get(), CLOCK_MONOTONIC, &t1);
                sensor->timeStamp = t1;
            }
        }

        // poll roundRobin Sensors
        sd_event_now(event.get(), CLOCK_MONOTONIC, &t1);
        auto toBeUpdated = roundRobinSensors[tid].size();
        while (((t1 - t0) < pollingTimeInUsec) && (toBeUpdated > 0))
        {
            auto sensor = roundRobinSensors[tid].front();

            sd_event_now(event.get(), CLOCK_MONOTONIC, &t1);
            elapsed = t1 - sensor->timeStamp;
            if (sensor->updateTime <= elapsed)
            {
                co_await getSensorReading(sensor);
                if (sensorPollTimers[tid] &&
                    !sensorPollTimers[tid]->isRunning())
                {
                    co_return PLDM_ERROR;
                }
                sd_event_now(event.get(), CLOCK_MONOTONIC, &t1);
                sensor->timeStamp = t1;
            }

            toBeUpdated--;
            roundRobinSensors[tid].pop();
            roundRobinSensors[tid].push(std::move(sensor));
            sd_event_now(event.get(), CLOCK_MONOTONIC, &t1);
        }

        if (verbose)
        {
            sd_event_now(event.get(), CLOCK_MONOTONIC, &t1);
            lg2::info("end sensor polling at {END}. duration(us):{DELTA}",
                      "END", t1, "DELTA", t1 - t0);
        }

        sd_event_now(event.get(), CLOCK_MONOTONIC, &t1);
    } while ((t1 - t0) >= pollingTimeInUsec);

    co_return PLDM_SUCCESS;
}

requester::Coroutine
    SensorManager::getSensorReading(std::shared_ptr<NumericSensor> sensor)
{
    auto tid = sensor->tid;
    auto sensorId = sensor->sensorId;
    Request request(sizeof(pldm_msg_hdr) + PLDM_GET_SENSOR_READING_REQ_BYTES);
    auto requestMsg = reinterpret_cast<pldm_msg*>(request.data());
    auto rc = encode_get_sensor_reading_req(0, sensorId, false, requestMsg);
    if (rc)
    {
        lg2::error("encode_get_sensor_reading_req failed, tid={TID}, rc={RC}.",
                   "TID", tid, "RC", rc);
        co_return rc;
    }

    const pldm_msg* responseMsg = NULL;
    size_t responseLen = 0;
    rc = co_await terminusManager.SendRecvPldmMsg(tid, request, &responseMsg,
                                                  &responseLen);
    if (rc)
    {
        co_return rc;
    }

    if (sensorPollTimers[tid] && !sensorPollTimers[tid]->isRunning())
    {
        co_return PLDM_ERROR;
    }

    uint8_t completionCode = PLDM_SUCCESS;
    uint8_t sensorDataSize = PLDM_SENSOR_DATA_SIZE_SINT32;
    uint8_t sensorOperationalState = 0;
    uint8_t sensorEventMessageEnable = 0;
    uint8_t presentState = 0;
    uint8_t previousState = 0;
    uint8_t eventState = 0;
    union_sensor_data_size presentReading;
    rc = decode_get_sensor_reading_resp(
        responseMsg, responseLen, &completionCode, &sensorDataSize,
        &sensorOperationalState, &sensorEventMessageEnable, &presentState,
        &previousState, &eventState,
        reinterpret_cast<uint8_t*>(&presentReading));
    if (rc)
    {
        lg2::error(
            "Failed to decode response of GetSensorReading, tid={TID}, rc={RC}.",
            "TID", tid, "RC", rc);
        sensor->handleErrGetSensorReading();
        co_return rc;
    }

    if (completionCode != PLDM_SUCCESS)
    {
        lg2::error(
            "Failed to decode response of GetSensorReading, tid={TID}, rc={RC}, cc={CC}.",
            "TID", tid, "RC", rc, "CC", completionCode);
        co_return completionCode;
    }

    switch (sensorOperationalState)
    {
        case PLDM_SENSOR_ENABLED:
            break;
        case PLDM_SENSOR_DISABLED:
            sensor->updateReading(true, false, 0);
            co_return completionCode;
        case PLDM_SENSOR_UNAVAILABLE:
        default:
            sensor->updateReading(false, false, 0);
            co_return completionCode;
    }

    double value;
    switch (sensorDataSize)
    {
        case PLDM_SENSOR_DATA_SIZE_UINT8:
            value = static_cast<double>(presentReading.value_u8);
            break;
        case PLDM_SENSOR_DATA_SIZE_SINT8:
            value = static_cast<double>(presentReading.value_s8);
            break;
        case PLDM_SENSOR_DATA_SIZE_UINT16:
            value = static_cast<double>(presentReading.value_u16);
            break;
        case PLDM_SENSOR_DATA_SIZE_SINT16:
            value = static_cast<double>(presentReading.value_s16);
            break;
        case PLDM_SENSOR_DATA_SIZE_UINT32:
            value = static_cast<double>(presentReading.value_u32);
            break;
        case PLDM_SENSOR_DATA_SIZE_SINT32:
            value = static_cast<double>(presentReading.value_s32);
            break;
        default:
            value = std::numeric_limits<double>::quiet_NaN();
            break;
    }

    sensor->updateReading(true, true, value);
    co_return completionCode;
}

} // namespace platform_mc
} // namespace pldm
