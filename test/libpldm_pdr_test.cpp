#include <array>

#include "libpldm/pdr.h"
#include "libpldm/platform.h"

#include <gtest/gtest.h>

TEST(PDRAccess, testInit)
{
    auto repo = pldm_pdr_init();
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 0);
    EXPECT_EQ(pldm_pdr_get_repo_size(repo), 0);
    pldm_pdr_destroy(&repo);
    EXPECT_EQ(repo, nullptr);
}

TEST(PDRUpdate, testAdd)
{
    auto repo = pldm_pdr_init();

    std::array<uint8_t, 10> data{};
    auto handle = pldm_pdr_add(repo, data.data(), data.size(), 0);
    EXPECT_EQ(handle, 1);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 1);
    EXPECT_EQ(pldm_pdr_get_repo_size(repo), data.size());

    handle = pldm_pdr_add(repo, data.data(), data.size(), 0);
    EXPECT_EQ(handle, 2);
    handle = pldm_pdr_add(repo, data.data(), data.size(), 0);
    EXPECT_EQ(handle, 3);
    handle = pldm_pdr_add(repo, data.data(), data.size(), 0xdeeddeed);
    EXPECT_EQ(handle, 0xdeeddeed);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 4);
    EXPECT_EQ(pldm_pdr_get_repo_size(repo), data.size() * 4);

    pldm_pdr_destroy(&repo);
}

TEST(PDRAccess, testGet)
{
    auto repo = pldm_pdr_init();

    std::array<uint32_t, 10> in{100, 345, 3, 6, 89, 0, 11, 45, 23434, 123123};
    pldm_pdr_add(repo, reinterpret_cast<uint8_t*>(in.data()), sizeof(in), 1);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 1);
    EXPECT_EQ(pldm_pdr_get_repo_size(repo), sizeof(in));
    uint32_t size{};
    uint32_t nextRecHdl{};
    uint8_t* outData = nullptr;
    auto hdl = pldm_pdr_get_record(repo, 0, &outData, &size, &nextRecHdl);
    EXPECT_NE(hdl, nullptr);
    EXPECT_EQ(size, sizeof(in));
    EXPECT_EQ(nextRecHdl, 0);
    EXPECT_EQ(memcmp(outData, in.data(), sizeof(in)), 0);
    outData = nullptr;

    auto hdl2 = pldm_pdr_get_record(repo, 1, &outData, &size, &nextRecHdl);
    EXPECT_EQ(hdl, hdl2);
    EXPECT_NE(hdl, nullptr);
    EXPECT_EQ(size, sizeof(in));
    EXPECT_EQ(nextRecHdl, 0);
    EXPECT_EQ(memcmp(outData, in.data(), sizeof(in)), 0);
    outData = nullptr;

    hdl = pldm_pdr_get_record(repo, 0xdeaddead, &outData, &size, &nextRecHdl);
    EXPECT_EQ(hdl, nullptr);
    EXPECT_EQ(size, 0);
    EXPECT_EQ(nextRecHdl, 0);
    EXPECT_EQ(outData, nullptr);
    outData = nullptr;

    std::array<uint32_t, 10> in2{1000, 3450, 30,  60,     890,
                                 0,    110,  450, 234034, 123123};
    pldm_pdr_add(repo, reinterpret_cast<uint8_t*>(in2.data()), sizeof(in2), 2);
    pldm_pdr_add(repo, reinterpret_cast<uint8_t*>(in2.data()), sizeof(in2), 3);
    pldm_pdr_add(repo, reinterpret_cast<uint8_t*>(in2.data()), sizeof(in2), 4);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 4);
    EXPECT_EQ(pldm_pdr_get_repo_size(repo), sizeof(in2) * 4);
    hdl = pldm_pdr_get_record(repo, 0, &outData, &size, &nextRecHdl);
    EXPECT_NE(hdl, nullptr);
    EXPECT_EQ(size, sizeof(in));
    EXPECT_EQ(nextRecHdl, 2);
    EXPECT_EQ(memcmp(outData, in.data(), sizeof(in)), 0);
    outData = nullptr;
    hdl2 = pldm_pdr_get_record(repo, 1, &outData, &size, &nextRecHdl);
    EXPECT_EQ(hdl, hdl2);
    EXPECT_NE(hdl, nullptr);
    EXPECT_EQ(size, sizeof(in));
    EXPECT_EQ(nextRecHdl, 2);
    EXPECT_EQ(memcmp(outData, in.data(), sizeof(in)), 0);
    outData = nullptr;
    hdl = pldm_pdr_get_record(repo, 2, &outData, &size, &nextRecHdl);
    EXPECT_NE(hdl, nullptr);
    EXPECT_EQ(size, sizeof(in2));
    EXPECT_EQ(nextRecHdl, 3);
    EXPECT_EQ(memcmp(outData, in2.data(), sizeof(in2)), 0);
    outData = nullptr;
    hdl = pldm_pdr_get_record(repo, 3, &outData, &size, &nextRecHdl);
    EXPECT_NE(hdl, nullptr);
    EXPECT_EQ(size, sizeof(in2));
    EXPECT_EQ(nextRecHdl, 4);
    EXPECT_EQ(memcmp(outData, in2.data(), sizeof(in2)), 0);
    outData = nullptr;
    hdl = pldm_pdr_get_record(repo, 4, &outData, &size, &nextRecHdl);
    EXPECT_NE(hdl, nullptr);
    EXPECT_EQ(size, sizeof(in2));
    EXPECT_EQ(nextRecHdl, 0);
    EXPECT_EQ(memcmp(outData, in2.data(), sizeof(in2)), 0);
    outData = nullptr;
}

TEST(PDRAccess, testGetNext)
{
    auto repo = pldm_pdr_init();

    std::array<uint32_t, 10> in{100, 345, 3, 6, 89, 0, 11, 45, 23434, 123123};
    pldm_pdr_add(repo, reinterpret_cast<uint8_t*>(in.data()), sizeof(in), 1);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 1);
    EXPECT_EQ(pldm_pdr_get_repo_size(repo), sizeof(in));
    uint32_t size{};
    uint32_t nextRecHdl{};
    uint8_t* outData = nullptr;
    auto hdl = pldm_pdr_get_record(repo, 0, &outData, &size, &nextRecHdl);
    EXPECT_NE(hdl, nullptr);
    EXPECT_EQ(size, sizeof(in));
    EXPECT_EQ(nextRecHdl, 0);
    EXPECT_EQ(memcmp(outData, in.data(), sizeof(in)), 0);
    outData = nullptr;

    std::array<uint32_t, 10> in2{1000, 3450, 30,  60,     890,
                                 0,    110,  450, 234034, 123123};
    pldm_pdr_add(repo, reinterpret_cast<uint8_t*>(in2.data()), sizeof(in2), 2);
    pldm_pdr_add(repo, reinterpret_cast<uint8_t*>(in2.data()), sizeof(in2), 3);
    pldm_pdr_add(repo, reinterpret_cast<uint8_t*>(in2.data()), sizeof(in2), 4);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 4);
    EXPECT_EQ(pldm_pdr_get_repo_size(repo), sizeof(in2) * 4);
    hdl = pldm_pdr_get_next_record(repo, hdl, &outData, &size, &nextRecHdl);
    EXPECT_NE(hdl, nullptr);
    EXPECT_EQ(size, sizeof(in2));
    EXPECT_EQ(nextRecHdl, 3);
    EXPECT_EQ(memcmp(outData, in2.data(), sizeof(in2)), 0);
    outData = nullptr;
    hdl = pldm_pdr_get_next_record(repo, hdl, &outData, &size, &nextRecHdl);
    EXPECT_NE(hdl, nullptr);
    EXPECT_EQ(size, sizeof(in2));
    EXPECT_EQ(nextRecHdl, 4);
    EXPECT_EQ(memcmp(outData, in2.data(), sizeof(in2)), 0);
    outData = nullptr;
    hdl = pldm_pdr_get_next_record(repo, hdl, &outData, &size, &nextRecHdl);
    EXPECT_NE(hdl, nullptr);
    EXPECT_EQ(size, sizeof(in2));
    EXPECT_EQ(nextRecHdl, 0);
    EXPECT_EQ(memcmp(outData, in2.data(), sizeof(in2)), 0);
    outData = nullptr;
}

TEST(PDRAccess, testFindByType)
{
    auto repo = pldm_pdr_init();

    std::array<uint8_t, sizeof(pldm_pdr_hdr)> data{};
    pldm_pdr_hdr* hdr = reinterpret_cast<pldm_pdr_hdr*>(data.data());
    hdr->type = 1;
    auto first = pldm_pdr_add(repo, data.data(), data.size(), 0);
    hdr->type = 2;
    auto second = pldm_pdr_add(repo, data.data(), data.size(), 0);
    hdr->type = 3;
    auto third = pldm_pdr_add(repo, data.data(), data.size(), 0);
    hdr->type = 4;
    auto fourth = pldm_pdr_add(repo, data.data(), data.size(), 0);

    uint8_t* outData = nullptr;
    uint32_t size{};
    auto firstRec =
        pldm_pdr_find_record_by_type(repo, 1, nullptr, &outData, &size);
    EXPECT_EQ(pldm_pdr_get_record_handle(repo, firstRec), first);
    outData = nullptr;
    auto secondRec =
        pldm_pdr_find_record_by_type(repo, 2, nullptr, &outData, &size);
    EXPECT_EQ(pldm_pdr_get_record_handle(repo, secondRec), second);
    outData = nullptr;
    auto thirdRec =
        pldm_pdr_find_record_by_type(repo, 3, nullptr, &outData, &size);
    EXPECT_EQ(pldm_pdr_get_record_handle(repo, thirdRec), third);
    outData = nullptr;
    auto fourthRec =
        pldm_pdr_find_record_by_type(repo, 4, nullptr, &outData, &size);
    EXPECT_EQ(pldm_pdr_get_record_handle(repo, fourthRec), fourth);
    outData = nullptr;
    auto rec =
        pldm_pdr_find_record_by_type(repo, 3, secondRec, &outData, &size);
    EXPECT_EQ(pldm_pdr_get_record_handle(repo, rec), third);
    outData = nullptr;
    rec = pldm_pdr_find_record_by_type(repo, 4, secondRec, &outData, &size);
    EXPECT_EQ(pldm_pdr_get_record_handle(repo, rec), fourth);
    outData = nullptr;
    rec = pldm_pdr_find_record_by_type(repo, 2, firstRec, &outData, &size);
    EXPECT_EQ(pldm_pdr_get_record_handle(repo, rec), second);
    outData = nullptr;

    pldm_pdr_destroy(&repo);
}
