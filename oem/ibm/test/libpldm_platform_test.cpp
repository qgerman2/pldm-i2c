#include <string.h>

#include <array>

#include "oem/ibm/libpldm/platform.h"

#include <gtest/gtest.h>

constexpr auto hdrSize = sizeof(pldm_msg_hdr);

TEST(GetAlertStatus, testGoodEncodeRequest)
{
    std::array<uint8_t, hdrSize + PLDM_GET_ALERT_STATUS_REQ_BYTES> requestMsg{};

    uint8_t versionId = 0x0;

    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    auto rc = encode_get_alert_status_req(0, versionId, request,
                                          PLDM_GET_ALERT_STATUS_REQ_BYTES);
    EXPECT_EQ(rc, PLDM_SUCCESS);

    struct pldm_get_alert_status_req* req =
        reinterpret_cast<struct pldm_get_alert_status_req*>(request->payload);

    EXPECT_EQ(versionId, req->version);
}

TEST(GetAlertStatus, testBadEncodeRequest)
{
    std::array<uint8_t, hdrSize + PLDM_GET_ALERT_STATUS_REQ_BYTES> requestMsg{};

    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    auto rc = encode_get_alert_status_req(0, 0x0, request,
                                          PLDM_GET_ALERT_STATUS_REQ_BYTES + 1);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(GetAlertStatus, testGoodDecodeResponse)
{
    uint8_t completionCode = PLDM_SUCCESS;
    uint32_t rack_entry = 0xFF000030;
    uint32_t pri_cec_node = 0x00008030;
    std::array<uint8_t, hdrSize + PLDM_GET_ALERT_STATUS_RESP_BYTES>
        responseMsg{};

    uint8_t retCompletionCode = 0;
    uint32_t retRack_entry = 0;
    uint32_t retPri_cec_node = 0;

    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    struct pldm_get_alert_status_resp* resp =
        reinterpret_cast<struct pldm_get_alert_status_resp*>(response->payload);
    resp->completion_code = completionCode;
    resp->rack_entry = htole32(rack_entry);
    resp->pri_cec_node = htole32(pri_cec_node);

    auto rc = decode_get_alert_status_resp(
        response, responseMsg.size() - hdrSize, &retCompletionCode,
        &retRack_entry, &retPri_cec_node);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(retCompletionCode, completionCode);
    EXPECT_EQ(retRack_entry, rack_entry);
    EXPECT_EQ(retPri_cec_node, pri_cec_node);
}

TEST(GetAlertStatus, testBadDecodeResponse)
{
    uint8_t completionCode = PLDM_SUCCESS;
    uint32_t rack_entry = 0xFF000030;
    uint32_t pri_cec_node = 0x00008030;
    std::array<uint8_t, hdrSize + PLDM_GET_ALERT_STATUS_RESP_BYTES>
        responseMsg{};

    uint8_t retCompletionCode = 0;
    uint32_t retRack_entry = 0;
    uint32_t retPri_cec_node = 0;

    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    struct pldm_get_alert_status_resp* resp =
        reinterpret_cast<struct pldm_get_alert_status_resp*>(response->payload);
    resp->completion_code = completionCode;
    resp->rack_entry = htole32(rack_entry);
    resp->pri_cec_node = htole32(pri_cec_node);

    auto rc = decode_get_alert_status_resp(
        response, responseMsg.size() - hdrSize + 1, &retCompletionCode,
        &retRack_entry, &retPri_cec_node);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}
