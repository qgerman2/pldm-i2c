#include "libpldmresponder/bios.hpp"
#include "libpldmresponder/bios_parser.hpp"
#include "libpldmresponder/bios_table.hpp"

#include <string.h>

#include <array>
#include <ctime>

#include "libpldm/base.h"
#include "libpldm/bios.h"

#include <gtest/gtest.h>

using namespace pldm::responder;
using namespace pldm::responder::utils;
using namespace bios_parser;
using namespace bios_parser::bios_enum;

TEST(epochToBCDTime, testTime)
{
    struct tm time
    {
    };
    time.tm_year = 119;
    time.tm_mon = 3;
    time.tm_mday = 13;
    time.tm_hour = 5;
    time.tm_min = 18;
    time.tm_sec = 13;
    time.tm_isdst = -1;

    time_t epochTime = mktime(&time);
    uint8_t seconds = 0;
    uint8_t minutes = 0;
    uint8_t hours = 0;
    uint8_t day = 0;
    uint8_t month = 0;
    uint16_t year = 0;

    epochToBCDTime(epochTime, seconds, minutes, hours, day, month, year);

    ASSERT_EQ(0x13, seconds);
    ASSERT_EQ(0x18, minutes);
    ASSERT_EQ(0x5, hours);
    ASSERT_EQ(0x13, day);
    ASSERT_EQ(0x4, month);
    ASSERT_EQ(0x2019, year);
}

TEST(GetBIOSStrings, allScenarios)
{
    using namespace bios_parser;
    // All the BIOS Strings in the BIOS JSON config files.
    Strings vec{"HMCManagedState",  "On",         "Off",
                "FWBootSide",       "Perm",       "Temp",
                "CodeUpdatePolicy", "Concurrent", "Disruptive"};

    Strings nullVec{};

    // Invalid directory
    auto strings = bios_parser::getStrings("./bios_json");
    ASSERT_EQ(strings == nullVec, true);

    strings = bios_parser::getStrings("./bios_jsons");
    ASSERT_EQ(strings == vec, true);
}

TEST(getAttrValue, allScenarios)
{
    using namespace bios_parser::bios_enum;
    // All the BIOS Strings in the BIOS JSON config files.
    AttrValuesMap valueMap{
        {"HMCManagedState", {false, {"On", "Off"}, {"On"}}},
        {"FWBootSide", {false, {"Perm", "Temp"}, {"Perm"}}},
        {"CodeUpdatePolicy",
         {false, {"Concurrent", "Disruptive"}, {"Concurrent"}}}};

    auto values = getValues("./bios_jsons");
    ASSERT_EQ(valueMap == values, true);

    CurrentValues cv{"Concurrent"};
    auto value = getAttrValue("CodeUpdatePolicy");
    ASSERT_EQ(value == cv, true);

    // Invalid attribute name
    ASSERT_THROW(getAttrValue("CodeUpdatePolic"), std::out_of_range);
}

TEST(GetBIOSStringTable, testGoodRequest)
{
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_GET_BIOS_TABLE_REQ_BYTES>
        requestPayload{};
    auto request = reinterpret_cast<pldm_msg*>(requestPayload.data());
    struct pldm_get_bios_table_req* req =
        (struct pldm_get_bios_table_req*)request->payload;
    req->transfer_handle = 9;
    req->transfer_op_flag = PLDM_GET_FIRSTPART;
    req->table_type = PLDM_BIOS_STRING_TABLE;

    Strings biosStrings = getStrings("./bios_jsons");
    std::sort(biosStrings.begin(), biosStrings.end());
    biosStrings.erase(std::unique(biosStrings.begin(), biosStrings.end()),
                      biosStrings.end());

    size_t requestPayloadLength = requestPayload.size() - sizeof(pldm_msg_hdr);
    std::string jsonFilePath("./bios_jsons");
    std::string biosFilePath("./bios");
    auto response = buildBIOSTables(request, requestPayloadLength,
                                    jsonFilePath.c_str(), biosFilePath.c_str());
    auto responsePtr = reinterpret_cast<pldm_msg*>(response.data());

    struct pldm_get_bios_table_resp* resp =
        reinterpret_cast<struct pldm_get_bios_table_resp*>(
            responsePtr->payload);

    ASSERT_EQ(0, resp->completion_code);
    ASSERT_EQ(0, resp->next_transfer_handle);
    ASSERT_EQ(PLDM_START_AND_END, resp->transfer_flag);

    uint16_t strLen = 0;
    uint8_t* tableData = reinterpret_cast<uint8_t*>(resp->table_data);

    for (auto elem : biosStrings)
    {
        struct pldm_bios_string_table_entry* ptr =
            reinterpret_cast<struct pldm_bios_string_table_entry*>(tableData);
        strLen = ptr->string_length;
        ASSERT_EQ(strLen, elem.size());
        ASSERT_EQ(0, memcmp(elem.c_str(), ptr->name, strLen));
        tableData += (sizeof(struct pldm_bios_string_table_entry) - 1) + strLen;

    } // end for
}

TEST(GetBIOSTable, testBadRequest)
{
    std::array<uint8_t, sizeof(pldm_msg_hdr) + PLDM_GET_BIOS_TABLE_REQ_BYTES>
        requestPayload{};
    auto request = reinterpret_cast<pldm_msg*>(requestPayload.data());
    struct pldm_get_bios_table_req* req =
        (struct pldm_get_bios_table_req*)request->payload;
    req->transfer_handle = 9;
    req->transfer_op_flag = PLDM_GET_FIRSTPART;
    req->table_type = 0xFF;

    size_t requestPayloadLength = requestPayload.size() - sizeof(pldm_msg_hdr);

    std::string jsonFilePath("./bios_jsons");
    std::string biosFilePath("./bios");
    auto response = buildBIOSTables(request, requestPayloadLength,
                                    jsonFilePath.c_str(), biosFilePath.c_str());
    auto responsePtr = reinterpret_cast<pldm_msg*>(response.data());

    struct pldm_get_bios_table_resp* resp =
        reinterpret_cast<struct pldm_get_bios_table_resp*>(
            responsePtr->payload);

    ASSERT_EQ(PLDM_INVALID_BIOS_TABLE_TYPE, resp->completion_code);
}
