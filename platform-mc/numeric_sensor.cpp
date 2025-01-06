#include "numeric_sensor.hpp"

#include "libpldm/platform.h"

#include "common/utils.hpp"
#include "requester/handler.hpp"

#include <phosphor-logging/commit.hpp>
#include <sdbusplus/asio/property.hpp>
#include <xyz/openbmc_project/Logging/Entry/client.hpp>
#include <xyz/openbmc_project/Sensor/Threshold/event.hpp>

#include <limits>
#include <regex>

PHOSPHOR_LOG2_USING;

namespace pldm
{
namespace platform_mc
{

inline bool NumericSensor::createInventoryPath(
    const std::string& associationPath, const std::string& sensorName,
    const uint16_t entityType, const uint16_t entityInstanceNum,
    const uint16_t containerId)
{
    auto& bus = pldm::utils::DBusHandler::getBus();
    std::string invPath = associationPath + "/" + sensorName;
    try
    {
        entityIntf = std::make_unique<EntityIntf>(bus, invPath.c_str());
    }
    catch (const sdbusplus::exception_t& e)
    {
        lg2::error(
            "Failed to create Entity interface for compact numeric sensor {PATH} error - {ERROR}",
            "PATH", invPath, "ERROR", e);
        return false;
    }
    entityIntf->entityType(entityType);
    entityIntf->entityInstanceNumber(entityInstanceNum);
    entityIntf->containerID(containerId);

    return true;
}

inline double getSensorDataValue(uint8_t sensor_data_size,
                                 union_sensor_data_size& value)
{
    double ret = std::numeric_limits<double>::quiet_NaN();
    switch (sensor_data_size)
    {
        case PLDM_SENSOR_DATA_SIZE_UINT8:
            ret = value.value_u8;
            break;
        case PLDM_SENSOR_DATA_SIZE_SINT8:
            ret = value.value_s8;
            break;
        case PLDM_SENSOR_DATA_SIZE_UINT16:
            ret = value.value_u16;
            break;
        case PLDM_SENSOR_DATA_SIZE_SINT16:
            ret = value.value_s16;
            break;
        case PLDM_SENSOR_DATA_SIZE_UINT32:
            ret = value.value_u32;
            break;
        case PLDM_SENSOR_DATA_SIZE_SINT32:
            ret = value.value_s32;
            break;
    }
    return ret;
}

inline double getRangeFieldValue(uint8_t range_field_format,
                                 union_range_field_format& value)
{
    double ret = std::numeric_limits<double>::quiet_NaN();
    switch (range_field_format)
    {
        case PLDM_RANGE_FIELD_FORMAT_UINT8:
            ret = value.value_u8;
            break;
        case PLDM_RANGE_FIELD_FORMAT_SINT8:
            ret = value.value_s8;
            break;
        case PLDM_RANGE_FIELD_FORMAT_UINT16:
            ret = value.value_u16;
            break;
        case PLDM_RANGE_FIELD_FORMAT_SINT16:
            ret = value.value_s16;
            break;
        case PLDM_RANGE_FIELD_FORMAT_UINT32:
            ret = value.value_u32;
            break;
        case PLDM_RANGE_FIELD_FORMAT_SINT32:
            ret = value.value_s32;
            break;
        case PLDM_RANGE_FIELD_FORMAT_REAL32:
            ret = value.value_f32;
            break;
    }
    return ret;
}

void NumericSensor::setSensorUnit(uint8_t baseUnit)
{
    sensorUnit = SensorUnit::DegreesC;
    useMetricInterface = false;
    switch (baseUnit)
    {
        case PLDM_SENSOR_UNIT_DEGRESS_C:
            sensorNameSpace = "/xyz/openbmc_project/sensors/temperature/";
            sensorUnit = SensorUnit::DegreesC;
            break;
        case PLDM_SENSOR_UNIT_VOLTS:
            sensorNameSpace = "/xyz/openbmc_project/sensors/voltage/";
            sensorUnit = SensorUnit::Volts;
            break;
        case PLDM_SENSOR_UNIT_AMPS:
            sensorNameSpace = "/xyz/openbmc_project/sensors/current/";
            sensorUnit = SensorUnit::Amperes;
            break;
        case PLDM_SENSOR_UNIT_RPM:
            sensorNameSpace = "/xyz/openbmc_project/sensors/fan_pwm/";
            sensorUnit = SensorUnit::RPMS;
            break;
        case PLDM_SENSOR_UNIT_WATTS:
            sensorNameSpace = "/xyz/openbmc_project/sensors/power/";
            sensorUnit = SensorUnit::Watts;
            break;
        case PLDM_SENSOR_UNIT_JOULES:
            sensorNameSpace = "/xyz/openbmc_project/sensors/energy/";
            sensorUnit = SensorUnit::Joules;
            break;
        case PLDM_SENSOR_UNIT_PERCENTAGE:
            sensorNameSpace = "/xyz/openbmc_project/sensors/utilization/";
            sensorUnit = SensorUnit::Percent;
            break;
        case PLDM_SENSOR_UNIT_COUNTS:
        case PLDM_SENSOR_UNIT_CORRECTED_ERRORS:
        case PLDM_SENSOR_UNIT_UNCORRECTABLE_ERRORS:
            sensorNameSpace = "/xyz/openbmc_project/metric/count/";
            useMetricInterface = true;
            break;
        case PLDM_SENSOR_UNIT_OEMUNIT:
            sensorNameSpace = "/xyz/openbmc_project/metric/oem/";
            useMetricInterface = true;
            break;
        default:
            lg2::error("Sensor {NAME} has Invalid baseUnit {UNIT}.", "NAME",
                       sensorName, "UNIT", baseUnit);
            throw sdbusplus::xyz::openbmc_project::Common::Error::
                InvalidArgument();
            break;
    }
}

NumericSensor::NumericSensor(
    const pldm_tid_t tid, const bool sensorDisabled,
    std::shared_ptr<pldm_numeric_sensor_value_pdr> pdr, std::string& sensorName,
    std::string& associationPath) : tid(tid), sensorName(sensorName)
{
    if (!pdr)
    {
        throw sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument();
    }

    sensorId = pdr->sensor_id;
    std::string path;
    MetricUnit metricUnit = MetricUnit::Count;
    setSensorUnit(pdr->base_unit);

    path = sensorNameSpace + sensorName;
    try
    {
        std::string tmp{};
        std::string interface = SENSOR_VALUE_INTF;
        if (useMetricInterface)
        {
            interface = METRIC_VALUE_INTF;
        }
        tmp = pldm::utils::DBusHandler().getService(path.c_str(),
                                                    interface.c_str());

        if (!tmp.empty())
        {
            throw sdbusplus::xyz::openbmc_project::Common::Error::
                TooManyResources();
        }
    }
    catch (const std::exception&)
    {
        /* The sensor object path is not created */
    }

    auto& bus = pldm::utils::DBusHandler::getBus();
    try
    {
        associationDefinitionsIntf =
            std::make_unique<AssociationDefinitionsInft>(bus, path.c_str());
    }
    catch (const sdbusplus::exception_t& e)
    {
        lg2::error(
            "Failed to create association interface for numeric sensor {PATH} error - {ERROR}",
            "PATH", path, "ERROR", e);
        throw sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument();
    }

    associationDefinitionsIntf->associations(
        {{"chassis", "all_sensors", associationPath}});

    double maxValue =
        getSensorDataValue(pdr->sensor_data_size, pdr->max_readable);
    double minValue =
        getSensorDataValue(pdr->sensor_data_size, pdr->min_readable);
    hysteresis = getSensorDataValue(pdr->sensor_data_size, pdr->hysteresis);

    bool hasCriticalThresholds = false;
    bool hasWarningThresholds = false;
    double criticalHigh = std::numeric_limits<double>::quiet_NaN();
    double criticalLow = std::numeric_limits<double>::quiet_NaN();
    double warningHigh = std::numeric_limits<double>::quiet_NaN();
    double warningLow = std::numeric_limits<double>::quiet_NaN();

    if (pdr->supported_thresholds.bits.bit0)
    {
        hasWarningThresholds = true;
        warningHigh =
            getRangeFieldValue(pdr->range_field_format, pdr->warning_high);
    }

    if (pdr->supported_thresholds.bits.bit3)
    {
        hasWarningThresholds = true;
        warningLow =
            getRangeFieldValue(pdr->range_field_format, pdr->warning_low);
    }

    if (pdr->supported_thresholds.bits.bit1)
    {
        hasCriticalThresholds = true;
        criticalHigh =
            getRangeFieldValue(pdr->range_field_format, pdr->critical_high);
    }

    if (pdr->supported_thresholds.bits.bit4)
    {
        hasCriticalThresholds = true;
        criticalLow =
            getRangeFieldValue(pdr->range_field_format, pdr->critical_low);
    }

    resolution = pdr->resolution;
    offset = pdr->offset;
    baseUnitModifier = pdr->unit_modifier;
    timeStamp = 0;

    /**
     * DEFAULT_SENSOR_UPDATER_INTERVAL is in milliseconds
     * updateTime is in microseconds
     */
    updateTime = static_cast<uint64_t>(DEFAULT_SENSOR_UPDATER_INTERVAL * 1000);
    if (std::isfinite(pdr->update_interval))
    {
        updateTime = pdr->update_interval * 1000000;
    }

    if (!useMetricInterface)
    {
        try
        {
            valueIntf = std::make_unique<ValueIntf>(bus, path.c_str());
        }
        catch (const sdbusplus::exception_t& e)
        {
            lg2::error(
                "Failed to create Value interface for numeric sensor {PATH} error - {ERROR}",
                "PATH", path, "ERROR", e);
            throw sdbusplus::xyz::openbmc_project::Common::Error::
                InvalidArgument();
        }
        valueIntf->maxValue(unitModifier(conversionFormula(maxValue)));
        valueIntf->minValue(unitModifier(conversionFormula(minValue)));
        valueIntf->unit(sensorUnit);
    }
    else
    {
        try
        {
            metricIntf = std::make_unique<MetricIntf>(bus, path.c_str());
        }
        catch (const sdbusplus::exception_t& e)
        {
            lg2::error(
                "Failed to create Metric interface for numeric sensor {PATH} error - {ERROR}",
                "PATH", path, "ERROR", e);
            throw sdbusplus::xyz::openbmc_project::Common::Error::
                InvalidArgument();
        }
        metricIntf->maxValue(unitModifier(conversionFormula(maxValue)));
        metricIntf->minValue(unitModifier(conversionFormula(minValue)));
        metricIntf->unit(metricUnit);
    }

    hysteresis = unitModifier(conversionFormula(hysteresis));

    if (!createInventoryPath(associationPath, sensorName, pdr->entity_type,
                             pdr->entity_instance_num, pdr->container_id))
    {
        throw sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument();
    }

    try
    {
        availabilityIntf =
            std::make_unique<AvailabilityIntf>(bus, path.c_str());
    }
    catch (const sdbusplus::exception_t& e)
    {
        lg2::error(
            "Failed to create Availability interface for numeric sensor {PATH} error - {ERROR}",
            "PATH", path, "ERROR", e);
        throw sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument();
    }
    availabilityIntf->available(true);

    try
    {
        operationalStatusIntf =
            std::make_unique<OperationalStatusIntf>(bus, path.c_str());
    }
    catch (const sdbusplus::exception_t& e)
    {
        lg2::error(
            "Failed to create Operation status interface for numeric sensor {PATH} error - {ERROR}",
            "PATH", path, "ERROR", e);
        throw sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument();
    }
    operationalStatusIntf->functional(!sensorDisabled);

    if (hasWarningThresholds && !useMetricInterface)
    {
        try
        {
            thresholdWarningIntf =
                std::make_unique<ThresholdWarningIntf>(bus, path.c_str());
        }
        catch (const sdbusplus::exception_t& e)
        {
            lg2::error(
                "Failed to create Threshold warning interface for numeric sensor {PATH} error - {ERROR}",
                "PATH", path, "ERROR", e);
            throw sdbusplus::xyz::openbmc_project::Common::Error::
                InvalidArgument();
        }
        thresholdWarningIntf->warningHigh(unitModifier(warningHigh));
        thresholdWarningIntf->warningLow(unitModifier(warningLow));
    }

    if (hasCriticalThresholds && !useMetricInterface)
    {
        try
        {
            thresholdCriticalIntf =
                std::make_unique<ThresholdCriticalIntf>(bus, path.c_str());
        }
        catch (const sdbusplus::exception_t& e)
        {
            lg2::error(
                "Failed to create Threshold critical interface for numeric sensor {PATH} error - {ERROR}",
                "PATH", path, "ERROR", e);
            throw sdbusplus::xyz::openbmc_project::Common::Error::
                InvalidArgument();
        }
        thresholdCriticalIntf->criticalHigh(unitModifier(criticalHigh));
        thresholdCriticalIntf->criticalLow(unitModifier(criticalLow));
    }
}

NumericSensor::NumericSensor(
    const pldm_tid_t tid, const bool sensorDisabled,
    std::shared_ptr<pldm_compact_numeric_sensor_pdr> pdr,
    std::string& sensorName, std::string& associationPath) :
    tid(tid), sensorName(sensorName)
{
    if (!pdr)
    {
        throw sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument();
    }

    sensorId = pdr->sensor_id;
    std::string path;
    MetricUnit metricUnit = MetricUnit::Count;
    setSensorUnit(pdr->base_unit);

    path = sensorNameSpace + sensorName;
    try
    {
        std::string tmp{};
        std::string interface = SENSOR_VALUE_INTF;
        if (useMetricInterface)
        {
            interface = METRIC_VALUE_INTF;
        }
        tmp = pldm::utils::DBusHandler().getService(path.c_str(),
                                                    interface.c_str());

        if (!tmp.empty())
        {
            throw sdbusplus::xyz::openbmc_project::Common::Error::
                TooManyResources();
        }
    }
    catch (const std::exception&)
    {
        /* The sensor object path is not created */
    }

    auto& bus = pldm::utils::DBusHandler::getBus();
    try
    {
        associationDefinitionsIntf =
            std::make_unique<AssociationDefinitionsInft>(bus, path.c_str());
    }
    catch (const sdbusplus::exception_t& e)
    {
        lg2::error(
            "Failed to create Association interface for compact numeric sensor {PATH} error - {ERROR}",
            "PATH", path, "ERROR", e);
        throw sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument();
    }
    associationDefinitionsIntf->associations(
        {{"chassis", "all_sensors", associationPath.c_str()}});

    double maxValue = std::numeric_limits<double>::quiet_NaN();
    double minValue = std::numeric_limits<double>::quiet_NaN();
    bool hasWarningThresholds = false;
    bool hasCriticalThresholds = false;
    double criticalHigh = std::numeric_limits<double>::quiet_NaN();
    double criticalLow = std::numeric_limits<double>::quiet_NaN();
    double warningHigh = std::numeric_limits<double>::quiet_NaN();
    double warningLow = std::numeric_limits<double>::quiet_NaN();

    if (pdr->range_field_support.bits.bit0)
    {
        hasWarningThresholds = true;
        warningHigh = pdr->warning_high;
    }
    if (pdr->range_field_support.bits.bit1)
    {
        hasWarningThresholds = true;
        warningLow = pdr->warning_low;
    }

    if (pdr->range_field_support.bits.bit2)
    {
        hasCriticalThresholds = true;
        criticalHigh = pdr->critical_high;
    }

    if (pdr->range_field_support.bits.bit3)
    {
        hasCriticalThresholds = true;
        criticalLow = pdr->critical_low;
    }

    resolution = std::numeric_limits<double>::quiet_NaN();
    offset = std::numeric_limits<double>::quiet_NaN();
    baseUnitModifier = pdr->unit_modifier;
    timeStamp = 0;
    hysteresis = 0;

    /**
     * DEFAULT_SENSOR_UPDATER_INTERVAL is in milliseconds
     * updateTime is in microseconds
     */
    updateTime = static_cast<uint64_t>(DEFAULT_SENSOR_UPDATER_INTERVAL * 1000);

    if (!useMetricInterface)
    {
        try
        {
            valueIntf = std::make_unique<ValueIntf>(bus, path.c_str());
        }
        catch (const sdbusplus::exception_t& e)
        {
            lg2::error(
                "Failed to create Value interface for compact numeric sensor {PATH} error - {ERROR}",
                "PATH", path, "ERROR", e);
            throw sdbusplus::xyz::openbmc_project::Common::Error::
                InvalidArgument();
        }
        valueIntf->maxValue(unitModifier(conversionFormula(maxValue)));
        valueIntf->minValue(unitModifier(conversionFormula(minValue)));
        valueIntf->unit(sensorUnit);
    }
    else
    {
        try
        {
            metricIntf = std::make_unique<MetricIntf>(bus, path.c_str());
        }
        catch (const sdbusplus::exception_t& e)
        {
            lg2::error(
                "Failed to create Metric interface for compact numeric sensor {PATH} error - {ERROR}",
                "PATH", path, "ERROR", e);
            throw sdbusplus::xyz::openbmc_project::Common::Error::
                InvalidArgument();
        }
        metricIntf->maxValue(unitModifier(conversionFormula(maxValue)));
        metricIntf->minValue(unitModifier(conversionFormula(minValue)));
        metricIntf->unit(metricUnit);
    }

    hysteresis = unitModifier(conversionFormula(hysteresis));

    if (!createInventoryPath(associationPath, sensorName, pdr->entity_type,
                             pdr->entity_instance, pdr->container_id))
    {
        throw sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument();
    }

    try
    {
        availabilityIntf =
            std::make_unique<AvailabilityIntf>(bus, path.c_str());
    }
    catch (const sdbusplus::exception_t& e)
    {
        lg2::error(
            "Failed to create Availability interface for compact numeric sensor {PATH} error - {ERROR}",
            "PATH", path, "ERROR", e);
        throw sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument();
    }
    availabilityIntf->available(true);

    try
    {
        operationalStatusIntf =
            std::make_unique<OperationalStatusIntf>(bus, path.c_str());
    }
    catch (const sdbusplus::exception_t& e)
    {
        lg2::error(
            "Failed to create Operational status interface for compact numeric sensor {PATH} error - {ERROR}",
            "PATH", path, "ERROR", e);
        throw sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument();
    }
    operationalStatusIntf->functional(!sensorDisabled);

    if (hasWarningThresholds && !useMetricInterface)
    {
        try
        {
            thresholdWarningIntf =
                std::make_unique<ThresholdWarningIntf>(bus, path.c_str());
        }
        catch (const sdbusplus::exception_t& e)
        {
            lg2::error(
                "Failed to create Warning threshold interface for compact numeric sensor {PATH} error - {ERROR}",
                "PATH", path, "ERROR", e);
            throw sdbusplus::xyz::openbmc_project::Common::Error::
                InvalidArgument();
        }
        thresholdWarningIntf->warningHigh(unitModifier(warningHigh));
        thresholdWarningIntf->warningLow(unitModifier(warningLow));
    }

    if (hasCriticalThresholds && !useMetricInterface)
    {
        try
        {
            thresholdCriticalIntf =
                std::make_unique<ThresholdCriticalIntf>(bus, path.c_str());
        }
        catch (const sdbusplus::exception_t& e)
        {
            lg2::error(
                "Failed to create Critical threshold interface for compact numeric sensor {PATH} error - {ERROR}",
                "PATH", path, "ERROR", e);
            throw sdbusplus::xyz::openbmc_project::Common::Error::
                InvalidArgument();
        }
        thresholdCriticalIntf->criticalHigh(unitModifier(criticalHigh));
        thresholdCriticalIntf->criticalLow(unitModifier(criticalLow));
    }
}

double NumericSensor::conversionFormula(double value)
{
    double convertedValue = value;
    if (std::isfinite(resolution))
    {
        convertedValue *= resolution;
    }
    if (std::isfinite(offset))
    {
        convertedValue += offset;
    }
    return convertedValue;
}

double NumericSensor::unitModifier(double value)
{
    if (!std::isfinite(value))
    {
        return value;
    }
    return value * std::pow(10, baseUnitModifier);
}

void NumericSensor::updateReading(bool available, bool functional, double value)
{
    if (!availabilityIntf || !operationalStatusIntf ||
        (!useMetricInterface && !valueIntf) ||
        (useMetricInterface && !metricIntf))
    {
        lg2::error(
            "Failed to update sensor {NAME} D-Bus interface don't exist.",
            "NAME", sensorName);
        return;
    }
    availabilityIntf->available(available);
    operationalStatusIntf->functional(functional);
    double curValue = 0;
    if (!useMetricInterface)
    {
        curValue = valueIntf->value();
    }
    else
    {
        curValue = metricIntf->value();
    }

    double newValue = std::numeric_limits<double>::quiet_NaN();
    if (functional && available)
    {
        newValue = unitModifier(conversionFormula(value));
        if (newValue != curValue &&
            (std::isfinite(newValue) || std::isfinite(curValue)))
        {
            if (!useMetricInterface)
            {
                valueIntf->value(newValue);
                updateThresholds();
            }
            else
            {
                metricIntf->value(newValue);
            }
        }
    }
    else
    {
        if (newValue != curValue &&
            (std::isfinite(newValue) || std::isfinite(curValue)))
        {
            if (!useMetricInterface)
            {
                valueIntf->value(std::numeric_limits<double>::quiet_NaN());
            }
            else
            {
                metricIntf->value(std::numeric_limits<double>::quiet_NaN());
            }
        }
    }
}

void NumericSensor::handleErrGetSensorReading()
{
    if (!operationalStatusIntf || (!useMetricInterface && !valueIntf) ||
        (useMetricInterface && !metricIntf))
    {
        lg2::error(
            "Failed to update sensor {NAME} D-Bus interfaces don't exist.",
            "NAME", sensorName);
        return;
    }
    operationalStatusIntf->functional(false);
    if (!useMetricInterface)
    {
        valueIntf->value(std::numeric_limits<double>::quiet_NaN());
    }
    else
    {
        metricIntf->value(std::numeric_limits<double>::quiet_NaN());
    }
}

bool NumericSensor::checkThreshold(bool alarm, bool direction, double value,
                                   double threshold, double hyst)
{
    if (direction)
    {
        if (value >= threshold)
        {
            return true;
        }
        if (value < (threshold - hyst))
        {
            return false;
        }
    }
    else
    {
        if (value <= threshold)
        {
            return true;
        }
        if (value > (threshold + hyst))
        {
            return false;
        }
    }
    return alarm;
}

bool
    NumericSensor::clearLog(std::optional<sdbusplus::message::object_path>& log)
{
    if (log)
    {
        try
        {
#if 0
        // Wait for merge.
        lg2::resolve(*log);
#endif
            log.reset();
            return true;
        }
        catch (std::exception& ec)
        {
            std::cerr << "Error trying to resolve: " << std::string(*log)
                      << " : " << ec.what() << std::endl;
        }
    }
    return false;
}

void NumericSensor::logNormalRange(double value)
{
    namespace Events =
        sdbusplus::event::xyz::openbmc_project::sensor::Threshold;
    std::string objPath = sensorNameSpace + "/" + sensorName;
    lg2::commit(Events::SensorReadingNormalRange(
        "SENSOR_NAME", objPath, "READING_VALUE", value, "UNITS", sensorUnit));
}

void NumericSensor::updateThresholds()
{
    double value = std::numeric_limits<double>::quiet_NaN();
    namespace Errors =
        sdbusplus::error::xyz::openbmc_project::sensor::Threshold;

    if ((!useMetricInterface && !valueIntf) ||
        (useMetricInterface && !metricIntf))
    {
        lg2::error(
            "Failed to update thresholds sensor {NAME} D-Bus interfaces don't exist.",
            "NAME", sensorName);
        return;
    }
    if (!useMetricInterface)
    {
        value = valueIntf->value();
    }
    else
    {
        value = metricIntf->value();
    }
    bool outstandingAlarm = false;
    bool clearedLog = false;
    std::string sensorObjPath = sensorNameSpace + "/" + sensorName;
    if (thresholdWarningIntf &&
        std::isfinite(thresholdWarningIntf->warningHigh()))
    {
        auto threshold = thresholdWarningIntf->warningHigh();
        auto alarm = thresholdWarningIntf->warningAlarmHigh();
        auto newAlarm =
            checkThreshold(alarm, true, value, threshold, hysteresis);
        outstandingAlarm |= alarm;
        if (alarm != newAlarm)
        {
            thresholdWarningIntf->warningAlarmHigh(newAlarm);
            if (newAlarm)
            {
                thresholdWarningIntf->warningHighAlarmAsserted(value);
                assertedUpperWarningLog =
                    lg2::commit(Errors::ReadingAboveUpperWarningThreshold(
                        "SENSOR_NAME", sensorObjPath, "READING_VALUE", value,
                        "UNITS", sensorUnit, "THRESHOLD_VALUE", threshold));
            }
            else
            {
                thresholdWarningIntf->warningHighAlarmDeasserted(value);
                clearedLog |= clearLog(assertedUpperWarningLog);
            }
        }
    }

    if (thresholdWarningIntf &&
        std::isfinite(thresholdWarningIntf->warningLow()))
    {
        auto threshold = thresholdWarningIntf->warningLow();
        auto alarm = thresholdWarningIntf->warningAlarmLow();
        outstandingAlarm |= alarm;
        auto newAlarm =
            checkThreshold(alarm, false, value, threshold, hysteresis);
        if (alarm != newAlarm)
        {
            thresholdWarningIntf->warningAlarmLow(newAlarm);
            if (newAlarm)
            {
                thresholdWarningIntf->warningLowAlarmAsserted(value);
                assertedLowerWarningLog =
                    lg2::commit(Errors::ReadingBelowLowerWarningThreshold(
                        "SENSOR_NAME", sensorObjPath, "READING_VALUE", value,
                        "UNITS", sensorUnit, "THRESHOLD_VALUE", threshold));
            }
            else
            {
                thresholdWarningIntf->warningLowAlarmDeasserted(value);
                clearedLog |= clearLog(assertedLowerWarningLog);
            }
        }
    }

    if (thresholdCriticalIntf &&
        std::isfinite(thresholdCriticalIntf->criticalHigh()))
    {
        auto threshold = thresholdCriticalIntf->criticalHigh();
        auto alarm = thresholdCriticalIntf->criticalAlarmHigh();
        outstandingAlarm |= alarm;
        auto newAlarm =
            checkThreshold(alarm, true, value, threshold, hysteresis);
        if (alarm != newAlarm)
        {
            thresholdCriticalIntf->criticalAlarmHigh(newAlarm);
            if (newAlarm)
            {
                thresholdCriticalIntf->criticalHighAlarmAsserted(value);
                assertedUpperCriticalLog =
                    lg2::commit(Errors::ReadingAboveUpperCriticalThreshold(
                        "SENSOR_NAME", sensorObjPath, "READING_VALUE", value,
                        "UNITS", sensorUnit, "THRESHOLD_VALUE", threshold));
            }
            else
            {
                thresholdCriticalIntf->criticalHighAlarmDeasserted(value);
                clearedLog |= clearLog(assertedUpperCriticalLog);
            }
        }
    }

    if (thresholdCriticalIntf &&
        std::isfinite(thresholdCriticalIntf->criticalLow()))
    {
        auto threshold = thresholdCriticalIntf->criticalLow();
        auto alarm = thresholdCriticalIntf->criticalAlarmLow();
        outstandingAlarm |= alarm;
        auto newAlarm =
            checkThreshold(alarm, false, value, threshold, hysteresis);
        if (alarm != newAlarm)
        {
            thresholdCriticalIntf->criticalAlarmLow(newAlarm);
            if (newAlarm)
            {
                thresholdCriticalIntf->criticalLowAlarmAsserted(value);
                assertedLowerCriticalLog =
                    lg2::commit(Errors::ReadingBelowLowerCriticalThreshold(
                        "SENSOR_NAME", sensorObjPath, "READING_VALUE", value,
                        "UNITS", sensorUnit, "THRESHOLD_VALUE", threshold));
            }
            else
            {
                thresholdCriticalIntf->criticalLowAlarmDeasserted(value);
                clearedLog |= clearLog(assertedLowerCriticalLog);
            }
        }
    }
    if (clearedLog && !outstandingAlarm)
    {
        logNormalRange(value);
    }
}

int NumericSensor::triggerThresholdEvent(
    pldm::utils::Level eventType, pldm::utils::Direction direction,
    double rawValue, bool newAlarm, bool assert)
{
    namespace Errors =
        sdbusplus::error::xyz::openbmc_project::sensor::Threshold;
    if (!valueIntf)
    {
        lg2::error(
            "Failed to update thresholds sensor {NAME} D-Bus interfaces don't exist.",
            "NAME", sensorName);
        return PLDM_ERROR;
    }

    auto value = unitModifier(conversionFormula(rawValue));
    lg2::error(
        "triggerThresholdEvent eventType {TID}, direction {SID} value {VAL} newAlarm {PSTATE} assert {ESTATE}",
        "TID", eventType, "SID", direction, "VAL", value, "PSTATE", newAlarm,
        "ESTATE", assert);

    bool outstandingAlarm = false;
    bool clearedLog = false;
    std::string sensorObjPath = sensorNameSpace + "/" + sensorName;

    switch (eventType)
    {
        case pldm::utils::Level::WARNING:
        {
            if (!thresholdWarningIntf)
            {
                lg2::error(
                    "Error:Trigger sensor warning event for non warning threshold sensors {NAME}",
                    "NAME", sensorName);
                return PLDM_ERROR;
            }
            if (direction == pldm::utils::Direction::HIGH &&
                std::isfinite(thresholdWarningIntf->warningHigh()))
            {
                auto alarm = thresholdWarningIntf->warningAlarmHigh();
                outstandingAlarm |= alarm;
                if (alarm == newAlarm)
                {
                    return PLDM_SUCCESS;
                }
                thresholdWarningIntf->warningAlarmHigh(newAlarm);
                if (assert)
                {
                    auto threshold = thresholdWarningIntf->warningHigh();
                    thresholdWarningIntf->warningHighAlarmAsserted(value);
                    assertedUpperWarningLog =
                        lg2::commit(Errors::ReadingAboveUpperWarningThreshold(
                            "SENSOR_NAME", sensorObjPath, "READING_VALUE",
                            value, "UNITS", sensorUnit, "THRESHOLD_VALUE",
                            threshold));
                }
                else
                {
                    thresholdWarningIntf->warningHighAlarmDeasserted(value);
                    clearedLog |= clearLog(assertedUpperWarningLog);
                }
            }
            else if (direction == pldm::utils::Direction::LOW &&
                     std::isfinite(thresholdWarningIntf->warningLow()))
            {
                auto alarm = thresholdWarningIntf->warningAlarmLow();
                outstandingAlarm |= alarm;
                if (alarm == newAlarm)
                {
                    return PLDM_SUCCESS;
                }
                thresholdWarningIntf->warningAlarmLow(newAlarm);
                if (assert)
                {
                    auto threshold = thresholdWarningIntf->warningLow();
                    thresholdWarningIntf->warningLowAlarmAsserted(value);
                    assertedLowerWarningLog =
                        lg2::commit(Errors::ReadingBelowLowerWarningThreshold(
                            "SENSOR_NAME", sensorObjPath, "READING_VALUE",
                            value, "UNITS", sensorUnit, "THRESHOLD_VALUE",
                            threshold));
                }
                else
                {
                    thresholdWarningIntf->warningLowAlarmDeasserted(value);
                    clearedLog |= clearLog(assertedLowerWarningLog);
                }
            }
            break;
        }
        case pldm::utils::Level::CRITICAL:
        {
            if (!thresholdCriticalIntf)
            {
                lg2::error(
                    "Error:Trigger sensor Critical event for non warning threshold sensors {NAME}",
                    "NAME", sensorName);
                return PLDM_ERROR;
            }
            if (direction == pldm::utils::Direction::HIGH &&
                std::isfinite(thresholdCriticalIntf->criticalHigh()))
            {
                auto alarm = thresholdCriticalIntf->criticalAlarmHigh();
                outstandingAlarm |= alarm;
                if (alarm == newAlarm)
                {
                    return PLDM_SUCCESS;
                }
                thresholdCriticalIntf->criticalAlarmHigh(newAlarm);
                if (assert)
                {
                    auto threshold = thresholdCriticalIntf->criticalHigh();
                    thresholdCriticalIntf->criticalHighAlarmAsserted(value);
                    assertedUpperCriticalLog =
                        lg2::commit(Errors::ReadingAboveUpperCriticalThreshold(
                            "SENSOR_NAME", sensorObjPath, "READING_VALUE",
                            value, "UNITS", sensorUnit, "THRESHOLD_VALUE",
                            threshold));
                }
                else
                {
                    thresholdCriticalIntf->criticalHighAlarmDeasserted(value);
                    clearedLog |= clearLog(assertedUpperCriticalLog);
                }
            }
            else if (direction == pldm::utils::Direction::LOW &&
                     std::isfinite(thresholdCriticalIntf->criticalLow()))
            {
                auto alarm = thresholdCriticalIntf->criticalAlarmLow();
                outstandingAlarm |= alarm;
                if (alarm == newAlarm)
                {
                    return PLDM_SUCCESS;
                }
                thresholdCriticalIntf->criticalAlarmLow(newAlarm);
                if (assert)
                {
                    auto threshold = thresholdCriticalIntf->criticalLow();
                    thresholdCriticalIntf->criticalLowAlarmAsserted(value);
                    assertedLowerCriticalLog =
                        lg2::commit(Errors::ReadingBelowLowerCriticalThreshold(
                            "SENSOR_NAME", sensorObjPath, "READING_VALUE",
                            value, "UNITS", sensorUnit, "THRESHOLD_VALUE",
                            threshold));
                }
                else
                {
                    thresholdCriticalIntf->criticalLowAlarmDeasserted(value);
                    clearedLog |= clearLog(assertedLowerCriticalLog);
                }
            }
            break;
        }

        default:
            break;
    }

    if (clearedLog && !outstandingAlarm)
    {
        logNormalRange(value);
    }

    return PLDM_SUCCESS;
}
} // namespace platform_mc
} // namespace pldm
