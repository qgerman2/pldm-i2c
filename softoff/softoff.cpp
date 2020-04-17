#include "config.h"

#include "softoff.hpp"

#include "utils.hpp"

#include <array>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>
#include <xyz/openbmc_project/State/Host/server.hpp>

#include "libpldm/platform.h"
#include "libpldm/requester/pldm.h"

namespace pldm
{

constexpr auto timeOutJsonPath = "/usr/share/pldm/softoff/softoff.json";
constexpr auto HOST_SOFTOFF_MCTP_ID = 0;
constexpr auto HOST_SOFTOFF_EFFECTER_ID = 0;
constexpr auto HOST_SOFTOFF_STATE = 0;

using Json = nlohmann::json;
namespace fs = std::filesystem;
using sdbusplus::exception::SdBusError;

using namespace sdbusplus::xyz::openbmc_project::State::server;
namespace sdbusRule = sdbusplus::bus::match::rules;

constexpr auto SYSTEMD_BUSNAME = "org.freedesktop.systemd1";
constexpr auto SYSTEMD_PATH = "/org/freedesktop/systemd1";
constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";
constexpr auto GUARD_CHASSIS_OFF_TARGET_SERVICE =
    "guardChassisOffTarget@.service";

constexpr auto PLDM_POWER_OFF_PATH =
    "/xyz/openbmc_project/pldm/softoff"; // This path is just an example. The
                                         // PLDM_POWER_OFF_PATH need to be
                                         // creat.
constexpr auto PLDM_POWER_OFF_INTERFACE =
    "xyz.openbmc_project.Pldm.Softoff"; // This interface is just an example.
                                        // The PLDM_POWER_OFF_INTERFACE need to
                                        // be creat.

PldmSoftPowerOff::PldmSoftPowerOff(sdbusplus::bus::bus& bus, sd_event* event) :
    bus(bus), timer(event),
    hostTransitionMatch(bus,
                        sdbusRule::propertiesChanged(PLDM_POWER_OFF_PATH,
                                                     PLDM_POWER_OFF_INTERFACE),
                        std::bind(&PldmSoftPowerOff::setHostSoftOffCompleteFlag,
                                  this, std::placeholders::_1))
{
    auto rc = setStateEffecterStates();
    if (rc != PLDM_SUCCESS)
    {
        std::cerr << "Message setStateEffecterStates to host failure. PLDM "
                     "error code = "
                  << std::hex << std::showbase << rc << "\n";
    }

    // Load json file get Timeout seconds
    parserJsonFile();

    // Start Timer
    using namespace std::chrono;
    auto time = duration_cast<microseconds>(seconds(timeOutSeconds));

    auto r = startTimer(time);
    if (r < 0)
    {
        std::cerr << "Failure to start Host soft off wait timer, ERRNO = " << r
                  << "\n";
    }
    else
    {
        std::cerr
            << "Timer started waiting for host soft off, TIMEOUT_IN_SEC = "
            << timeOutSeconds << "\n";
    }
}

void PldmSoftPowerOff::setHostSoftOffCompleteFlag(
    sdbusplus::message::message& msg)
{

    namespace server = sdbusplus::xyz::openbmc_project::State::server;

    using DbusProperty = std::string;

    using Value =
        std::variant<bool, uint8_t, int16_t, uint16_t, int32_t, uint32_t,
                     int64_t, uint64_t, double, std::string>;

    std::string interface;
    std::map<DbusProperty, Value> properties;

    msg.read(interface, properties);

    if (properties.find("RequestedHostTransition") == properties.end())
    {
        return;
    }

    auto& requestedState =
        std::get<std::string>(properties.at("RequestedHostTransition"));

    if (server::Host::convertTransitionFromString(requestedState) ==
        server::Host::Transition::Off)
    {
        // Disable the timer
        auto r = timer.stop();
        if (r < 0)
        {
            std::cerr << "PLDM soft off: Failure to STOP the timer. ERRNO=" << r
                      << "\n";
        }

        // This marks the completion of pldm soft power off.
        completed = true;
    }
}

int PldmSoftPowerOff::sendChassisOffcommand()
{
    try
    {
        pldm::utils::DBusMapping dbusMapping{"/xyz/openbmc_project/state/host0",
                                             "xyz.openbmc_project.State.Host",
                                             "RequestedHostTransition",
                                             "string"};
        pldm::utils::PropertyValue value =
            std::string("xyz.openbmc_project.State.Host.Transition.Off");

        pldm::utils::DBusHandler().setDbusProperty(dbusMapping, value);
    }
    catch (std::exception& e)
    {

        std::cerr << "Error Setting dbus property,ERROR=" << e.what() << "\n";
        return -1;
    }

    return 0;
}

int PldmSoftPowerOff::guardChassisOff(uint8_t guardState)
{
    std::string service = GUARD_CHASSIS_OFF_TARGET_SERVICE;
    auto p = service.find('@');
    assert(p != std::string::npos);
    if (guardState == ENABLE_GUARD)
    {
        service.insert(p + 1, "enable");
    }
    else if (guardState == DISABLE_GUARD)
    {
        service.insert(p + 1, "disable");
    }
    else
    {
        std::cerr << "Guard Chassis Off argument is wrong. Argument :  "
                  << guardState << "\n";
        return -1;
    }

    auto bus = sdbusplus::bus::new_default();
    try
    {
        auto method = bus.new_method_call(SYSTEMD_BUSNAME, SYSTEMD_PATH,
                                          SYSTEMD_INTERFACE, "StartUnit");
        method.append(service, "replace");

        bus.call(method);
        return 0;
    }
    catch (const SdBusError& e)
    {
        std::cerr << "PLDM soft off: Error staring service,ERROR=" << e.what()
                  << "\n";
        return -1;
    }

    return 0;
}

int PldmSoftPowerOff::setStateEffecterStates()
{
    uint8_t effecterCount = 1;
    std::array<uint8_t,
               sizeof(pldm_msg_hdr) + sizeof(HOST_SOFTOFF_EFFECTER_ID) +
                   sizeof(effecterCount) + sizeof(set_effecter_state_field)>
        requestMsg{};
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    set_effecter_state_field stateField{PLDM_REQUEST_SET, HOST_SOFTOFF_STATE};
    auto rc = encode_set_state_effecter_states_req(
        0, HOST_SOFTOFF_EFFECTER_ID, effecterCount, &stateField, request);
    if (rc != PLDM_SUCCESS)
    {
        std::cerr << "Message encode failure. PLDM error code = " << std::hex
                  << std::showbase << rc << "\n";
        return -1;
    }

    // Open connection to MCTP socket
    int fd = pldm_open();
    if (-1 == fd)
    {
        std::cerr << "Failed to init mctp"
                  << "\n";
        return -1;
    }

    uint8_t* responseMsg = nullptr;
    size_t responseMsgSize{};
    // Send PLDM request msg and wait for response
    rc = pldm_send_recv(HOST_SOFTOFF_MCTP_ID, fd, requestMsg.data(),
                        requestMsg.size(), &responseMsg, &responseMsgSize);
    if (rc < 0)
    {
        std::cerr << "Failed to send message/receive response. RC = " << rc
                  << ", errno = " << errno << "\n";
        return -1;
    }
    pldm_msg* response = reinterpret_cast<pldm_msg*>(responseMsg);
    std::cout << "Done. PLDM RC = " << std::hex << std::showbase
              << static_cast<uint16_t>(response->payload[0]) << std::endl;
    free(responseMsg);

    return 0;
}

int PldmSoftPowerOff::startTimer(const std::chrono::microseconds& usec)
{
    return timer.start(usec);
}

void PldmSoftPowerOff::parserJsonFile()
{
    fs::path dir(timeOutJsonPath);
    if (!fs::exists(dir) || fs::is_empty(dir))
    {
        std::cerr << "PLDM soft off time out JSON does not exist, PATH="
                  << timeOutJsonPath << "\n";
    }

    std::ifstream jsonFilePath(timeOutJsonPath);
    if (!jsonFilePath.is_open())
    {
        std::cerr << "Error opening PLDM soft off time out JSON file, PATH="
                  << timeOutJsonPath << "\n";
    }

    auto data = Json::parse(jsonFilePath);
    if (data.empty())
    {
        std::cerr << "Parsing PLDM soft off time out JSON file failed, FILE="
                  << timeOutJsonPath << "\n";
    }
    else
    {
        timeOutSeconds = data.value("softoff_timeout_secs", 0);
    }
}

} // namespace pldm
