#include "libpldm/pldm_rde_requester.h"

#include <iostream>
#include <map>
#include <string>
#include <vector>

#pragma once

#define RDE_NEGOTIATE_REDFISH_PARAMS_RESP_SIZE 12
#define RDE_NEGOTIATE_REDFISH_MEDIUM_PARAMS_RESP_SIZE 6
#define RDE_GET_DICT_SCHEMA_RESP_SIZE 6

extern std::map<uint8_t, int> rdeCommandRequestSize;

int performRdeDiscovery(std::string rdeDeviceId, int fd, int netId,
                        uint8_t destEid, uint8_t instanceId);

int performDictionaryDiscoveryForDevice(std::string rdeDeviceId, int fd,
                                        uint8_t destEid, uint8_t instanceId);

int getRdeFreeContextForRdeDevice(
    std::string rdeDevice, struct pldm_rde_requester_context* baseContext);

int getDictionaryForRidDev(std::string rdeDeviceId, uint32_t resourceId,
                               uint8_t** dictElem, uint32_t* dictLength);

int getManagerForRdeDevice(std::string rdeDevice,
                               struct pldm_rde_requester_manager** manager);
int verifyChecksumForMultipartRecv(std::vector<uint8_t>* payload);