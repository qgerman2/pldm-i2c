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

TEST(GetAlertStatus, testGoodEncodeResponse)
{
    uint8_t completionCode = 0;
    uint32_t rack_entry = 0xFF000030;
    uint32_t pri_cec_node = 0x00008030;

    std::vector<uint8_t> responseMsg(hdrSize +
                                     PLDM_GET_ALERT_STATUS_RESP_BYTES);
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc =
        encode_get_alert_status_resp(0, PLDM_SUCCESS, rack_entry, pri_cec_node,
                                     response, responseMsg.size() - hdrSize);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    struct pldm_get_alert_status_resp* resp =
        reinterpret_cast<struct pldm_get_alert_status_resp*>(response->payload);

    EXPECT_EQ(completionCode, resp->completion_code);
    EXPECT_EQ(rack_entry, le32toh(resp->rack_entry));
    EXPECT_EQ(pri_cec_node, le32toh(resp->pri_cec_node));
}

TEST(GetAlertStatus, testBadEncodeResponse)
{
    uint32_t rack_entry = 0xFF000030;
    uint32_t pri_cec_node = 0x00008030;

    std::vector<uint8_t> responseMsg(hdrSize +
                                     PLDM_GET_ALERT_STATUS_RESP_BYTES);
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = encode_get_alert_status_resp(0, PLDM_SUCCESS, rack_entry,
                                           pri_cec_node, response,
                                           responseMsg.size() - hdrSize + 1);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(GetAlertStatus, testGoodDecodeRequest)
{
    std::array<uint8_t, hdrSize + PLDM_GET_ALERT_STATUS_REQ_BYTES> requestMsg{};

    uint8_t versionId = 0x0;
    uint8_t retVersionId;

    auto req = reinterpret_cast<pldm_msg*>(requestMsg.data());
    struct pldm_get_alert_status_req* request =
        reinterpret_cast<struct pldm_get_alert_status_req*>(req->payload);

    request->version = versionId;

    auto rc = decode_get_alert_status_req(req, requestMsg.size() - hdrSize,
                                          &retVersionId);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(retVersionId, versionId);
}

TEST(GetAlertStatus, testBadDecodeRequest)
{
    std::array<uint8_t, hdrSize + PLDM_GET_ALERT_STATUS_REQ_BYTES> requestMsg{};

    uint8_t versionId = 0x0;
    uint8_t retVersionId;

    auto req = reinterpret_cast<pldm_msg*>(requestMsg.data());
    struct pldm_get_alert_status_req* request =
        reinterpret_cast<struct pldm_get_alert_status_req*>(req->payload);

    request->version = versionId;

    auto rc = decode_get_alert_status_req(req, requestMsg.size() - hdrSize + 1,
                                          &retVersionId);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}
