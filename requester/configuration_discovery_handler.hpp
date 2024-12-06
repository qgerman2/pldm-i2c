#pragma once

#include "common/types.hpp"
#include "common/utils.hpp"
#include "mctp_endpoint_discovery.hpp"

#include <expected>
#include <stdexcept>

namespace pldm
{

struct MCTPEndpoint : public std::tuple<eid, NetworkId, std::string>
{
    using std::tuple<eid, NetworkId, std::string>::tuple;
    static std::expected<MCTPEndpoint, std::string> create(
        eid eidValue, NetworkId networkId, const utils::PropertyMap& properties)
    {
        if (!properties.contains("Name"))
        {
            return std::unexpected("Missing name property");
        }
        auto name = std::get<std::string>(properties.at("Name"));
        return MCTPEndpoint(eidValue, networkId, name);
    }
    eid endpointId() const
    {
        return std::get<eid>(*this);
    }
    NetworkId networkId() const
    {
        return std::get<NetworkId>(*this);
    }
    const std::string& name() const
    {
        return std::get<std::string>(*this);
    }
};

class ConfigurationDiscoveryHandler : public MctpDiscoveryHandlerIntf
{
  public:
    ConfigurationDiscoveryHandler() = delete;
    ConfigurationDiscoveryHandler(const ConfigurationDiscoveryHandler&) =
        delete;
    ConfigurationDiscoveryHandler(ConfigurationDiscoveryHandler&&) = delete;
    ConfigurationDiscoveryHandler& operator=(
        const ConfigurationDiscoveryHandler&) = delete;
    ConfigurationDiscoveryHandler& operator=(ConfigurationDiscoveryHandler&&) =
        delete;
    ~ConfigurationDiscoveryHandler() = default;

    explicit ConfigurationDiscoveryHandler(
        const pldm::utils::DBusHandler* dBusIntf) : dBusIntf(dBusIntf)
    {}

    /** @brief Discover configuration associated with the new MCTP endpoints.
     *
     *  @param[in] newMctpInfos - information of discovered MCTP endpoint
     */
    void handleMctpEndpoints(const MctpInfos& newMctpInfos);

    /** @brief Remove configuration associated with the removed MCTP endpoints.
     *
     *  @param[in] removedMctpInfos - the removed MCTP endpoints
     */
    void handleRemovedMctpEndpoints(const MctpInfos& removedMctpInfos);

    /** @brief Helper function to invoke registered handlers for
     *  updating the availability status of the MCTP endpoint
     *
     *  @param[in] mctpInfo - information of the target endpoint
     *  @param[in] availability - new availability status
     */
    void updateMctpEndpointAvailability(const MctpInfo& mctpinfo,
                                        Availability availability);

    /** @brief Get existing configurations.
     *
     *  @return The configurations.
     */
    const std::map<std::string, MCTPEndpoint>& getConfigurations() const;

  private:
    /** @brief Search for associated configuration for the MctpInfo.
     *
     *  @param[in] mctpInfo - information of discovered MCTP endpoint
     */
    void searchConfigurationFor(MctpInfo mctpInfo);

    /** @brief Remove configuration associated with the removed MCTP endpoint.
     *
     *  @param[in] eid - endpoint Id of the removed MCTP Endpoint
     */
    void removeConfigByEid(uint8_t eidToRemove);

    /** @brief The configuration contains Dbus path and the MCTP endpoint
     * information.
     */
    std::map<std::string /*configDbusPath*/, MCTPEndpoint> configurations;

    /** @brief D-Bus Interface object*/
    const pldm::utils::DBusHandler* dBusIntf;
};

} // namespace pldm
