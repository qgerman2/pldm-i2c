#ifndef BASE_H
#define BASE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <asm/byteorder.h>
#include <stddef.h>
#include <stdint.h>

/** @brief PLDM Types
 */
enum pldm_supported_types {
	PLDM_BASE = 0x00,
};

/** @brief PLDM Commands
 */
enum pldm_supported_commands {
	PLDM_GET_PLDM_TYPES = 0x4,
	PLDM_GET_PLDM_COMMANDS = 0x5
};

/** @brief PLDM base codes
 */
enum pldm_completion_codes {
	PLDM_SUCCESS = 0x00,
	PLDM_ERROR = 0x01,
	PLDM_ERROR_INVALID_DATA = 0x02,
	PLDM_ERROR_INVALID_LENGTH = 0x03,
	PLDM_ERROR_NOT_READY = 0x04,
	PLDM_ERROR_UNSUPPORTED_PLDM_CMD = 0x05,
	PLDM_ERROR_INVALID_PLDM_TYPE = 0x20
};

/** @enum MessageType
 *
 *  The different message types supported by the PLDM specification.
 */
typedef enum {
	RESPONSE,	     //!< PLDM response
	REQUEST,	      //!< PLDM request
	RESERVED,	     //!< Reserved
	ASYNC_REQUEST_NOTIFY, //!< Unacknowledged PLDM request messages
} MessageType;

#define PLDM_INSTANCE_MAX 31
#define PLDM_MAX_TYPES 64
#define PLDM_MAX_CMDS_PER_TYPE 256

/* Response lengths are inclusive of completion code */
#define PLDM_GET_TYPES_RESP_DATA_BYTES 9
#define PLDM_GET_COMMANDS_RESP_DATA_BYTES 33

struct pldm_msg_t {
#if defined(__LITTLE_ENDIAN_BITFIELD)
	uint8_t instance_id : 5; //!< Instance ID
	uint8_t reserved : 1;    //!< Reserved
	uint8_t datagram : 1;    //!< Datagram bit
	uint8_t request : 1;     //!< Request bit
#elif defined(__BIG_ENDIAN_BITFIELD)
	uint8_t request : 1;     //!< Request bit
	uint8_t datagram : 1;    //!< Datagram bit
	uint8_t reserved : 1;    //!< Reserved
	uint8_t instance_id : 5; //!< Instance ID
#endif

#if defined(__LITTLE_ENDIAN_BITFIELD)
	uint8_t type : 6;       //!< PLDM type
	uint8_t header_ver : 2; //!< Header version
#elif defined(__BIG_ENDIAN_BITFIELD)
	uint8_t header_ver : 2;  //!< Header version
	uint8_t type : 6;	//!< PLDM type
#endif
	uint8_t command; //!< PLDM command code
	uint8_t *payload; //!< Msg payload(byte 0 is return code for responses)
} __attribute__((packed));

/** @struct pldm_version_t
 *
 * Structure representing PLDM ver32 type
 */
struct pldm_version_t {
	uint8_t major;
	uint8_t minor;
	uint8_t update;
	uint8_t alpha;
} __attribute__((packed));

/** @struct pldm_header_info
 *
 *  The information needed to prepare PLDM header and this is passed to the
 *  pack_pldm_header and unpack_pldm_header API.
 */
struct pldm_header_info {
	MessageType msg_type;    /* PLDM message type*/
	uint8_t instance;	/* PLDM instance id */
	uint8_t pldm_type;       /* PLDM type */
	uint8_t command;	 /* PLDM command code */
	uint8_t completion_code; /* PLDM completion code, applies only for
			response */
};

/**
 * @brief Populate the PLDM message with the PLDM header.The
 *        PLDM header size is PLDM_REQUEST_HEADER_LEN_BYTES for request and
 *        PLDM_RESPONSE_HEADER_LEN_BYTES response. The caller of this API
 *        allocates buffer for the PLDM header when forming the PLDM message.
 *        The buffer is passed to this API to pack the PLDM header.
 *
 * @param[in] hdr - Pointer to the PLDM header information
 * @param[in,out] msg - Pointer to PLDM message
 *
 * @return 0 on success, otherwise PLDM error codes.
 */
int pack_pldm_header(const struct pldm_header_info *hdr,
		     struct pldm_msg_t *msg);

/**
 * @brief Unpack the PLDM header and the PLDM message payload from the PLDM
 *        message. The pointer to the PLDM message and the size is passed as
 *        parameters. The header is unpacked, and the payload size is returned.
 *
 * @param[in] msg - Pointer to the PLDM message
 * @param[in] size - Size of the PLDM message
 * @param[out] hdr - Pointer to the PLDM header information
 * @param[out] payload_size - Size of the PLDM message payload size
 *
 * @return 0 on success, otherwise PLDM error codes.
 */
int unpack_pldm_header(const struct pldm_msg_t *msg, const size_t size,
		       struct pldm_header_info *hdr, size_t *payload_size);

/* Requester */

/* GetPLDMTypes */

/** @brief Create a PLDM request message for GetPLDMTypes
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in,out] msg - Message will be written to this
 *  @return pldm_completion_codes
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *         'msg.payload'
 */
int encode_get_types_req(uint8_t instance_id, struct pldm_msg_t *msg);

/* GetPLDMCommands */

/** @brief Create a PLDM request message for GetPLDMCommands
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in] type - PLDM Type
 *  @param[in] version - Version for PLDM Type
 *  @param[in,out] msg - Message will be written to this
 *  @return pldm_completion_codes
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *         'msg.payload'
 */
int encode_get_commands_req(uint8_t instance_id, uint8_t type,
			    struct pldm_version_t version,
			    struct pldm_msg_t *msg);

/* Responder */

/* GetPLDMTypes */

/** @brief Create a PLDM response message for GetPLDMTypes
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in] types - pointer to array uint8_t[8] containing supported
 *             types (MAX_TYPES/8) = 8), as per DSP0240
 *  @param[in,out] msg - Message will be written to this
 *  @return pldm_completion_codes
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *         'msg.payload'
 */
int encode_get_types_resp(uint8_t instance_id, const uint8_t *types,
			  struct pldm_msg_t *msg);

/* GetPLDMCommands */

/** @brief Decode GetPLDMCommands' request data
 *
 *  @param[in] msg - Request message
 *  @param[out] type - PLDM Type
 *  @param[out] version - Version for PLDM Type
 *  @return pldm_completion_codes
 */
int decode_get_commands_req(const struct pldm_msg_t *msg, uint8_t *type,
			    struct pldm_version_t *version);

/** @brief Create a PLDM response message for GetPLDMCommands
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in] commands - pointer to array uint8_t[32] containing supported
 *             commands (PLDM_MAX_CMDS_PER_TYPE/8) = 32), as per DSP0240
 *  @param[in,out] msg - Message will be written to this
 *  @return pldm_completion_codes
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *         'msg.payload'
 */
int encode_get_commands_resp(uint8_t instance_id, const uint8_t *commands,
			     struct pldm_msg_t *msg);

#ifdef __cplusplus
}
#endif

#endif /* BASE_H */
