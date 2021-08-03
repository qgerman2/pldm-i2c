#ifndef OEM_IBM_STATE_SETS_H
#define OEM_IBM_STATE_SETS_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief IBM OEM State Set IDs */
enum ibm_oem_pldm_state_set_ids {
	PLDM_OEM_IBM_FIRMWARE_UPDATE_STATE = 32768,
	PLDM_OEM_IBM_BOOT_STATE = 32769,
	PLDM_OEM_IBM_VERIFICATION_STATE = 32770,
	PLDM_OEM_IBM_SYSTEM_POWER_STATE = 32771,
};

enum ibm_oem_pldm_state_set_firmware_update_state_values {
	START = 0x1,
	END = 0x2,
	FAIL = 0x3,
	ABORT = 0x4,
	ACCEPT = 0x5,
	REJECT = 0x6,
};

enum ibm_oem_pldm_state_set_boot_state_values {
	P = 0x1,
	T = 0x2,
};

enum ibm_oem_pldm_state_set_verification_state_values {
	VALID = 0x0,
	ENTITLEMENT_FAIL = 0x1,
	BANNED_PLATFORM_FAIL = 0x2,
	MIN_MIF_FAIL = 0x4,
};

enum ibm_oem_pldm_state_set_system_power_state_values {
	POWER_CYCLE_HARD = 0x1,
};

#ifdef __cplusplus
}
#endif

#endif /* OEM_IBM_STATE_SETS_H */