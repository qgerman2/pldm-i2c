#include <string.h>

#include <array>
#include <cstring>
#include <vector>

#include "libpldm/base.h"
#include "libpldm/utils.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::ElementsAreArray;

constexpr auto hdrSize = sizeof(pldm_msg_hdr);

TEST(PackPLDMMessage, BadPathTest)
{
    struct pldm_header_info hdr;
    struct pldm_header_info* hdr_ptr = NULL;
    pldm_msg_hdr msg{};

    // PLDM header information pointer is NULL
    auto rc = pack_pldm_header(hdr_ptr, &msg);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    // PLDM message pointer is NULL
    rc = pack_pldm_header(&hdr, nullptr);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    // PLDM header information pointer and PLDM message pointer is NULL
    rc = pack_pldm_header(hdr_ptr, nullptr);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    // RESERVED message type
    hdr.msg_type = PLDM_RESERVED;
    rc = pack_pldm_header(&hdr, &msg);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    // Instance ID out of range
    hdr.msg_type = PLDM_REQUEST;
    hdr.instance = 32;
    rc = pack_pldm_header(&hdr, &msg);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    // PLDM type out of range
    hdr.msg_type = PLDM_REQUEST;
    hdr.instance = 31;
    hdr.pldm_type = 64;
    rc = pack_pldm_header(&hdr, &msg);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_PLDM_TYPE);
}

TEST(PackPLDMMessage, RequestMessageGoodPath)
{
    struct pldm_header_info hdr;
    pldm_msg_hdr msg{};

    // Message type is REQUEST and lower range of the field values
    hdr.msg_type = PLDM_REQUEST;
    hdr.instance = 0;
    hdr.pldm_type = 0;
    hdr.command = 0;

    auto rc = pack_pldm_header(&hdr, &msg);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(msg.request, 1);
    EXPECT_EQ(msg.datagram, 0);
    EXPECT_EQ(msg.instance_id, 0);
    EXPECT_EQ(msg.type, 0);
    EXPECT_EQ(msg.command, 0);

    // Message type is REQUEST and upper range of the field values
    hdr.instance = 31;
    hdr.pldm_type = 63;
    hdr.command = 255;

    rc = pack_pldm_header(&hdr, &msg);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(msg.request, 1);
    EXPECT_EQ(msg.datagram, 0);
    EXPECT_EQ(msg.instance_id, 31);
    EXPECT_EQ(msg.type, 63);
    EXPECT_EQ(msg.command, 255);

    // Message type is PLDM_ASYNC_REQUEST_NOTIFY
    hdr.msg_type = PLDM_ASYNC_REQUEST_NOTIFY;

    rc = pack_pldm_header(&hdr, &msg);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(msg.request, 1);
    EXPECT_EQ(msg.datagram, 1);
    EXPECT_EQ(msg.instance_id, 31);
    EXPECT_EQ(msg.type, 63);
    EXPECT_EQ(msg.command, 255);
}

TEST(PackPLDMMessage, ResponseMessageGoodPath)
{
    struct pldm_header_info hdr;
    pldm_msg_hdr msg{};

    // Message type is PLDM_RESPONSE and lower range of the field values
    hdr.msg_type = PLDM_RESPONSE;
    hdr.instance = 0;
    hdr.pldm_type = 0;
    hdr.command = 0;

    auto rc = pack_pldm_header(&hdr, &msg);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(msg.request, 0);
    EXPECT_EQ(msg.datagram, 0);
    EXPECT_EQ(msg.instance_id, 0);
    EXPECT_EQ(msg.type, 0);
    EXPECT_EQ(msg.command, 0);

    // Message type is PLDM_RESPONSE and upper range of the field values
    hdr.instance = 31;
    hdr.pldm_type = 63;
    hdr.command = 255;

    rc = pack_pldm_header(&hdr, &msg);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(msg.request, 0);
    EXPECT_EQ(msg.datagram, 0);
    EXPECT_EQ(msg.instance_id, 31);
    EXPECT_EQ(msg.type, 63);
    EXPECT_EQ(msg.command, 255);
}

TEST(UnpackPLDMMessage, BadPathTest)
{
    struct pldm_header_info hdr;

    // PLDM message pointer is NULL
    auto rc = unpack_pldm_header(nullptr, &hdr);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(UnpackPLDMMessage, RequestMessageGoodPath)
{
    struct pldm_header_info hdr;
    pldm_msg_hdr msg{};

    // Unpack PLDM request message and lower range of field values
    msg.request = 1;
    auto rc = unpack_pldm_header(&msg, &hdr);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(hdr.msg_type, PLDM_REQUEST);
    EXPECT_EQ(hdr.instance, 0);
    EXPECT_EQ(hdr.pldm_type, 0);
    EXPECT_EQ(hdr.command, 0);

    // Unpack PLDM async request message and lower range of field values
    msg.datagram = 1;
    rc = unpack_pldm_header(&msg, &hdr);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(hdr.msg_type, PLDM_ASYNC_REQUEST_NOTIFY);

    // Unpack PLDM request message and upper range of field values
    msg.datagram = 0;
    msg.instance_id = 31;
    msg.type = 63;
    msg.command = 255;
    rc = unpack_pldm_header(&msg, &hdr);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(hdr.msg_type, PLDM_REQUEST);
    EXPECT_EQ(hdr.instance, 31);
    EXPECT_EQ(hdr.pldm_type, 63);
    EXPECT_EQ(hdr.command, 255);
}

TEST(UnpackPLDMMessage, ResponseMessageGoodPath)
{
    struct pldm_header_info hdr;
    pldm_msg_hdr msg{};

    // Unpack PLDM response message and lower range of field values
    auto rc = unpack_pldm_header(&msg, &hdr);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(hdr.msg_type, PLDM_RESPONSE);
    EXPECT_EQ(hdr.instance, 0);
    EXPECT_EQ(hdr.pldm_type, 0);
    EXPECT_EQ(hdr.command, 0);

    // Unpack PLDM response message and upper range of field values
    msg.instance_id = 31;
    msg.type = 63;
    msg.command = 255;
    rc = unpack_pldm_header(&msg, &hdr);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(hdr.msg_type, PLDM_RESPONSE);
    EXPECT_EQ(hdr.instance, 31);
    EXPECT_EQ(hdr.pldm_type, 63);
    EXPECT_EQ(hdr.command, 255);
}

TEST(GetPLDMCommands, testEncodeRequest)
{
    uint8_t pldmType = 0x05;
    ver32_t version{0xFF, 0xFF, 0xFF, 0xFF};
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_GET_COMMANDS_REQ_BYTES>
        requestMsg{};
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    auto rc = encode_get_commands_req(0, pldmType, version, request);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(0, memcmp(request->payload, &pldmType, sizeof(pldmType)));
    EXPECT_EQ(0, memcmp(request->payload + sizeof(pldmType), &version,
                        sizeof(version)));
}

TEST(GetPLDMCommands, testDecodeRequest)
{
    uint8_t pldmType = 0x05;
    ver32_t version{0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t pldmTypeOut{};
    ver32_t versionOut{0xFF, 0xFF, 0xFF, 0xFF};
    std::array<uint8_t, hdrSize + PLDM_GET_COMMANDS_REQ_BYTES> requestMsg{};

    memcpy(requestMsg.data() + hdrSize, &pldmType, sizeof(pldmType));
    memcpy(requestMsg.data() + sizeof(pldmType) + hdrSize, &version,
           sizeof(version));

    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    auto rc = decode_get_commands_req(request, requestMsg.size() - hdrSize,
                                      &pldmTypeOut, &versionOut);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(pldmTypeOut, pldmType);
    EXPECT_EQ(0, memcmp(&versionOut, &version, sizeof(version)));
}

TEST(GetPLDMCommands, testEncodeResponse)
{
    uint8_t completionCode = 0;
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_GET_COMMANDS_RESP_BYTES>
        responseMsg{};
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    std::array<bitfield8_t, PLDM_MAX_CMDS_PER_TYPE / 8> commands{};
    commands[0].byte = 1;
    commands[1].byte = 2;
    commands[2].byte = 3;

    auto rc =
        encode_get_commands_resp(0, PLDM_SUCCESS, commands.data(), response);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    uint8_t* payload_ptr = response->payload;
    EXPECT_EQ(completionCode, payload_ptr[0]);
    EXPECT_EQ(1, payload_ptr[sizeof(completionCode)]);
    EXPECT_EQ(2,
              payload_ptr[sizeof(completionCode) + sizeof(commands[0].byte)]);
    EXPECT_EQ(3, payload_ptr[sizeof(completionCode) + sizeof(commands[0].byte) +
                             sizeof(commands[1].byte)]);
}

TEST(GetPLDMTypes, testEncodeResponse)
{
    uint8_t completionCode = 0;
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_GET_TYPES_RESP_BYTES>
        responseMsg{};
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    std::array<bitfield8_t, PLDM_MAX_TYPES / 8> types{};
    types[0].byte = 1;
    types[1].byte = 2;
    types[2].byte = 3;

    auto rc = encode_get_types_resp(0, PLDM_SUCCESS, types.data(), response);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    uint8_t* payload_ptr = response->payload;
    EXPECT_EQ(completionCode, payload_ptr[0]);
    EXPECT_EQ(1, payload_ptr[sizeof(completionCode)]);
    EXPECT_EQ(2, payload_ptr[sizeof(completionCode) + sizeof(types[0].byte)]);
    EXPECT_EQ(3, payload_ptr[sizeof(completionCode) + sizeof(types[0].byte) +
                             sizeof(types[1].byte)]);
}

TEST(GetPLDMTypes, testGoodDecodeResponse)
{
    std::array<uint8_t, hdrSize + PLDM_GET_TYPES_RESP_BYTES> responseMsg{};
    responseMsg[1 + hdrSize] = 1;
    responseMsg[2 + hdrSize] = 2;
    responseMsg[3 + hdrSize] = 3;
    std::array<bitfield8_t, PLDM_MAX_TYPES / 8> outTypes{};

    uint8_t completion_code;
    responseMsg[hdrSize] = PLDM_SUCCESS;

    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = decode_get_types_resp(response, responseMsg.size() - hdrSize,
                                    &completion_code, outTypes.data());

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(completion_code, PLDM_SUCCESS);
    EXPECT_EQ(responseMsg[1 + hdrSize], outTypes[0].byte);
    EXPECT_EQ(responseMsg[2 + hdrSize], outTypes[1].byte);
    EXPECT_EQ(responseMsg[3 + hdrSize], outTypes[2].byte);
}

TEST(GetPLDMTypes, testBadDecodeResponse)
{
    std::array<uint8_t, hdrSize + PLDM_GET_TYPES_RESP_BYTES> responseMsg{};
    responseMsg[1 + hdrSize] = 1;
    responseMsg[2 + hdrSize] = 2;
    responseMsg[3 + hdrSize] = 3;
    std::array<bitfield8_t, PLDM_MAX_TYPES / 8> outTypes{};

    uint8_t retcompletion_code = 0;
    responseMsg[hdrSize] = PLDM_SUCCESS;

    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = decode_get_types_resp(response, responseMsg.size() - hdrSize - 1,
                                    &retcompletion_code, outTypes.data());

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(GetPLDMCommands, testGoodDecodeResponse)
{
    std::array<uint8_t, hdrSize + PLDM_GET_COMMANDS_RESP_BYTES> responseMsg{};
    responseMsg[1 + hdrSize] = 1;
    responseMsg[2 + hdrSize] = 2;
    responseMsg[3 + hdrSize] = 3;
    std::array<bitfield8_t, PLDM_MAX_CMDS_PER_TYPE / 8> outTypes{};

    uint8_t completion_code;
    responseMsg[hdrSize] = PLDM_SUCCESS;

    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = decode_get_commands_resp(response, responseMsg.size() - hdrSize,
                                       &completion_code, outTypes.data());

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(completion_code, PLDM_SUCCESS);
    EXPECT_EQ(responseMsg[1 + hdrSize], outTypes[0].byte);
    EXPECT_EQ(responseMsg[2 + hdrSize], outTypes[1].byte);
    EXPECT_EQ(responseMsg[3 + hdrSize], outTypes[2].byte);
}

TEST(GetPLDMCommands, testBadDecodeResponse)
{
    std::array<uint8_t, hdrSize + PLDM_GET_COMMANDS_RESP_BYTES> responseMsg{};
    responseMsg[1 + hdrSize] = 1;
    responseMsg[2 + hdrSize] = 2;
    responseMsg[3 + hdrSize] = 3;
    std::array<bitfield8_t, PLDM_MAX_CMDS_PER_TYPE / 8> outTypes{};

    uint8_t retcompletion_code = 0;
    responseMsg[hdrSize] = PLDM_SUCCESS;

    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc =
        decode_get_commands_resp(response, responseMsg.size() - hdrSize - 1,
                                 &retcompletion_code, outTypes.data());

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_LENGTH);
}

TEST(GetPLDMVersion, testGoodEncodeRequest)
{
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_GET_VERSION_REQ_BYTES>
        requestMsg{};
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    uint8_t pldmType = 0x03;
    uint32_t transferHandle = 0x0;
    uint8_t opFlag = 0x01;

    auto rc =
        encode_get_version_req(0, transferHandle, opFlag, pldmType, request);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(
        0, memcmp(request->payload, &transferHandle, sizeof(transferHandle)));
    EXPECT_EQ(0, memcmp(request->payload + sizeof(transferHandle), &opFlag,
                        sizeof(opFlag)));
    EXPECT_EQ(0,
              memcmp(request->payload + sizeof(transferHandle) + sizeof(opFlag),
                     &pldmType, sizeof(pldmType)));
}

TEST(GetPLDMVersion, testBadEncodeRequest)
{
    uint8_t pldmType = 0x03;
    uint32_t transferHandle = 0x0;
    uint8_t opFlag = 0x01;

    auto rc =
        encode_get_version_req(0, transferHandle, opFlag, pldmType, nullptr);

    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}

TEST(GetPLDMVersion, testEncodeResponse)
{
    uint8_t completionCode = 0;
    uint32_t transferHandle = 0;
    uint8_t flag = PLDM_START_AND_END;
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_GET_VERSION_RESP_BYTES>
        responseMsg{};
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    ver32_t version = {0xFF, 0xFF, 0xFF, 0xFF};

    auto rc = encode_get_version_resp(0, PLDM_SUCCESS, 0, PLDM_START_AND_END,
                                      &version, sizeof(ver32_t), response);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(completionCode, response->payload[0]);
    EXPECT_EQ(0, memcmp(response->payload + sizeof(response->payload[0]),
                        &transferHandle, sizeof(transferHandle)));
    EXPECT_EQ(0, memcmp(response->payload + sizeof(response->payload[0]) +
                            sizeof(transferHandle),
                        &flag, sizeof(flag)));
    EXPECT_EQ(0, memcmp(response->payload + sizeof(response->payload[0]) +
                            sizeof(transferHandle) + sizeof(flag),
                        &version, sizeof(version)));
}

TEST(GetPLDMVersion, testDecodeRequest)
{
    std::array<uint8_t, hdrSize + PLDM_GET_VERSION_REQ_BYTES> requestMsg{};
    uint32_t transferHandle = 0x0;
    uint32_t retTransferHandle = 0x0;
    uint8_t flag = PLDM_GET_FIRSTPART;
    uint8_t retFlag = PLDM_GET_FIRSTPART;
    uint8_t pldmType = PLDM_BASE;
    uint8_t retType = PLDM_BASE;

    memcpy(requestMsg.data() + hdrSize, &transferHandle,
           sizeof(transferHandle));
    memcpy(requestMsg.data() + sizeof(transferHandle) + hdrSize, &flag,
           sizeof(flag));
    memcpy(requestMsg.data() + sizeof(transferHandle) + sizeof(flag) + hdrSize,
           &pldmType, sizeof(pldmType));

    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    auto rc = decode_get_version_req(request, requestMsg.size() - hdrSize,
                                     &retTransferHandle, &retFlag, &retType);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(transferHandle, retTransferHandle);
    EXPECT_EQ(flag, retFlag);
    EXPECT_EQ(pldmType, retType);
}

TEST(GetPLDMVersion, testDecodeResponse)
{
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_GET_VERSION_RESP_BYTES>
        responseMsg{};
    uint32_t transferHandle = 0x0;
    uint32_t retTransferHandle = 0x0;
    uint8_t flag = PLDM_START_AND_END;
    uint8_t retFlag = PLDM_START_AND_END;
    uint8_t completionCode = 0;
    ver32_t version = {0xFF, 0xFF, 0xFF, 0xFF};
    ver32_t versionOut;
    uint8_t completion_code;

    memcpy(responseMsg.data() + sizeof(completionCode) + hdrSize,
           &transferHandle, sizeof(transferHandle));
    memcpy(responseMsg.data() + sizeof(completionCode) +
               sizeof(transferHandle) + hdrSize,
           &flag, sizeof(flag));
    memcpy(responseMsg.data() + sizeof(completionCode) +
               sizeof(transferHandle) + sizeof(flag) + hdrSize,
           &version, sizeof(version));

    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = decode_get_version_resp(response, responseMsg.size() - hdrSize,
                                      &completion_code, &retTransferHandle,
                                      &retFlag, &versionOut);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(transferHandle, retTransferHandle);
    EXPECT_EQ(flag, retFlag);

    EXPECT_EQ(versionOut.major, version.major);
    EXPECT_EQ(versionOut.minor, version.minor);
    EXPECT_EQ(versionOut.update, version.update);
    EXPECT_EQ(versionOut.alpha, version.alpha);
}

TEST(GetTID, testEncodeRequest)
{
    pldm_msg request{};

    auto rc = encode_get_tid_req(0, &request);
    ASSERT_EQ(rc, PLDM_SUCCESS);
}

TEST(GetTID, testEncodeResponse)
{
    uint8_t completionCode = 0;
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_GET_TID_RESP_BYTES>
        responseMsg{};
    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());
    uint8_t tid = 1;

    auto rc = encode_get_tid_resp(0, PLDM_SUCCESS, tid, response);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    uint8_t* payload = response->payload;
    EXPECT_EQ(completionCode, payload[0]);
    EXPECT_EQ(1, payload[sizeof(completionCode)]);
}

TEST(GetTID, testDecodeResponse)
{
    std::array<uint8_t, hdrSize + PLDM_GET_TID_RESP_BYTES> responseMsg{};
    responseMsg[1 + hdrSize] = 1;

    uint8_t tid;
    uint8_t completion_code;
    responseMsg[hdrSize] = PLDM_SUCCESS;

    auto response = reinterpret_cast<pldm_msg*>(responseMsg.data());

    auto rc = decode_get_tid_resp(response, responseMsg.size() - hdrSize,
                                  &completion_code, &tid);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(completion_code, PLDM_SUCCESS);
    EXPECT_EQ(tid, 1);
}

TEST(MultipartReceive, testEncodeRequestPass)
{
    constexpr size_t max_msg_len = PLDM_MULTIPART_TRANSFER_MIN_SIZE;
    constexpr uint8_t instance_id = 0x01;
    constexpr uint8_t pldm_type = PLDM_BASE;
    constexpr uint8_t opflag = PLDM_XFER_FIRST_PART;
    constexpr uint32_t next_transfer_handle = 0x0;
    constexpr uint32_t section_offset = 0x0;
    constexpr uint32_t section_length = 0x10;
    constexpr uint32_t transfer_ctx = 0x01;
    std::vector<uint8_t> msg(max_msg_len);
    pldm_msg* req_msg = reinterpret_cast<pldm_msg*>(msg.data());

    int rc = encode_multipart_receive_req(
        instance_id, pldm_type, opflag, transfer_ctx, next_transfer_handle,
        section_offset, section_length, req_msg);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    pldm_multipart_receive_req* req_pkt =
        reinterpret_cast<pldm_multipart_receive_req*>(req_msg->payload);

    EXPECT_EQ(req_pkt->pldm_type, pldm_type);
    EXPECT_EQ(req_pkt->transfer_opflag, opflag);
    EXPECT_EQ(req_pkt->transfer_handle, next_transfer_handle);
    EXPECT_EQ(req_pkt->transfer_ctx, transfer_ctx);
    EXPECT_EQ(req_pkt->section_offset, section_offset);
    EXPECT_EQ(req_pkt->section_length, section_length);
}

TEST(MultipartReceive, testEncodeRequestFailBadParams)
{
    EXPECT_EQ(encode_multipart_receive_req(0, 0, 0, 0, 0, 0, 0, NULL),
              PLDM_ERROR_INVALID_DATA);
}

TEST(MultipartReceive, testEncodeRequestFailBadPldmType)
{
    constexpr size_t max_msg_len = PLDM_MULTIPART_TRANSFER_MIN_SIZE;
    constexpr uint8_t instance_id = 0x01;
    constexpr uint8_t pldm_type = 0xff;
    constexpr uint8_t opflag = PLDM_XFER_FIRST_PART;
    constexpr uint32_t next_transfer_handle = 0x0;
    constexpr uint32_t section_offset = 0x0;
    constexpr uint32_t section_length = 0x10;
    constexpr uint32_t transfer_ctx = 0x01;
    std::vector<uint8_t> msg(max_msg_len);
    pldm_msg* req_msg = reinterpret_cast<pldm_msg*>(msg.data());

    EXPECT_EQ(encode_multipart_receive_req(instance_id, pldm_type, opflag,
                                           transfer_ctx, next_transfer_handle,
                                           section_offset, section_length,
                                           req_msg),
              PLDM_ERROR_INVALID_PLDM_TYPE);
}

TEST(MultipartReceive, testEncodeRequestFailBadSectionOffset)
{
    constexpr size_t max_msg_len = PLDM_MULTIPART_TRANSFER_MIN_SIZE;
    constexpr uint8_t instance_id = 0x01;
    constexpr uint8_t pldm_type = PLDM_BASE;
    constexpr uint8_t opflag = PLDM_XFER_NEXT_PART;
    constexpr uint32_t next_transfer_handle = 0x01;
    constexpr uint32_t section_offset = 0x0;
    constexpr uint32_t section_length = 0x10;
    constexpr uint32_t transfer_ctx = 0x01;
    std::vector<uint8_t> msg(max_msg_len);
    pldm_msg* req_msg = reinterpret_cast<pldm_msg*>(msg.data());

    EXPECT_EQ(encode_multipart_receive_req(instance_id, pldm_type, opflag,
                                           transfer_ctx, next_transfer_handle,
                                           section_offset, section_length,
                                           req_msg),
              PLDM_ERROR_INVALID_DATA);
}

TEST(MultipartReceive, testEncodeRequestFailBadXferFlag)
{
    constexpr size_t max_msg_len = PLDM_MULTIPART_TRANSFER_MIN_SIZE;
    constexpr uint8_t instance_id = 0x01;
    constexpr uint8_t pldm_type = PLDM_BASE;
    constexpr uint8_t opflag = 0xff;
    constexpr uint32_t next_transfer_handle = 0x0;
    constexpr uint32_t section_offset = 0x0;
    constexpr uint32_t section_length = 0x10;
    constexpr uint32_t transfer_ctx = 0x01;
    std::vector<uint8_t> msg(max_msg_len);
    pldm_msg* req_msg = reinterpret_cast<pldm_msg*>(msg.data());

    EXPECT_EQ(encode_multipart_receive_req(instance_id, pldm_type, opflag,
                                           transfer_ctx, next_transfer_handle,
                                           section_offset, section_length,
                                           req_msg),
              PLDM_INVALID_TRANSFER_OPERATION_FLAG);
}

TEST(MultipartReceive, testEncodeRequestFailBadHandle)
{
    constexpr size_t max_msg_len = PLDM_MULTIPART_TRANSFER_MIN_SIZE;
    constexpr uint8_t instance_id = 0x01;
    constexpr uint8_t pldm_type = PLDM_BASE;
    constexpr uint8_t opflag = PLDM_XFER_NEXT_PART;
    constexpr uint32_t next_transfer_handle = 0x0;
    constexpr uint32_t section_offset = 0x100;
    constexpr uint32_t section_length = 0x10;
    constexpr uint32_t transfer_ctx = 0x01;
    std::vector<uint8_t> msg(max_msg_len);
    pldm_msg* req_msg = reinterpret_cast<pldm_msg*>(msg.data());

    EXPECT_EQ(encode_multipart_receive_req(instance_id, pldm_type, opflag,
                                           transfer_ctx, next_transfer_handle,
                                           section_offset, section_length,
                                           req_msg),
              PLDM_ERROR_INVALID_DATA);
}

TEST(MultipartReceive, testDecodeRequestPass)
{
    constexpr uint8_t kPldmType = PLDM_BASE;
    constexpr uint8_t kFlag = PLDM_XFER_FIRST_PART;
    constexpr uint32_t kTransferCtx = 0x01;
    constexpr uint32_t kTransferHandle = 0x10;
    constexpr uint32_t kSectionOffset = 0x0;
    constexpr uint32_t kSectionLength = 0x10;
    uint8_t pldm_type = 0x0;
    uint8_t flag = PLDM_GET_FIRSTPART;
    uint32_t transfer_ctx;
    uint32_t transfer_handle;
    uint32_t section_offset;
    uint32_t section_length;

    // Header values don't matter for this test.
    pldm_msg_hdr hdr{};
    // Assign values to the packet struct and memcpy to ensure correct byte
    // ordering.
    pldm_multipart_receive_req req_pkt = {
        .pldm_type = kPldmType,
        .transfer_opflag = kFlag,
        .transfer_ctx = kTransferCtx,
        .transfer_handle = kTransferHandle,
        .section_offset = kSectionOffset,
        .section_length = kSectionLength,
    };
    std::vector<uint8_t> req(sizeof(hdr) + PLDM_MULTIPART_RECEIVE_REQ_BYTES);
    std::memcpy(req.data(), &hdr, sizeof(hdr));
    std::memcpy(req.data() + sizeof(hdr), &req_pkt, sizeof(req_pkt));

    pldm_msg* pldm_request = reinterpret_cast<pldm_msg*>(req.data());
    int rc = decode_multipart_receive_req(
        pldm_request, req.size() - hdrSize, &pldm_type, &flag, &transfer_ctx,
        &transfer_handle, &section_offset, &section_length);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(pldm_type, kPldmType);
    EXPECT_EQ(flag, kFlag);
    EXPECT_EQ(transfer_ctx, kTransferCtx);
    EXPECT_EQ(transfer_handle, kTransferHandle);
    EXPECT_EQ(section_offset, kSectionOffset);
    EXPECT_EQ(section_length, kSectionLength);
}

TEST(MultipartReceive, testDecodeRequestFailNullData)
{
    EXPECT_EQ(decode_multipart_receive_req(NULL, 0, NULL, NULL, NULL, NULL,
                                           NULL, NULL),
              PLDM_ERROR_INVALID_DATA);
}

TEST(MultipartReceive, testDecodeRequestFailBadLength)
{
    constexpr uint8_t kPldmType = PLDM_BASE;
    constexpr uint8_t kFlag = PLDM_XFER_FIRST_PART;
    uint8_t pldm_type;
    uint8_t flag;
    uint32_t transfer_ctx;
    uint32_t transfer_handle;
    uint32_t section_offset;
    uint32_t section_length;

    // Header values don't matter for this test.
    pldm_msg_hdr hdr{};
    // Assign values to the packet struct and memcpy to ensure correct byte
    // ordering.
    pldm_multipart_receive_req req_pkt = {
        .pldm_type = kPldmType,
        .transfer_opflag = kFlag,
    };
    std::vector<uint8_t> req(sizeof(hdr) + PLDM_MULTIPART_RECEIVE_REQ_BYTES);
    std::memcpy(req.data(), &hdr, sizeof(hdr));
    std::memcpy(req.data() + sizeof(hdr), &req_pkt, sizeof(req_pkt));

    pldm_msg* pldm_request = reinterpret_cast<pldm_msg*>(req.data());
    EXPECT_EQ(decode_multipart_receive_req(
                  pldm_request, (req.size() - hdrSize) + 1, &pldm_type, &flag,
                  &transfer_ctx, &transfer_handle, &section_offset,
                  &section_length),
              PLDM_ERROR_INVALID_LENGTH);
}

TEST(MultipartReceive, testDecodeRequestFailBadPldmType)
{
    constexpr uint8_t kPldmType = 0xff;
    constexpr uint8_t kFlag = PLDM_XFER_FIRST_PART;
    uint8_t pldm_type;
    uint8_t flag;
    uint32_t transfer_ctx;
    uint32_t transfer_handle;
    uint32_t section_offset;
    uint32_t section_length;

    // Header values don't matter for this test.
    pldm_msg_hdr hdr{};
    // Assign values to the packet struct and memcpy to ensure correct byte
    // ordering.
    pldm_multipart_receive_req req_pkt = {
        .pldm_type = kPldmType,
        .transfer_opflag = kFlag,
    };
    std::vector<uint8_t> req(sizeof(hdr) + PLDM_MULTIPART_RECEIVE_REQ_BYTES);
    std::memcpy(req.data(), &hdr, sizeof(hdr));
    std::memcpy(req.data() + sizeof(hdr), &req_pkt, sizeof(req_pkt));

    pldm_msg* pldm_request = reinterpret_cast<pldm_msg*>(req.data());
    EXPECT_EQ(decode_multipart_receive_req(pldm_request, req.size() - hdrSize,
                                           &pldm_type, &flag, &transfer_ctx,
                                           &transfer_handle, &section_offset,
                                           &section_length),
              PLDM_ERROR_INVALID_PLDM_TYPE);
}

TEST(MultipartReceive, testDecodeRequestFailBadTransferFlag)
{
    constexpr uint8_t kPldmType = PLDM_BASE;
    constexpr uint8_t kFlag = PLDM_XFER_CURRENT_PART + 0x10;
    uint8_t pldm_type;
    uint8_t flag;
    uint32_t transfer_ctx;
    uint32_t transfer_handle;
    uint32_t section_offset;
    uint32_t section_length;

    // Header values don't matter for this test.
    pldm_msg_hdr hdr{};
    // Assign values to the packet struct and memcpy to ensure correct byte
    // ordering.
    pldm_multipart_receive_req req_pkt = {
        .pldm_type = kPldmType,
        .transfer_opflag = kFlag,
    };
    std::vector<uint8_t> req(sizeof(hdr) + PLDM_MULTIPART_RECEIVE_REQ_BYTES);
    std::memcpy(req.data(), &hdr, sizeof(hdr));
    std::memcpy(req.data() + sizeof(hdr), &req_pkt, sizeof(req_pkt));

    pldm_msg* pldm_request = reinterpret_cast<pldm_msg*>(req.data());
    EXPECT_EQ(decode_multipart_receive_req(pldm_request, req.size() - hdrSize,
                                           &pldm_type, &flag, &transfer_ctx,
                                           &transfer_handle, &section_offset,
                                           &section_length),
              PLDM_INVALID_TRANSFER_OPERATION_FLAG);
}

TEST(MultipartReceive, testDecodeRequestFailBadOffset)
{
    constexpr uint8_t kPldmType = PLDM_BASE;
    constexpr uint8_t kFlag = PLDM_XFER_NEXT_PART;
    constexpr uint32_t kSectionOffset = 0x0;
    uint8_t pldm_type;
    uint8_t flag;
    uint32_t transfer_ctx;
    uint32_t transfer_handle;
    uint32_t section_offset;
    uint32_t section_length;

    // Header values don't matter for this test.
    pldm_msg_hdr hdr{};
    // Assign values to the packet struct and memcpy to ensure correct byte
    // ordering.
    pldm_multipart_receive_req req_pkt = {
        .pldm_type = kPldmType,
        .transfer_opflag = kFlag,
        .section_offset = kSectionOffset,
    };
    std::vector<uint8_t> req(sizeof(hdr) + PLDM_MULTIPART_RECEIVE_REQ_BYTES);
    std::memcpy(req.data(), &hdr, sizeof(hdr));
    std::memcpy(req.data() + sizeof(hdr), &req_pkt, sizeof(req_pkt));

    pldm_msg* pldm_request = reinterpret_cast<pldm_msg*>(req.data());
    EXPECT_EQ(decode_multipart_receive_req(pldm_request, req.size() - hdrSize,
                                           &pldm_type, &flag, &transfer_ctx,
                                           &transfer_handle, &section_offset,
                                           &section_length),
              PLDM_ERROR_INVALID_DATA);
}

TEST(MultipartReceive, testDecodeRequestFailBadHandle)
{
    constexpr uint8_t kPldmType = PLDM_BASE;
    constexpr uint8_t kFlag = PLDM_XFER_NEXT_PART;
    constexpr uint32_t kSectionOffset = 0x100;
    constexpr uint32_t kTransferHandle = 0x0;
    uint8_t pldm_type;
    uint8_t flag;
    uint32_t transfer_ctx;
    uint32_t transfer_handle;
    uint32_t section_offset;
    uint32_t section_length;

    // Header values don't matter for this test.
    pldm_msg_hdr hdr{};
    // Assign values to the packet struct and memcpy to ensure correct byte
    // ordering.
    pldm_multipart_receive_req req_pkt = {
        .pldm_type = kPldmType,
        .transfer_opflag = kFlag,
        .transfer_handle = kTransferHandle,
        .section_offset = kSectionOffset,
    };
    std::vector<uint8_t> req(sizeof(hdr) + PLDM_MULTIPART_RECEIVE_REQ_BYTES);
    std::memcpy(req.data(), &hdr, sizeof(hdr));
    std::memcpy(req.data() + sizeof(hdr), &req_pkt, sizeof(req_pkt));

    pldm_msg* pldm_request = reinterpret_cast<pldm_msg*>(req.data());
    EXPECT_EQ(decode_multipart_receive_req(pldm_request, req.size() - hdrSize,
                                           &pldm_type, &flag, &transfer_ctx,
                                           &transfer_handle, &section_offset,
                                           &section_length),
              PLDM_ERROR_INVALID_DATA);
}

TEST(MultipartReceive, testEncodeResponsePass)
{
    constexpr size_t max_msg_len = 256;
    constexpr uint8_t instance_id = 0x01;
    constexpr uint8_t completion_code = PLDM_SUCCESS;
    constexpr uint8_t type = PLDM_BASE;
    constexpr uint8_t flag = PLDM_GET_FIRSTPART;
    constexpr uint32_t next_transfer_handle = 0x0;
    constexpr uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05,
                                0x06, 0x07, 0x08, 0x09};
    constexpr uint32_t data_length = sizeof(data);
    uint32_t crc = crc32(data, data_length);

    std::vector<uint8_t> msg(max_msg_len);
    pldm_msg* resp_msg = reinterpret_cast<pldm_msg*>(msg.data());
    int rc = encode_multipart_receive_resp(instance_id, completion_code, type,
                                           flag, next_transfer_handle,
                                           data_length, data, crc, resp_msg);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    pldm_multipart_receive_resp* resp_pkt =
        reinterpret_cast<pldm_multipart_receive_resp*>(resp_msg->payload);

    EXPECT_EQ(resp_pkt->completion_code, completion_code);
    EXPECT_EQ(resp_pkt->transfer_opflag, flag);
    EXPECT_EQ(resp_pkt->next_transfer_handle, next_transfer_handle);
    EXPECT_EQ(resp_pkt->data_length, data_length);
    EXPECT_EQ(std::memcmp(resp_pkt->data, data, data_length), 0);
    // CRC32 is part of the last 4 bytes.
    EXPECT_EQ(std::memcmp(&resp_pkt->data[data_length], &crc, sizeof(crc)), 0);
}

TEST(MultipartReceive, testEncodeResponseFailBadParams)
{
    EXPECT_EQ(encode_multipart_receive_resp(0, 0, 0, 0, 0, 0, NULL, 0, NULL),
              PLDM_ERROR_INVALID_DATA);
}

TEST(MultipartReceive, testEncodeResponseFailBadType)
{
    constexpr size_t max_msg_len = 256;
    constexpr uint8_t instance_id = 0x01;
    constexpr uint8_t completion_code = PLDM_SUCCESS;
    constexpr uint8_t type = 0xff;
    constexpr uint8_t flag = PLDM_GET_FIRSTPART;
    constexpr uint32_t next_transfer_handle = 0x0;
    constexpr uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05,
                                0x06, 0x07, 0x08, 0x09};
    constexpr uint32_t data_length = sizeof(data);
    uint32_t crc = crc32(data, data_length);

    std::vector<uint8_t> msg(max_msg_len);
    pldm_msg* resp_msg = reinterpret_cast<pldm_msg*>(msg.data());
    EXPECT_EQ(encode_multipart_receive_resp(instance_id, completion_code, type,
                                            flag, next_transfer_handle,
                                            data_length, data, crc, resp_msg),
              PLDM_ERROR_INVALID_PLDM_TYPE);
}

TEST(MultipartReceive, testEncodeResponseFailBadTransferFlag)
{
    constexpr size_t max_msg_len = 256;
    constexpr uint8_t instance_id = 0x01;
    constexpr uint8_t completion_code = PLDM_SUCCESS;
    constexpr uint8_t type = PLDM_BASE;
    constexpr uint8_t flag = 0xff;
    constexpr uint32_t next_transfer_handle = 0x0;
    constexpr uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05,
                                0x06, 0x07, 0x08, 0x09};
    constexpr uint32_t data_length = sizeof(data);
    uint32_t crc = crc32(data, data_length);

    std::vector<uint8_t> msg(max_msg_len);
    pldm_msg* resp_msg = reinterpret_cast<pldm_msg*>(msg.data());
    EXPECT_EQ(encode_multipart_receive_resp(instance_id, completion_code, type,
                                            flag, next_transfer_handle,
                                            data_length, data, crc, resp_msg),
              PLDM_INVALID_TRANSFER_OPERATION_FLAG);
}

TEST(NegotiateTransferParameters, testEncodeRequestPass)
{
    constexpr uint8_t instance_id = 0x01;
    constexpr uint16_t requester_part_size = 0x1000;
    constexpr bitfield8_t requester_protocol_support[8] = {
        {.byte = 0x0c},
    };

    std::vector<uint8_t> msg(sizeof(pldm_msg_hdr) +
                             PLDM_NEGOTIATE_TRANSFER_PARAMETERS_REQ_BYTES);
    pldm_msg* resp_msg = reinterpret_cast<pldm_msg*>(msg.data());

    int rc = encode_negotiate_transfer_parameters_req(
        instance_id, requester_part_size, requester_protocol_support, resp_msg);
    EXPECT_EQ(rc, PLDM_SUCCESS);
    pldm_negotiate_transfer_parameters_req* resp_pkt =
        reinterpret_cast<pldm_negotiate_transfer_parameters_req*>(
            resp_msg->payload);

    EXPECT_EQ(resp_pkt->part_size, requester_part_size);
    EXPECT_EQ(std::memcmp(resp_pkt->protocol_support,
                          requester_protocol_support,
                          sizeof(requester_protocol_support)),
              0);
}

TEST(NegotiateTransferParameters, testEncodeRequestFailBadParams)
{
    EXPECT_EQ(encode_negotiate_transfer_parameters_req(0, 0, NULL, NULL),
              PLDM_ERROR_INVALID_DATA);
}

TEST(NegotiateTransferParameters, testEncodeRequestFailBadXferLen)
{
    constexpr uint8_t instance_id = 0x01;
    constexpr uint16_t requester_part_size =
        PLDM_MULTIPART_TRANSFER_MIN_SIZE - 1;
    constexpr bitfield8_t requester_protocol_support[8] = {
        {.byte = 0x0c},
    };

    std::vector<uint8_t> msg(sizeof(pldm_msg_hdr) +
                             PLDM_NEGOTIATE_TRANSFER_PARAMETERS_REQ_BYTES);
    pldm_msg* resp_msg = reinterpret_cast<pldm_msg*>(msg.data());

    EXPECT_EQ(encode_negotiate_transfer_parameters_req(
                  instance_id, requester_part_size, requester_protocol_support,
                  resp_msg),
              PLDM_ERROR_UNSUPPORTED_PLDM_CMD);
}

TEST(NegotiateTransferParameters, testDecodeRequestPass)
{
    constexpr uint16_t kRequesterPartSize = 0x1000;
    constexpr bitfield8_t kRequesterProtocolSupport[8] = {
        {.byte = 0x0c},
    };
    uint16_t requester_part_size;
    bitfield8_t requester_protocol_support[8];

    // Header values don't matter for this test.
    pldm_msg_hdr hdr{};
    // Assign values to the packet struct and memcpy to ensure correct byte
    // ordering.
    pldm_negotiate_transfer_parameters_req req_pkt{};
    req_pkt.part_size = kRequesterPartSize;
    std::memcpy(req_pkt.protocol_support, kRequesterProtocolSupport,
                sizeof(req_pkt.protocol_support));
    std::vector<uint8_t> req(sizeof(hdr) +
                             PLDM_NEGOTIATE_TRANSFER_PARAMETERS_REQ_BYTES);
    std::memcpy(req.data(), &hdr, sizeof(hdr));
    std::memcpy(req.data() + sizeof(hdr), &req_pkt, sizeof(req_pkt));

    pldm_msg* pldm_request = reinterpret_cast<pldm_msg*>(req.data());
    int rc = decode_negotiate_transfer_parameters_req(
        pldm_request, req.size() - hdrSize, &requester_part_size,
        requester_protocol_support);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(requester_part_size, kRequesterPartSize);
    EXPECT_EQ(std::memcmp(requester_protocol_support, kRequesterProtocolSupport,
                          sizeof(requester_protocol_support)),
              0);
}

TEST(NegotiateTransferParameters, testDecodeRequestFailNullParams)
{
    EXPECT_EQ(decode_negotiate_transfer_parameters_req(NULL, 0, NULL, NULL),
              PLDM_ERROR_INVALID_DATA);
}

TEST(NegotiateTransferParameters, testDecodeRequestFailBadXferLen)
{
    constexpr uint16_t kRequesterPartSize =
        PLDM_MULTIPART_TRANSFER_MIN_SIZE - 1;
    uint16_t requester_part_size;
    bitfield8_t requester_protocol_support[8];

    // Header values don't matter for this test.
    pldm_msg_hdr hdr{};
    // Assign values to the packet struct and memcpy to ensure correct byte
    // ordering.
    pldm_negotiate_transfer_parameters_req req_pkt{};
    req_pkt.part_size = kRequesterPartSize;
    std::vector<uint8_t> req(sizeof(hdr) +
                             PLDM_NEGOTIATE_TRANSFER_PARAMETERS_REQ_BYTES);
    std::memcpy(req.data(), &hdr, sizeof(hdr));
    std::memcpy(req.data() + sizeof(hdr), &req_pkt, sizeof(req_pkt));

    pldm_msg* pldm_request = reinterpret_cast<pldm_msg*>(req.data());
    EXPECT_EQ(decode_negotiate_transfer_parameters_req(
                  pldm_request, req.size() - hdrSize, &requester_part_size,
                  requester_protocol_support),
              PLDM_ERROR_UNSUPPORTED_PLDM_CMD);
}

TEST(NegotiateTransferParameters, testEncodeResponsePass)
{
    constexpr uint8_t instance_id = 0x01;
    constexpr uint8_t completion_code = PLDM_SUCCESS;
    constexpr uint16_t requester_part_size = 0x1000;
    constexpr bitfield8_t requester_protocol_support[8] = {
        {.byte = 0x0c},
    };

    std::vector<uint8_t> msg(sizeof(pldm_msg_hdr) +
                             PLDM_NEGOTIATE_TRANSFER_PARAMETERS_RESP_BYTES);
    pldm_msg* resp_msg = reinterpret_cast<pldm_msg*>(msg.data());
    int rc = encode_negotiate_transfer_parameters_resp(
        instance_id, completion_code, requester_part_size,
        requester_protocol_support, resp_msg);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    pldm_negotiate_transfer_parameters_resp* resp_pkt =
        reinterpret_cast<pldm_negotiate_transfer_parameters_resp*>(
            resp_msg->payload);
    EXPECT_EQ(resp_pkt->completion_code, completion_code);
    EXPECT_EQ(resp_pkt->part_size, requester_part_size);
    EXPECT_EQ(std::memcmp(resp_pkt->protocol_support,
                          requester_protocol_support,
                          sizeof(requester_protocol_support)),
              0);
}

TEST(NegotiateTransferParameters, testEncodeResponseFailBadParams)
{
    EXPECT_EQ(encode_negotiate_transfer_parameters_resp(0, 0, 0, NULL, NULL),
              PLDM_ERROR_INVALID_DATA);
}

TEST(NegotiateTransferParameters, testEncodeResponseFailBadXferLen)
{
    constexpr uint8_t instance_id = 0x01;
    constexpr uint8_t completion_code = PLDM_SUCCESS;
    constexpr uint16_t requester_part_size =
        PLDM_MULTIPART_TRANSFER_MIN_SIZE - 1;
    constexpr bitfield8_t requester_protocol_support[8] = {
        {.byte = 0x0c},
    };

    std::vector<uint8_t> msg(sizeof(pldm_msg_hdr) +
                             PLDM_NEGOTIATE_TRANSFER_PARAMETERS_RESP_BYTES);
    pldm_msg* resp_msg = reinterpret_cast<pldm_msg*>(msg.data());
    EXPECT_EQ(encode_negotiate_transfer_parameters_resp(
                  instance_id, completion_code, requester_part_size,
                  requester_protocol_support, resp_msg),
              PLDM_ERROR_UNSUPPORTED_PLDM_CMD);
}

TEST(NegotiateTransferParameters, testDecodeResponsePass)
{
    constexpr uint8_t kCompletionCode = PLDM_SUCCESS;
    constexpr uint16_t kResponderPartSize = 0x1000;
    constexpr bitfield8_t kResponderProtocolSupport[8] = {
        {.byte = 0x0c},
    };
    uint16_t responder_part_size;
    uint8_t completion_code;
    bitfield8_t responder_protocol_support[8];

    // Header values don't matter for this test.
    pldm_msg_hdr hdr{};
    // Assign values to the packet struct and memcpy to ensure correct byte
    // ordering.
    pldm_negotiate_transfer_parameters_resp resp_pkt{};
    resp_pkt.part_size = kResponderPartSize;
    resp_pkt.completion_code = kCompletionCode;
    std::memcpy(resp_pkt.protocol_support, kResponderProtocolSupport,
                sizeof(resp_pkt.protocol_support));
    std::vector<uint8_t> resp(sizeof(hdr) +
                              PLDM_NEGOTIATE_TRANSFER_PARAMETERS_RESP_BYTES);
    std::memcpy(resp.data(), &hdr, sizeof(hdr));
    std::memcpy(resp.data() + sizeof(hdr), &resp_pkt, sizeof(resp_pkt));

    pldm_msg* pldm_response = reinterpret_cast<pldm_msg*>(resp.data());
    int rc = decode_negotiate_transfer_parameters_resp(
        pldm_response, resp.size() - hdrSize, &completion_code,
        &responder_part_size, responder_protocol_support);

    EXPECT_EQ(rc, PLDM_SUCCESS);
    EXPECT_EQ(completion_code, kCompletionCode);
    EXPECT_EQ(responder_part_size, kResponderPartSize);
    EXPECT_EQ(std::memcmp(responder_protocol_support, kResponderProtocolSupport,
                          sizeof(responder_protocol_support)),
              0);
}

TEST(NegotiateTransferParameters, testDecodeResponseFailBadParams)
{
    EXPECT_EQ(decode_negotiate_transfer_parameters_resp(NULL, 0, 0, 0, NULL),
              PLDM_ERROR_INVALID_DATA);
}

TEST(NegotiateTransferParameters, testDecodeResponseFailBadXferLen)
{
    constexpr uint8_t kCompletionCode = PLDM_SUCCESS;
    constexpr uint16_t kResponderPartSize =
        PLDM_MULTIPART_TRANSFER_MIN_SIZE - 1;
    constexpr bitfield8_t kResponderProtocolSupport[8] = {
        {.byte = 0x0c},
    };
    uint16_t responder_part_size;
    uint8_t completion_code;
    bitfield8_t responder_protocol_support[8];

    // Header values don't matter for this test.
    pldm_msg_hdr hdr{};
    // Assign values to the packet struct and memcpy to ensure correct byte
    // ordering.
    pldm_negotiate_transfer_parameters_resp resp_pkt{};
    resp_pkt.part_size = kResponderPartSize;
    resp_pkt.completion_code = kCompletionCode;
    std::memcpy(resp_pkt.protocol_support, kResponderProtocolSupport,
                sizeof(resp_pkt.protocol_support));
    std::vector<uint8_t> resp(sizeof(hdr) +
                              PLDM_NEGOTIATE_TRANSFER_PARAMETERS_RESP_BYTES);
    std::memcpy(resp.data(), &hdr, sizeof(hdr));
    std::memcpy(resp.data() + sizeof(hdr), &resp_pkt, sizeof(resp_pkt));

    pldm_msg* pldm_response = reinterpret_cast<pldm_msg*>(resp.data());
    EXPECT_EQ(decode_negotiate_transfer_parameters_resp(
                  pldm_response, resp.size() - hdrSize, &completion_code,
                  &responder_part_size, responder_protocol_support),
              PLDM_ERROR_UNSUPPORTED_PLDM_CMD);
}

TEST(CcOnlyResponse, testEncode)
{
    struct pldm_msg responseMsg;

    auto rc =
        encode_cc_only_resp(0 /*instance id*/, 1 /*pldm type*/, 2 /*command*/,
                            3 /*complection code*/, &responseMsg);
    EXPECT_EQ(rc, PLDM_SUCCESS);

    auto p = reinterpret_cast<uint8_t*>(&responseMsg);
    EXPECT_THAT(std::vector<uint8_t>(p, p + sizeof(responseMsg)),
                ElementsAreArray({0, 1, 2, 3}));

    rc = encode_cc_only_resp(PLDM_INSTANCE_MAX + 1, 1, 2, 3, &responseMsg);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);

    rc = encode_cc_only_resp(0, 1, 2, 3, nullptr);
    EXPECT_EQ(rc, PLDM_ERROR_INVALID_DATA);
}
