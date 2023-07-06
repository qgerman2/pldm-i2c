#pragma once

#include <vector>
#include <iostream>

#define PLDM_MAX_REQUEST_BYTES 2048
#define MAX_RETRIES_FOR_REQUEST 500

enum ContextState
{
    RDE_CONTEXT_FREE = 0,
    RDE_CONTEXT_NOT_FREE = 1,
    RDE_NO_CONTEXT_FOUND = 2,
};

int verify_checksum_for_multipart_recv2(std::vector<uint8_t>* payload);
