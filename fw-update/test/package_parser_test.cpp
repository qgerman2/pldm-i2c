#include "fw-update/package_parser.hpp"

#include <typeinfo>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace pldm::fw_update;

TEST(PackageParser, ValidPkgSingleDescriptorSingleComponent)
{
    std::vector<uint8_t> fwPkgHdr{
        0xF0, 0x18, 0x87, 0x8C, 0xCB, 0x7D, 0x49, 0x43, 0x98, 0x00, 0xA0, 0x2F,
        0x05, 0x9A, 0xCA, 0x02, 0x01, 0x8B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x19, 0x0C, 0xE5, 0x07, 0x00, 0x08, 0x00, 0x01, 0x0E,
        0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x53, 0x74, 0x72, 0x69, 0x6E,
        0x67, 0x31, 0x01, 0x2E, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0E,
        0x00, 0x00, 0x01, 0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x53, 0x74,
        0x72, 0x69, 0x6E, 0x67, 0x32, 0x02, 0x00, 0x10, 0x00, 0x16, 0x20, 0x23,
        0xC9, 0x3E, 0xC5, 0x41, 0x15, 0x95, 0xF4, 0x48, 0x70, 0x1D, 0x49, 0xD6,
        0x75, 0x01, 0x00, 0x0A, 0x00, 0x64, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
        0x00, 0x00, 0x00, 0x8B, 0x00, 0x00, 0x00, 0x1B, 0x00, 0x00, 0x00, 0x01,
        0x0E, 0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x53, 0x74, 0x72, 0x69,
        0x6E, 0x67, 0x33, 0x4F, 0x96, 0xAE, 0x56};

    constexpr uintmax_t pkgSize = 166;
    constexpr std::string_view pkgVersion{"VersionString1"};
    auto pkgHeaderInfo =
        reinterpret_cast<const pldm_package_header_information*>(
            fwPkgHdr.data());
    EXPECT_EQ(pkgHeaderInfo->package_header_size, fwPkgHdr.size());

    auto parser = std::make_unique<PackageParserV1>();
    auto obj = parser.get();
    EXPECT_EQ(typeid(*obj).name(), typeid(PackageParserV1).name());

    parser->parse(fwPkgHdr, pkgSize);
    EXPECT_EQ(parser->pkgHeaderSize, fwPkgHdr.size());
    EXPECT_EQ(parser->pkgVersion, pkgVersion);

    auto outfwDeviceIDRecords = parser->getFwDeviceIDRecords();
    FirmwareDeviceIDRecords fwDeviceIDRecords{
        {1,
         {0},
         "VersionString2",
         {{PLDM_FWUP_UUID,
           std::vector<uint8_t>{0x16, 0x20, 0x23, 0xC9, 0x3E, 0xC5, 0x41, 0x15,
                                0x95, 0xF4, 0x48, 0x70, 0x1D, 0x49, 0xD6,
                                0x75}}},
         {}},
    };
    EXPECT_EQ(outfwDeviceIDRecords, fwDeviceIDRecords);

    auto outCompImageInfos = parser->getComponentImageInfos();
    ComponentImageInfos compImageInfos{
        {10, 100, 0xFFFFFFFF, 0, 0, 139, 27, "VersionString3"}};
    EXPECT_EQ(outCompImageInfos, compImageInfos);
}

TEST(PackageParser, ValidPkgMultipleDescriptorsMultipleComponents)
{
    std::vector<uint8_t> fwPkgHdr{
        0xF0, 0x18, 0x87, 0x8C, 0xCB, 0x7D, 0x49, 0x43, 0x98, 0x00, 0xA0, 0x2F,
        0x05, 0x9A, 0xCA, 0x02, 0x01, 0x46, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x19, 0x0C, 0xE5, 0x07, 0x00, 0x08, 0x00, 0x01, 0x0E,
        0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x53, 0x74, 0x72, 0x69, 0x6E,
        0x67, 0x31, 0x03, 0x45, 0x00, 0x03, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0E,
        0x00, 0x00, 0x03, 0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x53, 0x74,
        0x72, 0x69, 0x6E, 0x67, 0x32, 0x02, 0x00, 0x10, 0x00, 0x12, 0x44, 0xD2,
        0x64, 0x8D, 0x7D, 0x47, 0x18, 0xA0, 0x30, 0xFC, 0x8A, 0x56, 0x58, 0x7D,
        0x5B, 0x01, 0x00, 0x04, 0x00, 0x47, 0x16, 0x00, 0x00, 0xFF, 0xFF, 0x0B,
        0x00, 0x01, 0x07, 0x4F, 0x70, 0x65, 0x6E, 0x42, 0x4D, 0x43, 0x12, 0x34,
        0x2E, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0E, 0x00, 0x00, 0x07,
        0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x53, 0x74, 0x72, 0x69, 0x6E,
        0x67, 0x33, 0x02, 0x00, 0x10, 0x00, 0x12, 0x44, 0xD2, 0x64, 0x8D, 0x7D,
        0x47, 0x18, 0xA0, 0x30, 0xFC, 0x8A, 0x56, 0x58, 0x7D, 0x5C, 0x2E, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0E, 0x00, 0x00, 0x01, 0x56, 0x65,
        0x72, 0x73, 0x69, 0x6F, 0x6E, 0x53, 0x74, 0x72, 0x69, 0x6E, 0x67, 0x34,
        0x02, 0x00, 0x10, 0x00, 0x12, 0x44, 0xD2, 0x64, 0x8D, 0x7D, 0x47, 0x18,
        0xA0, 0x30, 0xFC, 0x8A, 0x56, 0x58, 0x7D, 0x5D, 0x03, 0x00, 0x0A, 0x00,
        0x64, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x46, 0x01,
        0x00, 0x00, 0x1B, 0x00, 0x00, 0x00, 0x01, 0x0E, 0x56, 0x65, 0x72, 0x73,
        0x69, 0x6F, 0x6E, 0x53, 0x74, 0x72, 0x69, 0x6E, 0x67, 0x35, 0x0A, 0x00,
        0xC8, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x01, 0x00, 0x61, 0x01,
        0x00, 0x00, 0x1B, 0x00, 0x00, 0x00, 0x01, 0x0E, 0x56, 0x65, 0x72, 0x73,
        0x69, 0x6F, 0x6E, 0x53, 0x74, 0x72, 0x69, 0x6E, 0x67, 0x36, 0x10, 0x00,
        0x2C, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x0C, 0x00, 0x7C, 0x01,
        0x00, 0x00, 0x1B, 0x00, 0x00, 0x00, 0x01, 0x0E, 0x56, 0x65, 0x72, 0x73,
        0x69, 0x6F, 0x6E, 0x53, 0x74, 0x72, 0x69, 0x6E, 0x67, 0x37, 0xF1, 0x90,
        0x9C, 0x71};

    constexpr uintmax_t pkgSize = 407;
    constexpr std::string_view pkgVersion{"VersionString1"};
    auto pkgHeaderInfo = reinterpret_cast<const pldm_package_header_information*>(
        fwPkgHdr.data());
    EXPECT_EQ(pkgHeaderInfo->package_header_size, fwPkgHdr.size());

    auto parser = std::make_unique<PackageParserV1>();
    auto obj = parser.get();
    EXPECT_EQ(typeid(*obj).name(), typeid(PackageParserV1).name());
    EXPECT_EQ(parser->pkgHeaderSize, fwPkgHdr.size());
    EXPECT_EQ(parser->pkgVersion, pkgVersion);

    parser->parse(fwPkgHdr, pkgSize);
    auto outfwDeviceIDRecords = parser->getFwDeviceIDRecords();
    FirmwareDeviceIDRecords fwDeviceIDRecords{
        {1,
         {0, 1},
         "VersionString2",
         {{PLDM_FWUP_UUID,
           std::vector<uint8_t>{0x12, 0x44, 0xD2, 0x64, 0x8D, 0x7D, 0x47, 0x18,
                                0xA0, 0x30, 0xFC, 0x8A, 0x56, 0x58, 0x7D,
                                0x5B}},
          {PLDM_FWUP_IANA_ENTERPRISE_ID,
           std::vector<uint8_t>{0x47, 0x16, 0x00, 0x00}},
          {PLDM_FWUP_VENDOR_DEFINED,
           std::make_tuple("OpenBMC", std::vector<uint8_t>{0x12, 0x34})}},
         {}},
        {0,
         {0, 1, 2},
         "VersionString3",
         {{PLDM_FWUP_UUID,
           std::vector<uint8_t>{0x12, 0x44, 0xD2, 0x64, 0x8D, 0x7D, 0x47, 0x18,
                                0xA0, 0x30, 0xFC, 0x8A, 0x56, 0x58, 0x7D,
                                0x5C}}},
         {}},
        {0,
         {0},
         "VersionString4",
         {{PLDM_FWUP_UUID,
           std::vector<uint8_t>{0x12, 0x44, 0xD2, 0x64, 0x8D, 0x7D, 0x47, 0x18,
                                0xA0, 0x30, 0xFC, 0x8A, 0x56, 0x58, 0x7D,
                                0x5D}}},
         {}},
    };
    EXPECT_EQ(outfwDeviceIDRecords, fwDeviceIDRecords);

    auto outCompImageInfos = parser->getComponentImageInfos();
    ComponentImageInfos compImageInfos{
        {10, 100, 0xFFFFFFFF, 0, 0, 326, 27, "VersionString5"},
        {10, 200, 0xFFFFFFFF, 0, 1, 353, 27, "VersionString6"},
        {16, 300, 0xFFFFFFFF, 1, 12, 380, 27, "VersionString7"}};
    EXPECT_EQ(outCompImageInfos, compImageInfos);
}

TEST(PackageParser, InvalidPkgBadChecksum)
{
    std::vector<uint8_t> fwPkgHdr{
        0xF0, 0x18, 0x87, 0x8C, 0xCB, 0x7D, 0x49, 0x43, 0x98, 0x00, 0xA0, 0x2F,
        0x05, 0x9A, 0xCA, 0x02, 0x01, 0x8B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x19, 0x0C, 0xE5, 0x07, 0x00, 0x08, 0x00, 0x01, 0x0E,
        0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x53, 0x74, 0x72, 0x69, 0x6E,
        0x67, 0x31, 0x01, 0x2E, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0E,
        0x00, 0x00, 0x01, 0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x53, 0x74,
        0x72, 0x69, 0x6E, 0x67, 0x32, 0x02, 0x00, 0x10, 0x00, 0x16, 0x20, 0x23,
        0xC9, 0x3E, 0xC5, 0x41, 0x15, 0x95, 0xF4, 0x48, 0x70, 0x1D, 0x49, 0xD6,
        0x75, 0x01, 0x00, 0x0A, 0x00, 0x64, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
        0x00, 0x00, 0x00, 0x8B, 0x00, 0x00, 0x00, 0x1B, 0x00, 0x00, 0x00, 0x01,
        0x0E, 0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x53, 0x74, 0x72, 0x69,
        0x6E, 0x67, 0x33, 0x4F, 0x96, 0xAE, 0x57};

    constexpr uintmax_t pkgSize = 166;
    constexpr std::string_view pkgVersion{"VersionString1"};
    auto pkgHeaderInfo =
        reinterpret_cast<const pldm_package_header_information*>(
            fwPkgHdr.data());
    EXPECT_EQ(pkgHeaderInfo->package_header_size, fwPkgHdr.size());

    auto parser = std::make_unique<PackageParserV1>();
    auto obj = parser.get();
    EXPECT_EQ(typeid(*obj).name(), typeid(PackageParserV1).name());

    EXPECT_THROW(parser->parse(fwPkgHdr, pkgSize), std::exception);
}
