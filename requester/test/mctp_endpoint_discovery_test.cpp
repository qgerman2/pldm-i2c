#include "common/utils.hpp"
#include "requester/test/mock_mctp_discovery_handler_intf.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::_;

TEST(MctpEndpointDiscoveryTest, OnehandleMCTPEndpoint)
{
    auto& bus = pldm::utils::DBusHandler::getBus();
    pldm::MockManager manager;
    auto mctpDiscoveryHandler = std::make_unique<pldm::MctpDiscovery>(
        bus, std::initializer_list<pldm::MctpDiscoveryHandlerIntf*>{&manager});

    EXPECT_CALL(manager, handleMCTPEndpoints(_)).Times(1);
    mctpDiscoveryHandler->loadStaticEndpoints("./static_eid_table.json");
}

TEST(MctpEndpointDiscoveryTest, MultipleHandleMCTPEndpoints)
{
    auto& bus = pldm::utils::DBusHandler::getBus();
    pldm::MockManager manager1;
    pldm::MockManager manager2;
    auto mctpDiscoveryHandler = std::make_unique<pldm::MctpDiscovery>(
        bus, std::initializer_list<pldm::MctpDiscoveryHandlerIntf*>{
                 &manager1, &manager2});

    EXPECT_CALL(manager1, handleMCTPEndpoints(_)).Times(1);
    EXPECT_CALL(manager2, handleMCTPEndpoints(_)).Times(1);
    mctpDiscoveryHandler->loadStaticEndpoints("./static_eid_table.json");
}