#include "libpldmresponder/effecters.hpp"
#include "libpldmresponder/pdr.hpp"

#include "libpldm/platform.h"

#include <gtest/gtest.h>

using namespace pldm::responder;

TEST(GeneratePDR, testGoodJson)
{
    using namespace pdr;
    using namespace effecter::dbus_mapping;
    Repo& pdrRepo = get("./pdr_jsons/state_sensor/good");

    // 2 entries
    ASSERT_EQ(pdrRepo.numEntries(), 2);

    // Check first PDR
    pdr::Entry e = pdrRepo.at(1);
    pldm_state_sensor_pdr* pdr =
        reinterpret_cast<pldm_state_sensor_pdr*>(e.data());

    ASSERT_EQ(pdr->hdr.record_handle, 1);
    ASSERT_EQ(pdr->hdr.version, 1);
    ASSERT_EQ(pdr->hdr.type, PLDM_STATE_SENSOR_PDR);
    ASSERT_EQ(pdr->hdr.record_change_num, 0);
    ASSERT_EQ(pdr->hdr.length, 17);

    ASSERT_EQ(pdr->terminus_handle, 0);
    ASSERT_EQ(pdr->sensor_id, 1);
    ASSERT_EQ(pdr->entity_type, 33);
    ASSERT_EQ(pdr->entity_instance, 0);
    ASSERT_EQ(pdr->container_id, 0);
    ASSERT_EQ(pdr->sensor_init, PLDM_NO_INIT);
    ASSERT_EQ(pdr->sensor_aux_names_pdr, false);
    ASSERT_EQ(pdr->composite_sensor_count, 1);
    state_effecter_or_sensor_possible_states* states =
        reinterpret_cast<state_effecter_or_sensor_possible_states*>(
            pdr->possible_states);
    ASSERT_EQ(states->state_set_id, 196);
    ASSERT_EQ(states->possible_states_size, 1);
    bitfield8_t bf1{};
    bf1.byte = 2;
    ASSERT_EQ(states->states[0].byte, bf1.byte);

    auto paths = get(pdr->sensor_id);
    ASSERT_EQ(paths[0], "/foo/bar");

    // Check second PDR
    e = pdrRepo.at(2);
    pdr = reinterpret_cast<pldm_state_sensor_pdr*>(e.data());

    ASSERT_EQ(pdr->hdr.record_handle, 2);
    ASSERT_EQ(pdr->hdr.version, 1);
    ASSERT_EQ(pdr->hdr.type, PLDM_STATE_SENSOR_PDR);
    ASSERT_EQ(pdr->hdr.record_change_num, 0);
    ASSERT_EQ(pdr->hdr.length, 22);

    ASSERT_EQ(pdr->terminus_handle, 0);
    ASSERT_EQ(pdr->sensor_id, 2);
    ASSERT_EQ(pdr->entity_type, 100);
    ASSERT_EQ(pdr->entity_instance, 0);
    ASSERT_EQ(pdr->container_id, 0);
    ASSERT_EQ(pdr->sensor_init, PLDM_NO_INIT);
    ASSERT_EQ(pdr->sensor_aux_names_pdr, false);
    ASSERT_EQ(pdr->composite_sensor_count, 2);
    states = reinterpret_cast<state_effecter_or_sensor_possible_states*>(
        pdr->possible_states);
    ASSERT_EQ(states->state_set_id, 197);
    ASSERT_EQ(states->possible_states_size, 1);
    bf1.byte = 2;
    ASSERT_EQ(states->states[0].byte, bf1.byte);
    states = reinterpret_cast<state_effecter_or_sensor_possible_states*>(
        pdr->possible_states +
        sizeof(state_effecter_or_sensor_possible_states));
    ASSERT_EQ(states->state_set_id, 198);
    ASSERT_EQ(states->possible_states_size, 2);
    bitfield8_t bf2[2];
    bf2[0].byte = 38;
    bf2[1].byte = 128;
    ASSERT_EQ(states->states[0].byte, bf2[0].byte);
    ASSERT_EQ(states->states[1].byte, bf2[1].byte);

    paths = get(pdr->sensor_id);
    ASSERT_EQ(paths[0], "/foo/bar");
    ASSERT_EQ(paths[1], "/foo/bar/baz");
}

TEST(GeneratePDR, testNoJson)
{
    using namespace pdr;
    Repo& pdrRepo = get("./pdr_jsons/not_there");

    ASSERT_EQ(pdrRepo.numEntries(), 2);
}

TEST(GeneratePDR, testMalformedJson)
{
    using namespace pdr;
    Repo& pdrRepo = get("./pdr_jsons/state_sensor/good");
    ASSERT_EQ(pdrRepo.numEntries(), 2);
    pdrRepo.makeEmpty();
    ASSERT_THROW(get("./pdr_jsons/state_sensor/malformed"), std::exception);
}
