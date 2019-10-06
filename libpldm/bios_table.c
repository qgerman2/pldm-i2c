#include <assert.h>
#include <endian.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "bios.h"
#include "bios_table.h"

int pldm_bios_table_attr_entry_enum_decode_pv_num(
    const struct pldm_bios_attr_table_entry *entry, uint8_t *pv_num)
{
	*pv_num = entry->metadata[0];
	return PLDM_SUCCESS;
}

int pldm_bios_table_attr_entry_enum_decode_def_num(
    const struct pldm_bios_attr_table_entry *entry, uint8_t *def_num)
{
	uint8_t pv_num;
	pldm_bios_table_attr_entry_enum_decode_pv_num(entry, &pv_num);
	*def_num = entry->metadata[sizeof(uint8_t) /* pv_num */ +
				   sizeof(uint16_t) * pv_num];
	return PLDM_SUCCESS;
}

/** @brief Get length of an enum attribute entry
 */
static size_t
attr_table_entry_length_enum(const struct pldm_bios_attr_table_entry *entry)
{
	uint8_t pv_num, def_num;
	pldm_bios_table_attr_entry_enum_decode_pv_num(entry, &pv_num);
	pldm_bios_table_attr_entry_enum_decode_def_num(entry, &def_num);
	return sizeof(*entry) - 1 + sizeof(pv_num) + pv_num * sizeof(uint16_t) +
	       sizeof(def_num) + def_num;
}

#define ATTR_ENTRY_STRING_LENGTH_MIN                                           \
	sizeof(uint8_t) /* string type */ +                                    \
	    sizeof(uint16_t) /* minimum string length */ +                     \
	    sizeof(uint16_t) /* maximum string length */ +                     \
	    sizeof(uint16_t) /* default string length */

int pldm_bios_table_attr_entry_string_decode_def_string_length(
    const struct pldm_bios_attr_table_entry *entry, uint16_t *def_string_length)
{
	int def_string_pos = ATTR_ENTRY_STRING_LENGTH_MIN - sizeof(uint16_t);
	*def_string_length = *(uint16_t *)(entry->metadata + def_string_pos);
	*def_string_length = le16toh(*def_string_length);
	return PLDM_SUCCESS;
}

/** @brief Get length of a string attribute entry
 */
static size_t
attr_table_entry_length_string(const struct pldm_bios_attr_table_entry *entry)
{
	uint16_t def_string_len;
	pldm_bios_table_attr_entry_string_decode_def_string_length(
	    entry, &def_string_len);
	return sizeof(*entry) - 1 + ATTR_ENTRY_STRING_LENGTH_MIN +
	       def_string_len;
}

struct attr_table_entry {
	uint8_t attr_type;
	size_t (*entry_length_handler)(
	    const struct pldm_bios_attr_table_entry *);
};

static struct attr_table_entry attr_table_entrys[] = {
    {.attr_type = PLDM_BIOS_ENUMERATION,
     .entry_length_handler = attr_table_entry_length_enum},
    {.attr_type = PLDM_BIOS_ENUMERATION_READ_ONLY,
     .entry_length_handler = attr_table_entry_length_enum},
    {.attr_type = PLDM_BIOS_STRING,
     .entry_length_handler = attr_table_entry_length_string},
    {.attr_type = PLDM_BIOS_STRING_READ_ONLY,
     .entry_length_handler = attr_table_entry_length_string},
};

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

static struct attr_table_entry *find_attr_table_entry_by_type(uint8_t attr_type)
{
	size_t i;
	for (i = 0; i < ARRAY_SIZE(attr_table_entrys); i++) {
		if (attr_type == attr_table_entrys[i].attr_type)
			return &attr_table_entrys[i];
	}
	return NULL;
}

static size_t
attr_table_entry_length(const struct pldm_bios_attr_table_entry *entry)
{
	struct attr_table_entry *attr_table_entry =
	    find_attr_table_entry_by_type(entry->attr_type);
	assert(attr_table_entry != NULL);
	assert(attr_table_entry->entry_length_handler != NULL);

	return attr_table_entry->entry_length_handler(entry);
}

struct pldm_bios_table_iter {
	const uint8_t *table_data;
	size_t table_len;
	size_t next_pos;
};

struct pldm_bios_table_iter *pldm_bios_table_iter_create(const uint8_t *table,
							 size_t length)
{
	struct pldm_bios_table_iter *iter = malloc(sizeof(*iter));
	assert(iter != NULL);
	iter->table_data = table;
	iter->table_len = length;
	iter->next_pos = 0;

	return iter;
}

void pldm_bios_table_iter_free(struct pldm_bios_table_iter *iter)
{
	free(iter);
}

#define pad_and_check_max 7
bool pldm_bios_table_attr_iter_is_end(const struct pldm_bios_table_iter *iter)
{
	if (iter->table_len - iter->next_pos <= pad_and_check_max)
		return true;
	return false;
}

const struct pldm_bios_attr_table_entry *
pldm_bios_table_attr_entry_next(struct pldm_bios_table_iter *iter)
{
	if (pldm_bios_table_attr_iter_is_end(iter))
		return NULL;
	struct pldm_bios_attr_table_entry *entry =
	    (struct pldm_bios_attr_table_entry *)(iter->table_data +
						  iter->next_pos);
	iter->next_pos += attr_table_entry_length(entry);

	return entry;
}