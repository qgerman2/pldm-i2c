#include "firmware_update.h"
#include <endian.h>
#include <string.h>

/** @brief Check whether string type value is valid
 *
 *  @return true if string type value is valid, false if not
 */
static bool is_string_type_valid(uint8_t string_type)
{
	switch (string_type) {
	case PLDM_STR_TYPE_UNKNOWN:
		return false;
	case PLDM_STR_TYPE_ASCII:
	case PLDM_STR_TYPE_UTF_8:
	case PLDM_STR_TYPE_UTF_16:
	case PLDM_STR_TYPE_UTF_16LE:
	case PLDM_STR_TYPE_UTF_16BE:
		return true;
	default:
		return false;
	}
}

/** @brief Return the length of the descriptor type described in firmware update
 *         specification
 *
 *  @return length of the descriptor type if descriptor type is valid else
 *          return 0
 */
static uint16_t get_descriptor_type_length(uint16_t descriptor_type)
{
	switch (descriptor_type) {

	case PLDM_FWUP_PCI_VENDOR_ID:
		return PLDM_FWUP_PCI_VENDOR_ID_LENGTH;
	case PLDM_FWUP_IANA_ENTERPRISE_ID:
		return PLDM_FWUP_IANA_ENTERPRISE_ID_LENGTH;
	case PLDM_FWUP_UUID:
		return PLDM_FWUP_UUID_LENGTH;
	case PLDM_FWUP_PNP_VENDOR_ID:
		return PLDM_FWUP_PNP_VENDOR_ID_LENGTH;
	case PLDM_FWUP_ACPI_VENDOR_ID:
		return PLDM_FWUP_ACPI_VENDOR_ID_LENGTH;
	case PLDM_FWUP_IEEE_ASSIGNED_COMPANY_ID:
		return PLDM_FWUP_IEEE_ASSIGNED_COMPANY_ID_LENGTH;
	case PLDM_FWUP_SCSI_VENDOR_ID:
		return PLDM_FWUP_SCSI_VENDOR_ID_LENGTH;
	case PLDM_FWUP_PCI_DEVICE_ID:
		return PLDM_FWUP_PCI_DEVICE_ID_LENGTH;
	case PLDM_FWUP_PCI_SUBSYSTEM_VENDOR_ID:
		return PLDM_FWUP_PCI_SUBSYSTEM_VENDOR_ID_LENGTH;
	case PLDM_FWUP_PCI_SUBSYSTEM_ID:
		return PLDM_FWUP_PCI_SUBSYSTEM_ID_LENGTH;
	case PLDM_FWUP_PCI_REVISION_ID:
		return PLDM_FWUP_PCI_REVISION_ID_LENGTH;
	case PLDM_FWUP_PNP_PRODUCT_IDENTIFIER:
		return PLDM_FWUP_PNP_PRODUCT_IDENTIFIER_LENGTH;
	case PLDM_FWUP_ACPI_PRODUCT_IDENTIFIER:
		return PLDM_FWUP_ACPI_PRODUCT_IDENTIFIER_LENGTH;
	case PLDM_FWUP_ASCII_MODEL_NUMBER_LONG_STRING:
		return PLDM_FWUP_ASCII_MODEL_NUMBER_LONG_STRING_LENGTH;
	case PLDM_FWUP_ASCII_MODEL_NUMBER_SHORT_STRING:
		return PLDM_FWUP_ASCII_MODEL_NUMBER_SHORT_STRING_LENGTH;
	case PLDM_FWUP_SCSI_PRODUCT_ID:
		return PLDM_FWUP_SCSI_PRODUCT_ID_LENGTH;
	case PLDM_FWUP_UBM_CONTROLLER_DEVICE_CODE:
		return PLDM_FWUP_UBM_CONTROLLER_DEVICE_CODE_LENGTH;
	default:
		return 0;
	}
}

int decode_pldm_package_header_info(
    const uint8_t *data, size_t length,
    struct pldm_package_header_information *package_header_info,
    struct variable_field *package_version_str)
{
	if (data == NULL || package_header_info == NULL ||
	    package_version_str == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (length < sizeof(struct pldm_package_header_information)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_package_header_information *data_header =
	    (struct pldm_package_header_information *)(data);

	if (!is_string_type_valid(data_header->package_version_string_type) ||
	    (data_header->package_version_string_length == 0)) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (length < sizeof(struct pldm_package_header_information) +
			 data_header->package_version_string_length) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	if ((data_header->component_bitmap_bit_length %
	     PLDM_FWUP_COMPONENT_BITMAP_MULTIPLE) != 0) {
		return PLDM_ERROR_INVALID_DATA;
	}

	memcpy(package_header_info->uuid, data_header->uuid,
	       sizeof(data_header->uuid));
	package_header_info->package_header_format_version =
	    data_header->package_header_format_version;
	package_header_info->package_header_size =
	    le16toh(data_header->package_header_size);
	memcpy(package_header_info->timestamp104, data_header->timestamp104,
	       sizeof(data_header->timestamp104));
	package_header_info->component_bitmap_bit_length =
	    le16toh(data_header->component_bitmap_bit_length);
	package_header_info->package_version_string_type =
	    data_header->package_version_string_type;
	package_header_info->package_version_string_length =
	    data_header->package_version_string_length;
	package_version_str->ptr =
	    data + sizeof(struct pldm_package_header_information);
	package_version_str->length =
	    package_header_info->package_version_string_length;

	return PLDM_SUCCESS;
}

int decode_firmware_device_id_record(
    const uint8_t *data, size_t length, uint16_t component_bitmap_bit_length,
    struct pldm_firmware_device_id_record *fw_device_id_record,
    struct variable_field *applicable_components,
    struct variable_field *comp_image_set_version_str,
    struct variable_field *record_descriptors,
    struct variable_field *fw_device_pkg_data)
{
	if (data == NULL || fw_device_id_record == NULL ||
	    applicable_components == NULL ||
	    comp_image_set_version_str == NULL || record_descriptors == NULL ||
	    fw_device_pkg_data == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (length < sizeof(struct pldm_firmware_device_id_record)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	if ((component_bitmap_bit_length %
	     PLDM_FWUP_COMPONENT_BITMAP_MULTIPLE) != 0) {
		return PLDM_ERROR_INVALID_DATA;
	}

	struct pldm_firmware_device_id_record *data_record =
	    (struct pldm_firmware_device_id_record *)(data);

	if (!is_string_type_valid(
		data_record->comp_image_set_version_string_type) ||
	    (data_record->comp_image_set_version_string_length == 0)) {
		return PLDM_ERROR_INVALID_DATA;
	}

	fw_device_id_record->record_length =
	    le16toh(data_record->record_length);
	fw_device_id_record->descriptor_count = data_record->descriptor_count;
	fw_device_id_record->device_update_option_flags.value =
	    le32toh(data_record->device_update_option_flags.value);
	fw_device_id_record->comp_image_set_version_string_type =
	    data_record->comp_image_set_version_string_type;
	fw_device_id_record->comp_image_set_version_string_length =
	    data_record->comp_image_set_version_string_length;
	fw_device_id_record->fw_device_pkg_data_length =
	    le16toh(data_record->fw_device_pkg_data_length);

	if (length < fw_device_id_record->record_length) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	uint16_t applicable_components_length =
	    component_bitmap_bit_length / PLDM_FWUP_COMPONENT_BITMAP_MULTIPLE;
	uint16_t calc_min_record_length =
	    sizeof(struct pldm_firmware_device_id_record) +
	    applicable_components_length +
	    data_record->comp_image_set_version_string_length +
	    PLDM_FWUP_DEVICE_DESCRIPTOR_MIN_LEN +
	    fw_device_id_record->fw_device_pkg_data_length;

	if (fw_device_id_record->record_length < calc_min_record_length) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	applicable_components->ptr =
	    data + sizeof(struct pldm_firmware_device_id_record);
	applicable_components->length = applicable_components_length;

	comp_image_set_version_str->ptr =
	    applicable_components->ptr + applicable_components->length;
	comp_image_set_version_str->length =
	    fw_device_id_record->comp_image_set_version_string_length;

	record_descriptors->ptr = comp_image_set_version_str->ptr +
				  comp_image_set_version_str->length;
	record_descriptors->length =
	    fw_device_id_record->record_length -
	    sizeof(struct pldm_firmware_device_id_record) -
	    applicable_components_length -
	    fw_device_id_record->comp_image_set_version_string_length -
	    fw_device_id_record->fw_device_pkg_data_length;

	if (fw_device_id_record->fw_device_pkg_data_length) {
		fw_device_pkg_data->ptr =
		    record_descriptors->ptr + record_descriptors->length;
		fw_device_pkg_data->length =
		    fw_device_id_record->fw_device_pkg_data_length;
	}

	return PLDM_SUCCESS;
}

int decode_descriptor_type_length_value(const uint8_t *data, size_t length,
					uint16_t *descriptor_type,
					struct variable_field *descriptor_data)
{
	uint16_t descriptor_length = 0;

	if (data == NULL || descriptor_type == NULL ||
	    descriptor_data == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (length < PLDM_FWUP_DEVICE_DESCRIPTOR_MIN_LEN) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_descriptor_tlv *entry =
	    (struct pldm_descriptor_tlv *)(data);

	*descriptor_type = le16toh(entry->descriptor_type);
	descriptor_length = le16toh(entry->descriptor_length);
	if (*descriptor_type != PLDM_FWUP_VENDOR_DEFINED) {
		if (descriptor_length !=
		    get_descriptor_type_length(*descriptor_type)) {
			return PLDM_ERROR_INVALID_LENGTH;
		}
	}

	if (length < (sizeof(*descriptor_type) + sizeof(descriptor_length) +
		      descriptor_length)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	descriptor_data->ptr = entry->descriptor_data;
	descriptor_data->length = descriptor_length;

	return PLDM_SUCCESS;
}

int decode_vendor_defined_descriptor_value(
    const uint8_t *data, size_t length, uint8_t *descriptor_title_str_type,
    struct variable_field *descriptor_title_str,
    struct variable_field *descriptor_data)
{
	if (data == NULL || descriptor_title_str_type == NULL ||
	    descriptor_title_str == NULL || descriptor_data == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (length < sizeof(struct pldm_vendor_defined_descriptor_title_data)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_vendor_defined_descriptor_title_data *entry =
	    (struct pldm_vendor_defined_descriptor_title_data *)(data);
	if (!is_string_type_valid(
		entry->vendor_defined_descriptor_title_str_type) ||
	    (entry->vendor_defined_descriptor_title_str_len == 0)) {
		return PLDM_ERROR_INVALID_DATA;
	}

	// Assuming atleast 1 byte of VendorDefinedDescriptorData
	if (length < (sizeof(struct pldm_vendor_defined_descriptor_title_data) +
		      entry->vendor_defined_descriptor_title_str_len)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	*descriptor_title_str_type =
	    entry->vendor_defined_descriptor_title_str_type;
	descriptor_title_str->ptr = entry->vendor_defined_descriptor_title_str;
	descriptor_title_str->length =
	    entry->vendor_defined_descriptor_title_str_len;

	descriptor_data->ptr =
	    descriptor_title_str->ptr + descriptor_title_str->length;
	descriptor_data->length =
	    length - sizeof(entry->vendor_defined_descriptor_title_str_type) -
	    sizeof(entry->vendor_defined_descriptor_title_str_len) -
	    descriptor_title_str->length;

	return PLDM_SUCCESS;
}

int decode_pldm_comp_image_info(
    const uint8_t *data, size_t length,
    struct pldm_component_image_information *pldm_comp_image_info,
    struct variable_field *comp_version_str)
{
	if (data == NULL || pldm_comp_image_info == NULL ||
	    comp_version_str == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (length < sizeof(struct pldm_component_image_information)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_component_image_information *data_header =
	    (struct pldm_component_image_information *)(data);

	if (!is_string_type_valid(data_header->comp_version_string_type) ||
	    (data_header->comp_version_string_length == 0)) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (length < sizeof(struct pldm_component_image_information) +
			 data_header->comp_version_string_length) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	pldm_comp_image_info->comp_classification =
	    le16toh(data_header->comp_classification);
	pldm_comp_image_info->comp_identifier =
	    le16toh(data_header->comp_identifier);
	pldm_comp_image_info->comp_comparison_stamp =
	    le32toh(data_header->comp_comparison_stamp);
	pldm_comp_image_info->comp_options.value =
	    le16toh(data_header->comp_options.value);
	pldm_comp_image_info->requested_comp_activation_method.value =
	    le16toh(data_header->requested_comp_activation_method.value);
	pldm_comp_image_info->comp_location_offset =
	    le32toh(data_header->comp_location_offset);
	pldm_comp_image_info->comp_size = le32toh(data_header->comp_size);
	pldm_comp_image_info->comp_version_string_type =
	    data_header->comp_version_string_type;
	pldm_comp_image_info->comp_version_string_length =
	    data_header->comp_version_string_length;

	if ((pldm_comp_image_info->comp_options.bits.bit1 == false &&
	     pldm_comp_image_info->comp_comparison_stamp !=
		 PLDM_FWUP_INVALID_COMPONENT_COMPARISON_TIMESTAMP)) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (pldm_comp_image_info->comp_location_offset == 0 ||
	    pldm_comp_image_info->comp_size == 0) {
		return PLDM_ERROR_INVALID_DATA;
	}

	comp_version_str->ptr =
	    data + sizeof(struct pldm_component_image_information);
	comp_version_str->length =
	    pldm_comp_image_info->comp_version_string_length;

	return PLDM_SUCCESS;
}

int encode_query_device_identifiers_req(uint8_t instance_id,
					size_t payload_length,
					struct pldm_msg *msg)
{
	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length != PLDM_QUERY_DEVICE_IDENTIFIERS_REQ_BYTES) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	return encode_pldm_header_only(PLDM_REQUEST, instance_id, PLDM_FWUP,
				       PLDM_QUERY_DEVICE_IDENTIFIERS, msg);
}

int decode_query_device_identifiers_resp(const struct pldm_msg *msg,
					 size_t payload_length,
					 uint8_t *completion_code,
					 uint32_t *device_identifiers_len,
					 uint8_t *descriptor_count,
					 uint8_t **descriptor_data)
{
	if (msg == NULL || completion_code == NULL ||
	    device_identifiers_len == NULL || descriptor_count == NULL ||
	    descriptor_data == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	*completion_code = msg->payload[0];
	if (PLDM_SUCCESS != *completion_code) {
		return PLDM_SUCCESS;
	}

	if (payload_length <
	    sizeof(struct pldm_query_device_identifiers_resp)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_query_device_identifiers_resp *response =
	    (struct pldm_query_device_identifiers_resp *)msg->payload;
	*device_identifiers_len = le32toh(response->device_identifiers_len);

	if (*device_identifiers_len < PLDM_FWUP_DEVICE_DESCRIPTOR_MIN_LEN) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	if (payload_length !=
	    sizeof(struct pldm_query_device_identifiers_resp) +
		*device_identifiers_len) {
		return PLDM_ERROR_INVALID_LENGTH;
	}
	*descriptor_count = response->descriptor_count;

	if (*descriptor_count == 0) {
		return PLDM_ERROR_INVALID_DATA;
	}
	*descriptor_data =
	    (uint8_t *)(msg->payload +
			sizeof(struct pldm_query_device_identifiers_resp));
	return PLDM_SUCCESS;
}

int encode_get_firmware_parameters_req(uint8_t instance_id,
				       size_t payload_length,
				       struct pldm_msg *msg)
{
	if (msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length != PLDM_GET_FIRMWARE_PARAMETERS_REQ_BYTES) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	return encode_pldm_header_only(PLDM_REQUEST, instance_id, PLDM_FWUP,
				       PLDM_GET_FIRMWARE_PARAMETERS, msg);
}

int decode_get_firmware_parameters_resp(
    const struct pldm_msg *msg, size_t payload_length,
    struct pldm_get_firmware_parameters_resp *resp_data,
    struct variable_field *active_comp_image_set_ver_str,
    struct variable_field *pending_comp_image_set_ver_str,
    struct variable_field *comp_parameter_table)
{
	if (msg == NULL || resp_data == NULL ||
	    active_comp_image_set_ver_str == NULL ||
	    pending_comp_image_set_ver_str == NULL ||
	    comp_parameter_table == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length < sizeof(struct pldm_get_firmware_parameters_resp)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_get_firmware_parameters_resp *response =
	    (struct pldm_get_firmware_parameters_resp *)msg->payload;

	resp_data->completion_code = response->completion_code;

	if (PLDM_SUCCESS != resp_data->completion_code) {
		return PLDM_SUCCESS;
	}

	if (!is_string_type_valid(
		response->active_comp_image_set_ver_str_type) ||
	    (response->active_comp_image_set_ver_str_len == 0)) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (response->pending_comp_image_set_ver_str_len == 0) {
		if (response->pending_comp_image_set_ver_str_type !=
		    PLDM_STR_TYPE_UNKNOWN) {
			return PLDM_ERROR_INVALID_DATA;
		}
	} else {
		if (!is_string_type_valid(
			response->pending_comp_image_set_ver_str_type)) {
			return PLDM_ERROR_INVALID_DATA;
		}
	}

	size_t partial_response_length =
	    sizeof(struct pldm_get_firmware_parameters_resp) +
	    response->active_comp_image_set_ver_str_len +
	    response->pending_comp_image_set_ver_str_len;

	if (payload_length < partial_response_length) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	resp_data->capabilities_during_update.value =
	    le32toh(response->capabilities_during_update.value);
	resp_data->comp_count = le16toh(response->comp_count);
	resp_data->active_comp_image_set_ver_str_type =
	    response->active_comp_image_set_ver_str_type;
	resp_data->active_comp_image_set_ver_str_len =
	    response->active_comp_image_set_ver_str_len;
	resp_data->pending_comp_image_set_ver_str_type =
	    response->pending_comp_image_set_ver_str_type;
	resp_data->pending_comp_image_set_ver_str_len =
	    response->pending_comp_image_set_ver_str_len;

	active_comp_image_set_ver_str->ptr =
	    msg->payload + sizeof(struct pldm_get_firmware_parameters_resp);
	active_comp_image_set_ver_str->length =
	    resp_data->active_comp_image_set_ver_str_len;

	if (resp_data->pending_comp_image_set_ver_str_len != 0) {
		pending_comp_image_set_ver_str->ptr =
		    msg->payload +
		    sizeof(struct pldm_get_firmware_parameters_resp) +
		    resp_data->active_comp_image_set_ver_str_len;
		pending_comp_image_set_ver_str->length =
		    resp_data->pending_comp_image_set_ver_str_len;
	} else {
		pending_comp_image_set_ver_str->ptr = NULL;
		pending_comp_image_set_ver_str->length = 0;
	}

	if (payload_length > partial_response_length && resp_data->comp_count) {
		comp_parameter_table->ptr =
		    msg->payload +
		    sizeof(struct pldm_get_firmware_parameters_resp) +
		    resp_data->active_comp_image_set_ver_str_len +
		    resp_data->pending_comp_image_set_ver_str_len;
		comp_parameter_table->length =
		    payload_length - partial_response_length;
	} else {
		comp_parameter_table->ptr = NULL;
		comp_parameter_table->length = 0;
	}

	return PLDM_SUCCESS;
}

int decode_get_firmware_parameters_resp_comp_entry(
    const uint8_t *data, size_t length,
    struct pldm_component_parameter_entry *component_data,
    struct variable_field *active_comp_ver_str,
    struct variable_field *pending_comp_ver_str)
{
	if (data == NULL || component_data == NULL ||
	    active_comp_ver_str == NULL || pending_comp_ver_str == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (length < sizeof(struct pldm_component_parameter_entry)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_component_parameter_entry *entry =
	    (struct pldm_component_parameter_entry *)(data);
	if (entry->active_comp_ver_str_len == 0) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	size_t entry_length = sizeof(struct pldm_component_parameter_entry) +
			      entry->active_comp_ver_str_len +
			      entry->pending_comp_ver_str_len;

	if (length < entry_length) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	component_data->comp_classification =
	    le16toh(entry->comp_classification);
	component_data->comp_identifier = le16toh(entry->comp_identifier);
	component_data->comp_classification_index =
	    entry->comp_classification_index;
	component_data->active_comp_comparison_stamp =
	    le32toh(entry->active_comp_comparison_stamp);
	component_data->active_comp_ver_str_type =
	    entry->active_comp_ver_str_type;
	component_data->active_comp_ver_str_len =
	    entry->active_comp_ver_str_len;
	memcpy(component_data->active_comp_release_date,
	       entry->active_comp_release_date,
	       sizeof(entry->active_comp_release_date));
	component_data->pending_comp_comparison_stamp =
	    le32toh(entry->pending_comp_comparison_stamp);
	component_data->pending_comp_ver_str_type =
	    entry->pending_comp_ver_str_type;
	component_data->pending_comp_ver_str_len =
	    entry->pending_comp_ver_str_len;
	memcpy(component_data->pending_comp_release_date,
	       entry->pending_comp_release_date,
	       sizeof(entry->pending_comp_release_date));
	component_data->comp_activation_methods.value =
	    le16toh(entry->comp_activation_methods.value);
	component_data->capabilities_during_update.value =
	    le32toh(entry->capabilities_during_update.value);

	active_comp_ver_str->ptr =
	    data + sizeof(struct pldm_component_parameter_entry);
	active_comp_ver_str->length = entry->active_comp_ver_str_len;

	if (entry->pending_comp_ver_str_len != 0) {

		pending_comp_ver_str->ptr =
		    data + sizeof(struct pldm_component_parameter_entry) +
		    entry->active_comp_ver_str_len;
		pending_comp_ver_str->length = entry->pending_comp_ver_str_len;
	} else {
		pending_comp_ver_str->ptr = NULL;
		pending_comp_ver_str->length = 0;
	}
	return PLDM_SUCCESS;
}

int encode_request_update_req(uint8_t instance_id, uint32_t max_transfer_size,
			      uint16_t num_of_comp,
			      uint8_t max_outstanding_transfer_req,
			      uint16_t pkg_data_len,
			      uint8_t comp_image_set_ver_str_type,
			      uint8_t comp_image_set_ver_str_len,
			      const struct variable_field *comp_img_set_ver_str,
			      struct pldm_msg *msg, size_t payload_length)
{
	if (comp_img_set_ver_str == NULL || comp_img_set_ver_str->ptr == NULL ||
	    msg == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length != sizeof(struct pldm_request_update_req) +
				  comp_img_set_ver_str->length) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	if ((comp_image_set_ver_str_len == 0) ||
	    (comp_image_set_ver_str_len != comp_img_set_ver_str->length)) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if ((max_transfer_size < PLDM_FWUP_BASELINE_TRANSFER_SIZE) ||
	    (max_outstanding_transfer_req < PLDM_FWUP_MIN_OUTSTANDING_REQ)) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (!is_string_type_valid(comp_image_set_ver_str_type)) {
		return PLDM_ERROR_INVALID_DATA;
	}

	struct pldm_header_info header = {0};
	header.instance = instance_id;
	header.msg_type = PLDM_REQUEST;
	header.pldm_type = PLDM_FWUP;
	header.command = PLDM_REQUEST_UPDATE;
	uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
	if (rc) {
		return rc;
	}

	struct pldm_request_update_req *request =
	    (struct pldm_request_update_req *)msg->payload;

	request->max_transfer_size = htole32(max_transfer_size);
	request->num_of_comp = htole16(num_of_comp);
	request->max_outstanding_transfer_req = max_outstanding_transfer_req;
	request->pkg_data_len = htole16(pkg_data_len);
	request->comp_image_set_ver_str_type = comp_image_set_ver_str_type;
	request->comp_image_set_ver_str_len = comp_image_set_ver_str_len;

	memcpy(msg->payload + sizeof(struct pldm_request_update_req),
	       comp_img_set_ver_str->ptr, comp_img_set_ver_str->length);

	return PLDM_SUCCESS;
}

int decode_request_update_resp(const struct pldm_msg *msg,
			       size_t payload_length, uint8_t *completion_code,
			       uint16_t *fd_meta_data_len,
			       uint8_t *fd_will_send_pkg_data)
{
	if (msg == NULL || completion_code == NULL ||
	    fd_meta_data_len == NULL || fd_will_send_pkg_data == NULL ||
	    !payload_length) {
		return PLDM_ERROR_INVALID_DATA;
	}

	*completion_code = msg->payload[0];
	if (*completion_code != PLDM_SUCCESS) {
		return PLDM_SUCCESS;
	}

	if (payload_length != sizeof(struct pldm_request_update_resp)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_request_update_resp *response =
	    (struct pldm_request_update_resp *)msg->payload;

	*fd_meta_data_len = le16toh(response->fd_meta_data_len);
	*fd_will_send_pkg_data = response->fd_will_send_pkg_data;

	return PLDM_SUCCESS;
}

/** @brief Check whether Component Response Code is valid
 *
 *  @return true if is from below mentioned values, false if not
 */
static bool check_resp_code_valid(const uint8_t comp_resp_code)
{
	switch (comp_resp_code) {
	case COMP_CAN_BE_UPDATED:
	case COMP_COMPARISON_STAMP_IDENTICAL:
	case COMP_COMPARISON_STAMP_LOWER:
	case INVALID_COMP_COMPARISON_STAMP:
	case COMP_CONFLICT:
	case COMP_PREREQUISITES:
	case COMP_NOT_SUPPORTED:
	case COMP_SECURITY_RESTRICTIONS:
	case INCOMPLETE_COMP_IMAGE_SET:
	case COMP_VER_STR_IDENTICAL:
	case COMP_VER_STR_LOWER:
	case FD_DOWN_STREAM_DEVICE_NOT_UPDATE_SUBSEQUENTLY:
		return true;

	default:
		if (comp_resp_code >= FD_VENDOR_COMP_STATUS_CODE_RANGE_MIN &&
		    comp_resp_code <= FD_VENDOR_COMP_STATUS_CODE_RANGE_MAX) {
			return true;
		}
		return false;
	}
}

/** @brief Check whether Component Response is valid
 *
 *  @return true if is from below mentioned values, false if not
 */
static bool check_comp_resp_valid(const uint8_t comp_resp)
{
	switch (comp_resp) {
	case PLDM_COMP_CAN_BE_UPDATEABLE:
	case PLDM_COMP_MAY_BE_UPDATEABLE:
		return true;

	default:
		return false;
	}
}

/** @brief Check whether Component Classification is valid
 *
 *  @return true if it is from below mentioned values, false if not
 */
static bool check_comp_classification_valid(const uint16_t comp_classification)
{
	switch (comp_classification) {
	case PLDM_COMP_UNKNOWN:
	case PLDM_COMP_OTHER:
	case PLDM_COMP_DRIVER:
	case PLDM_COMP_CONFIGURATION_SOFTWARE:
	case PLDM_COMP_APPLICATION_SOFTWARE:
	case PLDM_COMP_INSTRUMENTATION:
	case PLDM_COMP_FIRMWARE_OR_BIOS:
	case PLDM_COMP_DIAGNOSTIC_SOFTWARE:
	case PLDM_COMP_OPERATING_SYSTEM:
	case PLDM_COMP_MIDDLEWARE:
	case PLDM_COMP_FIRMWARE:
	case PLDM_COMP_BIOS_OR_FCODE:
	case PLDM_COMP_SUPPORT_OR_SERVICEPACK:
	case PLDM_COMP_SOFTWARE_BUNDLE:
	case PLDM_COMP_DOWNSTREAM_DEVICE:
		return true;

	default:
		return false;
	}
}

/** @brief Check whether Request Update Request is valid
 *
 *  @return true if request is valid,false if not
 */
static int validate_pass_component_table_req(
    const struct pldm_pass_component_table_req *data)
{
	if (!check_comp_classification_valid(
		htole16(data->comp_classification))) {
		return PLDM_ERROR_INVALID_DATA;
	}
	if (!check_transfer_flag_valid(data->transfer_flag)) {
		return PLDM_INVALID_TRANSFER_OPERATION_FLAG;
	}
	if (!is_string_type_valid(data->comp_ver_str_type)) {
		return PLDM_ERROR_INVALID_DATA;
	}
	return PLDM_SUCCESS;
}

/** @brief Fill the Pass Component Table Request
 *
 */
static void
fill_pass_component_table_req(const struct pldm_pass_component_table_req *data,
			      struct pldm_pass_component_table_req *request)
{
	request->transfer_flag = data->transfer_flag;
	request->comp_classification = htole16(data->comp_classification);
	request->comp_identifier = htole16(data->comp_identifier);
	request->comp_classification_index = data->comp_classification_index;
	request->comp_comparison_stamp = htole32(data->comp_comparison_stamp);
	request->comp_ver_str_type = data->comp_ver_str_type;
	request->comp_ver_str_len = data->comp_ver_str_len;
}

int encode_pass_component_table_req(
     uint8_t instance_id, struct pldm_msg *msg,
     size_t payload_length,
    const struct pldm_pass_component_table_req *data,
    struct variable_field *comp_ver_str)
{
	if (msg == NULL || data == NULL || comp_ver_str == NULL ||
	    comp_ver_str->ptr == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length != sizeof(struct pldm_pass_component_table_req) +
				  comp_ver_str->length) {
		return PLDM_ERROR_INVALID_LENGTH;
	}
	if (comp_ver_str->length != data->comp_ver_str_len) {
		return PLDM_ERROR_INVALID_DATA;
	}
	int rc = validate_pass_component_table_req(data);
	if ((PLDM_SUCCESS != rc)) {
		return rc;
	}

	rc = encode_pldm_header_only(PLDM_REQUEST, instance_id, PLDM_FWUP,
				     PLDM_PASS_COMPONENT_TABLE, msg);

	if (PLDM_SUCCESS != rc) {
		return rc;
	}

	struct pldm_pass_component_table_req *request =
	    (struct pldm_pass_component_table_req *)msg->payload;

	if (request == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	fill_pass_component_table_req(data, request);

	memcpy(msg->payload + sizeof(struct pldm_pass_component_table_req),
	       comp_ver_str->ptr, comp_ver_str->length);

	return PLDM_SUCCESS;
}

int decode_pass_component_table_resp(const struct pldm_msg *msg,
				     const size_t payload_length,
				     uint8_t *completion_code,
				     uint8_t *comp_resp,
				     uint8_t *comp_resp_code)
{
	if (msg == NULL || completion_code == NULL || comp_resp == NULL ||
	    comp_resp_code == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length != sizeof(struct pldm_pass_component_table_resp)) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	*completion_code = msg->payload[0];

	if (*completion_code != PLDM_SUCCESS) {
		return PLDM_SUCCESS;
	}

	struct pldm_pass_component_table_resp *response =
	    (struct pldm_pass_component_table_resp *)msg->payload;

	if (response == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (!check_comp_resp_valid(response->comp_resp)) {
		return PLDM_ERROR_INVALID_DATA;
	}

	*comp_resp = response->comp_resp;

	if (!check_resp_code_valid(response->comp_resp_code)) {
		return PLDM_ERROR_INVALID_DATA;
	}

	*comp_resp_code = response->comp_resp_code;

	return PLDM_SUCCESS;
}
