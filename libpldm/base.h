#ifndef BASE_H
#define BASE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <asm/byteorder.h>
#include <stddef.h>
#include <stdint.h>

#include "pldm_types.h"

/** @brief PLDM Types
 */
enum pldm_supported_types {
	PLDM_BASE = 0x00,
	PLDM_PLATFORM = 0x02,
	PLDM_BIOS = 0x03,
};

/** @brief PLDM Commands
 */
enum pldm_supported_commands {
	PLDM_GET_PLDM_VERSION = 0x3,
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

enum transfer_op_flag {
	PLDM_GET_NEXTPART = 0,
	PLDM_GET_FIRSTPART = 1,
};

enum transfer_resp_flag {
	PLDM_START = 0x01,
	PLDM_MIDDLE = 0x02,
	PLDM_END = 0x04,
	PLDM_START_AND_END = 0x05,
};

/** @enum MessageType
 *
 *  The different message types supported by the PLDM specification.
 */
typedef enum {
	PLDM_RESPONSE,		   //!< PLDM response
	PLDM_REQUEST,		   //!< PLDM request
	PLDM_RESERVED,		   //!< Reserved
	PLDM_ASYNC_REQUEST_NOTIFY, //!< Unacknowledged PLDM request messages
} MessageType;

#define PLDM_INSTANCE_MAX 31
#define PLDM_MAX_TYPES 64
#define PLDM_MAX_CMDS_PER_TYPE 256

/* Message payload lengths */
#define PLDM_GET_COMMANDS_REQ_BYTES 5
#define PLDM_GET_VERSION_REQ_BYTES 6

/* Response lengths are inclusive of completion code */
#define PLDM_GET_TYPES_RESP_BYTES 9
#define PLDM_GET_COMMANDS_RESP_BYTES 33
/* Response data has only one version and does not contain the checksum */
#define PLDM_GET_VERSION_RESP_BYTES 10

/** @struct pldm_msg_hdr
 *
 * Structure representing PLDM message header fields
 */
struct pldm_msg_hdr {
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
} __attribute__((packed));

/** @struct pldm_msg
 *
 * Structure representing PLDM message
 */
struct pldm_msg {
	struct pldm_msg_hdr hdr; //!< PLDM message header
	uint8_t payload[1]; //!< &payload[0] is the beginning of the payload
} __attribute__((packed));

/** @struct pldm_header_info
 *
 *  The information needed to prepare PLDM header and this is passed to the
 *  pack_pldm_header and unpack_pldm_header API.
 */
struct pldm_header_info {
	MessageType msg_type;    //!< PLDM message type
	uint8_t instance;	//!< PLDM instance id
	uint8_t pldm_type;       //!< PLDM type
	uint8_t command;	 //!< PLDM command code
	uint8_t completion_code; //!< PLDM completion code, applies for response
};

/** @struct PLDM_GetTypes_Response
 *
 *  Structure representing PLDM get types response.
 *  The GetPLDMTypes command enables management controllers
 *  to discover the PLDM type capabilities supported by the
 *  PLDM terminus and get a list of the PLDM types that are supported.
 */
struct PLDM_GetTypes_Response {
	uint8_t completion_code;
	bitfield8_t types[8];
} __attribute__((packed));

/** @struct PLDM_GetCommands_Request
 *
 *  Structure representing PLDM get commands request.
 *  The GetPLDMCommands command enables management controllers
 *  to discover the PLDM command capabilities supported by the
 *  PLDM terminus for a specific PLDM Type and version as a responder.
 */
struct PLDM_GetCommands_Request {
	uint8_t type;
	ver32_t version;
} __attribute__((packed));

/** @struct PLDM_GetCommands_Response
 *
 *  Structure representing PLDM get commands response.
 *  The GetPLDMCommands command enables management controllers
 *  to discover the PLDM command capabilities supported by the
 *  PLDM terminus for a specific PLDM Type and version as a responder.
 */
struct PLDM_GetCommands_Response {
	uint8_t completion_code;
	bitfield8_t commands[32];
} __attribute__((packed));

/** @struct PLDM_GetVersion_Request
 *
 *  Structure representing PLDM get version request.
 *  The GetPLDMVersion command can be used to retrieve the
 *  PLDM base specification versions that the PLDM terminus
 *  supports, as well as the PLDM Type specification versions
 *  supported for each PLDM Type.
 */
struct PLDM_GetVersion_Request {
	uint32_t transfer_handle;
	uint8_t transfer_opflag;
	uint8_t type;
} __attribute__((packed));

/** @struct PLDM_GetVersion_Response
 *
 *  Structure representing PLDM get version response.
 *  The GetPLDMVersion command can be used to retrieve the
 *  PLDM base specification versions that the PLDM terminus
 *  supports, as well as the PLDM Type specification versions
 *  supported for each PLDM Type.
 */

struct PLDM_GetVersion_Response {
	uint8_t completion_code;
	uint32_t next_transfer_handle;
	uint8_t transfer_flag;
	uint8_t version_data[1];
} __attribute__((packed));

/**
 * @brief Populate the PLDM message with the PLDM header.The caller of this API
 *        allocates buffer for the PLDM header when forming the PLDM message.
 *        The buffer is passed to this API to pack the PLDM header.
 *
 * @param[in] hdr - Pointer to the PLDM header information
 * @param[out] msg - Pointer to PLDM message header
 *
 * @return 0 on success, otherwise PLDM error codes.
 */
int pack_pldm_header(const struct pldm_header_info *hdr,
		     struct pldm_msg_hdr *msg);

/**
 * @brief Unpack the PLDM header from the PLDM message.
 *
 * @param[in] msg - Pointer to the PLDM message header
 * @param[out] hdr - Pointer to the PLDM header information
 *
 * @return 0 on success, otherwise PLDM error codes.
 */
int unpack_pldm_header(const struct pldm_msg_hdr *msg,
		       struct pldm_header_info *hdr);

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
int encode_get_types_req(uint8_t instance_id, struct pldm_msg *msg);

/** @brief Decode a GetPLDMTypes response message
 *
 *  @param[in] msg - Response message payload
 *  @param[in] payload_length - Length of response message payload
 *  @param[out] completion_code - Pointer to response msg's PLDM completion code
 *  @param[out] types - pointer to array bitfield8_t[8] containing supported
 *              types (MAX_TYPES/8) = 8), as per DSP0240
 *  @return pldm_completion_codes
 */
int decode_get_types_resp(const uint8_t *msg, size_t payload_length,
			  uint8_t *completion_code, bitfield8_t *types);

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
int encode_get_commands_req(uint8_t instance_id, uint8_t type, ver32_t version,
			    struct pldm_msg *msg);

/** @brief Decode a GetPLDMCommands response message
 *
 *  @param[in] msg - Response message payload
 *  @param[in] payload_length - Length of reponse message payload
 *  @param[out] completion_code - Pointer to response msg's PLDM completion code
 *  @param[in] commands - pointer to array bitfield8_t[32] containing supported
 *             commands (PLDM_MAX_CMDS_PER_TYPE/8) = 32), as per DSP0240
 *  @return pldm_completion_codes
 */
int decode_get_commands_resp(const uint8_t *msg, size_t payload_length,
			     uint8_t *completion_code, bitfield8_t *commands);

/* GetPLDMVersion */

/** @brief Create a PLDM request for GetPLDMVersion
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in] transfer_handle - Handle to identify PLDM version data transfer.
 *         This handle is ignored by the responder when the
 *         transferop_flag is set to getFirstPart.
 *  @param[in] transfer_opflag - flag to indicate whether it is start of
 *         transfer
 *  @param[in] type -  PLDM Type for which version is requested
 *  @param[in,out] msg - Message will be written to this
 *  @return pldm_completion_codes
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *         'msg.payload'
 */
int encode_get_version_req(uint8_t instance_id, uint32_t transfer_handle,
			   uint8_t transfer_opflag, uint8_t type,
			   struct pldm_msg *msg);

/** @brief Decode a GetPLDMVersion response message
 *
 *  @param[in] msg - Response message payload
 *  @param[in] payload_length - Length of reponse message payload
 *  @param[out] completion_code - Pointer to response msg's PLDM completion code
 *  @param[out] next_transfer_handle - the next handle for the next part of data
 *  @param[out] transfer_flag - flag to indicate the part of data
 *  @return pldm_completion_codes
 */
int decode_get_version_resp(const uint8_t *msg, size_t payload_length,
			    uint8_t *completion_code,
			    uint32_t *next_transfer_handle,
			    uint8_t *transfer_flag, ver32_t *version);

/* Responder */

/* GetPLDMTypes */

/** @brief Create a PLDM response message for GetPLDMTypes
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in] completion_code - PLDM completion code
 *  @param[in] types - pointer to array bitfield8_t[8] containing supported
 *             types (MAX_TYPES/8) = 8), as per DSP0240
 *  @param[in,out] msg - Message will be written to this
 *  @return pldm_completion_codes
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *         'msg.payload'
 */
int encode_get_types_resp(uint8_t instance_id, uint8_t completion_code,
			  const bitfield8_t *types, struct pldm_msg *msg);

/* GetPLDMCommands */

/** @brief Decode GetPLDMCommands' request data
 *
 *  @param[in] msg - Request message
 *  @param[in] payload_length - Length of request message payload
 *  @param[out] type - PLDM Type
 *  @param[out] version - Version for PLDM Type
 *  @return pldm_completion_codes
 */
int decode_get_commands_req(const uint8_t *msg, size_t payload_length,
			    uint8_t *type, ver32_t *version);

/** @brief Create a PLDM response message for GetPLDMCommands
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in] completion_code - PLDM completion code
 *  @param[in] commands - pointer to array bitfield8_t[32] containing supported
 *             commands (PLDM_MAX_CMDS_PER_TYPE/8) = 32), as per DSP0240
 *  @param[in,out] msg - Message will be written to this
 *  @return pldm_completion_codes
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *         'msg.payload'
 */
int encode_get_commands_resp(uint8_t instance_id, uint8_t completion_code,
			     const bitfield8_t *commands, struct pldm_msg *msg);

/* GetPLDMVersion */

/** @brief Create a PLDM response for GetPLDMVersion
 *
 *  @param[in] instance_id - Message's instance id
 *  @param[in] completion_code - PLDM completion code
 *  @param[in] next_transfer_handle - Handle to identify next portion of
 *              data transfer
 *  @param[in] transfer_flag - Represents the part of transfer
 *  @param[in] version_data - the version data
 *  @param[in] version_size - size of version data
 *  @param[in,out] msg - Message will be written to this
 *  @return pldm_completion_codes
 *  @note  Caller is responsible for memory alloc and dealloc of param
 *         'msg.payload'
 */
int encode_get_version_resp(uint8_t instance_id, uint8_t completion_code,
			    uint32_t next_transfer_handle,
			    uint8_t transfer_flag, const ver32_t *version_data,
			    size_t version_size, struct pldm_msg *msg);

/** @brief Decode a GetPLDMVersion request message
 *
 *  @param[in] msg - Request message
 *  @param[in] payload_length - length of request message payload
 *  @param[out] transfer_handle - the handle of data
 *  @param[out] transfer_opflag - Transfer Flag
 *  @param[out] type - PLDM type for which version is requested
 *  @return pldm_completion_codes
 */
int decode_get_version_req(const uint8_t *msg, size_t payload_length,
			   uint32_t *transfer_handle, uint8_t *transfer_opflag,
			   uint8_t *type);

#ifdef __cplusplus
}
#endif

#endif /* BASE_H */
