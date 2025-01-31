#include "pldmd/invoker.hpp"

#include <libpldm/base.h>

#include <stdexcept>

#include <gtest/gtest.h>

using namespace pldm;
using namespace pldm::responder;
constexpr Command testCmd = 0xFF;
constexpr Type testType = 0xFF;
constexpr pldm_tid_t tid = 0;

class TestHandler : public CmdHandler
{
  public:
    TestHandler()
    {
        handlers.emplace(testCmd, [this](uint8_t tid, const pldm_msg* request,
                                         size_t payloadLength) {
            return this->handle(tid, request, payloadLength);
        });
    }

    Response handle(uint8_t /*tid*/, const pldm_msg* /*request*/,
                    size_t /*payloadLength*/)
    {
        return {100, 200};
    }
};

TEST(CcOnlyResponse, testEncode)
{
    std::vector<uint8_t> requestMsg(sizeof(pldm_msg_hdr));
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    encode_get_types_req(0, request);

    auto responseMsg = CmdHandler::ccOnlyResponse(request, PLDM_ERROR);
    std::vector<uint8_t> expectMsg = {0, 0, 4, 1};
    EXPECT_EQ(responseMsg, expectMsg);
}

TEST(Registration, testSuccess)
{
    Invoker invoker{};
    invoker.registerHandler(testType, std::make_unique<TestHandler>());
    auto result = invoker.handle(tid, testType, testCmd, nullptr, 0);
    ASSERT_EQ(result[0], 100);
    ASSERT_EQ(result[1], 200);
}

TEST(Registration, testFailure)
{
    Invoker invoker{};
    const Response kExpectedBadTypeResponse = {0x01, 0x02, 0x03,
                                               PLDM_ERROR_INVALID_PLDM_TYPE};
    const Response kExpectedBadCmdResponse = {0x01, 0x02, 0x03,
                                              PLDM_ERROR_UNSUPPORTED_PLDM_CMD};
    const std::array<uint8_t, sizeof(pldm_msg)> kDummyPldmRequestBacking = {
        0x01, 0x02, 0x03};
    const pldm_msg* kDummyPldmRequest =
        reinterpret_cast<const pldm_msg*>(kDummyPldmRequestBacking.data());

    EXPECT_EQ(invoker.handle(tid, testType, testCmd, kDummyPldmRequest, 0),
              kExpectedBadTypeResponse);

    invoker.registerHandler(testType, std::make_unique<TestHandler>());
    uint8_t badCmd = 0xFE;
    EXPECT_EQ(invoker.handle(tid, testType, badCmd, kDummyPldmRequest, 0),
              kExpectedBadCmdResponse);
}
