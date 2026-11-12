/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "canopen_internal.h"

#define CANOPEN_OD_COMM_FIRST 0x1000U
#define CANOPEN_OD_COMM_LAST  0x1FFFU

static bool od_is_comm(uint16_t index)
{
	return (index >= CANOPEN_OD_COMM_FIRST) && (index <= CANOPEN_OD_COMM_LAST);
}

size_t canopen_od_type_size(uint8_t type)
{
	switch (type) {
	case CANOPEN_OD_TYPE_BOOLEAN:
	case CANOPEN_OD_TYPE_INTEGER8:
	case CANOPEN_OD_TYPE_UNSIGNED8:
		return 1U;
	case CANOPEN_OD_TYPE_INTEGER16:
	case CANOPEN_OD_TYPE_UNSIGNED16:
		return 2U;
	case CANOPEN_OD_TYPE_INTEGER24:
	case CANOPEN_OD_TYPE_UNSIGNED24:
		return 3U;
	case CANOPEN_OD_TYPE_INTEGER32:
	case CANOPEN_OD_TYPE_UNSIGNED32:
	case CANOPEN_OD_TYPE_REAL32:
		return 4U;
	case CANOPEN_OD_TYPE_INTEGER40:
	case CANOPEN_OD_TYPE_UNSIGNED40:
		return 5U;
	case CANOPEN_OD_TYPE_INTEGER48:
	case CANOPEN_OD_TYPE_UNSIGNED48:
		return 6U;
	case CANOPEN_OD_TYPE_INTEGER56:
	case CANOPEN_OD_TYPE_UNSIGNED56:
		return 7U;
	case CANOPEN_OD_TYPE_INTEGER64:
	case CANOPEN_OD_TYPE_UNSIGNED64:
	case CANOPEN_OD_TYPE_REAL64:
		return 8U;
	default:
		return 0U;
	}
}

static bool od_is_signed(uint8_t type)
{
	switch (type) {
	case CANOPEN_OD_TYPE_INTEGER8:
	case CANOPEN_OD_TYPE_INTEGER16:
	case CANOPEN_OD_TYPE_INTEGER24:
	case CANOPEN_OD_TYPE_INTEGER32:
	case CANOPEN_OD_TYPE_INTEGER40:
	case CANOPEN_OD_TYPE_INTEGER48:
	case CANOPEN_OD_TYPE_INTEGER56:
	case CANOPEN_OD_TYPE_INTEGER64:
		return true;
	default:
		return false;
	}
}

static bool od_native_size_valid(uint8_t type, uint16_t size)
{
	size_t wire = canopen_od_type_size(type);

	if (wire == 0U) {
		return size > 0U;
	}

	return ((size == 1U) || (size == 2U) || (size == 4U) || (size == 8U)) && (size >= wire);
}

static uint64_t od_native_load(const void *src, uint16_t size)
{
	uint8_t u8;
	uint16_t u16;
	uint32_t u32;
	uint64_t u64;

	switch (size) {
	case 1U:
		memcpy(&u8, src, sizeof(u8));
		return u8;
	case 2U:
		memcpy(&u16, src, sizeof(u16));
		return u16;
	case 4U:
		memcpy(&u32, src, sizeof(u32));
		return u32;
	default:
		memcpy(&u64, src, sizeof(u64));
		return u64;
	}
}

static void od_native_store(void *dst, uint16_t size, uint64_t value)
{
	uint8_t u8 = (uint8_t)value;
	uint16_t u16 = (uint16_t)value;
	uint32_t u32 = (uint32_t)value;

	switch (size) {
	case 1U:
		memcpy(dst, &u8, sizeof(u8));
		break;
	case 2U:
		memcpy(dst, &u16, sizeof(u16));
		break;
	case 4U:
		memcpy(dst, &u32, sizeof(u32));
		break;
	default:
		memcpy(dst, &value, sizeof(value));
		break;
	}
}

size_t canopen_od_wire_size(const struct canopen_od_ref *ref)
{
	size_t wire = canopen_od_type_size(ref->type);

	return (wire != 0U) ? wire : ref->size;
}

uint32_t canopen_od_read_wire(const struct canopen_od_ref *ref, uint8_t *buf, size_t max,
			      size_t *len)
{
	size_t wire = canopen_od_type_size(ref->type);
	uint8_t le[sizeof(uint64_t)];
	const uint8_t *nul;

	if (wire == 0U) {
		wire = ref->size;
		if (ref->type == CANOPEN_OD_TYPE_VISIBLE_STRING) {
			nul = memchr(ref->src, '\0', ref->size);
			if (nul != NULL) {
				wire = nul - (const uint8_t *)ref->src;
			}
		}
	}

	if (wire > max) {
		return CANOPEN_SDO_ABORT_OUT_OF_MEMORY;
	}

	if (canopen_od_type_size(ref->type) != 0U) {
		sys_put_le64(od_native_load(ref->src, ref->size), le);
		memcpy(buf, le, wire);
	} else {
		memcpy(buf, ref->src, wire);
	}

	*len = wire;

	return CANOPEN_SDO_ABORT_NONE;
}

uint32_t canopen_od_write_wire(struct canopen_node *node, const struct canopen_od_ref *ref,
			       const uint8_t *buf, size_t len)
{
	size_t wire = canopen_od_type_size(ref->type);
	uint64_t value = 0U;
	uint64_t native = 0U;
	const void *data = buf;
	uint32_t sdo_abort;

	if (wire != 0U) {
		if (len != wire) {
			return (len > wire) ? CANOPEN_SDO_ABORT_LENGTH_TOO_HIGH
					    : CANOPEN_SDO_ABORT_LENGTH_TOO_LOW;
		}

		for (size_t i = wire; i > 0U; i--) {
			value = (value << 8) | buf[i - 1U];
		}

		if (od_is_signed(ref->type) && (wire < sizeof(uint64_t)) &&
		    ((value & BIT64((wire * 8U) - 1U)) != 0U)) {
			value |= GENMASK64(63U, wire * 8U);
		}

		if ((ref->type == CANOPEN_OD_TYPE_BOOLEAN) && (value > 1U)) {
			return CANOPEN_SDO_ABORT_INVALID_VALUE;
		}

		od_native_store(&native, ref->size, value);
		data = &native;
		len = ref->size;
	} else if (len > ref->size) {
		return CANOPEN_SDO_ABORT_LENGTH_TOO_HIGH;
	} else if ((ref->type != CANOPEN_OD_TYPE_VISIBLE_STRING) && (len < ref->size)) {
		return CANOPEN_SDO_ABORT_LENGTH_TOO_LOW;
	}

	if (od_is_comm(ref->index)) {
		return canopen_od_comm_write(node, ref, (uint32_t)value);
	}

	if ((ref->entry != NULL) && (ref->entry->write_hook != NULL)) {
		sdo_abort = ref->entry->write_hook(ref->entry, ref->sub, data, len,
						   ref->entry->user_data);
		if (sdo_abort != CANOPEN_SDO_ABORT_NONE) {
			return sdo_abort;
		}
	}

	memcpy(ref->dst, data, len);
	if (len < ref->size) {
		memset((uint8_t *)ref->dst + len, 0, ref->size - len);
	}

	return CANOPEN_SDO_ABORT_NONE;
}

static void od_ref_set(struct canopen_od_ref *ref, const struct canopen_od_entry *entry,
		       const struct canopen_od_sub *sub, uint8_t sub_index, void *data)
{
	ref->entry = entry;
	ref->src = data;
	ref->dst = data;
	ref->size = sub->size;
	ref->index = entry->index;
	ref->sub = sub_index;
	ref->type = sub->type;
	ref->attr = sub->attr;
}

static void od_ref_set_u8(struct canopen_od_ref *ref, const struct canopen_od_entry *entry,
			  const uint8_t *value)
{
	ref->entry = entry;
	ref->src = value;
	ref->dst = NULL;
	ref->size = sizeof(uint8_t);
	ref->index = entry->index;
	ref->sub = 0U;
	ref->type = CANOPEN_OD_TYPE_UNSIGNED8;
	ref->attr = CANOPEN_OD_CONST;
}

static const struct canopen_od_entry *od_app_lookup(const struct canopen_od *od, uint16_t index)
{
	size_t lo = 0U;
	size_t hi = od->count;

	while (lo < hi) {
		size_t mid = lo + ((hi - lo) / 2U);
		const struct canopen_od_entry *entry = &od->entries[mid];

		if (entry->index == index) {
			return entry;
		}

		if (entry->index < index) {
			lo = mid + 1U;
		} else {
			hi = mid;
		}
	}

	return NULL;
}

static uint32_t od_app_find(const struct canopen_od *od, uint16_t index, uint8_t sub,
			    struct canopen_od_ref *ref)
{
	const struct canopen_od_entry *entry = od_app_lookup(od, index);
	const struct canopen_od_sub *s;

	if (entry == NULL) {
		return CANOPEN_SDO_ABORT_NO_OBJECT;
	}

	switch (entry->code) {
	case CANOPEN_OD_CODE_VAR:
		if (sub != 0U) {
			return CANOPEN_SDO_ABORT_NO_SUB_INDEX;
		}
		od_ref_set(ref, entry, &entry->subs[0], sub, entry->subs[0].data);
		return CANOPEN_SDO_ABORT_NONE;
	case CANOPEN_OD_CODE_ARRAY:
		if (sub == 0U) {
			od_ref_set_u8(ref, entry, &entry->count);
			return CANOPEN_SDO_ABORT_NONE;
		}
		if (sub > entry->count) {
			return CANOPEN_SDO_ABORT_NO_SUB_INDEX;
		}
		s = &entry->subs[0];
		od_ref_set(ref, entry, s, sub, (uint8_t *)s->data + ((sub - 1U) * s->size));
		return CANOPEN_SDO_ABORT_NONE;
	default:
		if ((sub == 0U) && (entry->subs[0].sub != 0U)) {
			od_ref_set_u8(ref, entry, &entry->subs[entry->count - 1U].sub);
			return CANOPEN_SDO_ABORT_NONE;
		}
		for (uint8_t i = 0U; i < entry->count; i++) {
			s = &entry->subs[i];
			if (s->sub == sub) {
				od_ref_set(ref, entry, s, sub, s->data);
				return CANOPEN_SDO_ABORT_NONE;
			}
		}
		return CANOPEN_SDO_ABORT_NO_SUB_INDEX;
	}
}

uint32_t canopen_od_find(struct canopen_node *node, uint16_t index, uint8_t sub,
			 struct canopen_od_ref *ref)
{
	if (od_is_comm(index)) {
		return canopen_od_comm_find(node, index, sub, ref);
	}

	if (node->cfg->od == NULL) {
		return CANOPEN_SDO_ABORT_NO_OBJECT;
	}

	return od_app_find(node->cfg->od, index, sub, ref);
}

int canopen_od_validate(const struct canopen_od *od)
{
	for (size_t i = 0U; i < od->count; i++) {
		const struct canopen_od_entry *entry = &od->entries[i];

		if (od_is_comm(entry->index) || (entry->count == 0U) ||
		    ((i > 0U) && (od->entries[i - 1U].index >= entry->index))) {
			return -EINVAL;
		}

		for (uint8_t j = 0U; j < entry->count; j++) {
			const struct canopen_od_sub *s = &entry->subs[j];

			if ((s->type == CANOPEN_OD_TYPE_DOMAIN) ||
			    !od_native_size_valid(s->type, s->size)) {
				return -EINVAL;
			}

			if ((entry->code == CANOPEN_OD_CODE_RECORD) && (j > 0U) &&
			    (entry->subs[j - 1U].sub >= s->sub)) {
				return -EINVAL;
			}

			if (entry->code == CANOPEN_OD_CODE_ARRAY) {
				break;
			}
		}
	}

	return 0;
}

int canopen_od_read(struct canopen_node *node, uint16_t index, uint8_t sub, void *buf, size_t len)
{
	struct canopen_od_ref ref;
	int err = 0;

	k_mutex_lock(&node->lock, K_FOREVER);

	if (canopen_od_find(node, index, sub, &ref) != CANOPEN_SDO_ABORT_NONE) {
		err = -ENOENT;
	} else if (len != ref.size) {
		err = -EINVAL;
	} else {
		memcpy(buf, ref.src, len);
	}

	k_mutex_unlock(&node->lock);

	return err;
}

int canopen_od_write(struct canopen_node *node, uint16_t index, uint8_t sub, const void *buf,
		     size_t len)
{
	struct canopen_od_ref ref;
	int err = 0;

	k_mutex_lock(&node->lock, K_FOREVER);

	if (canopen_od_find(node, index, sub, &ref) != CANOPEN_SDO_ABORT_NONE) {
		err = -ENOENT;
	} else if (od_is_comm(index) ? ((ref.attr & CANOPEN_OD_ATTR_WRITE) == 0U)
				     : (ref.dst == NULL)) {
		err = -EACCES;
	} else if (len != ref.size) {
		err = -EINVAL;
	} else if (od_is_comm(index)) {
		if (canopen_od_comm_write(node, &ref, (uint32_t)od_native_load(buf, ref.size)) !=
		    CANOPEN_SDO_ABORT_NONE) {
			err = -EINVAL;
		}
	} else {
		memcpy(ref.dst, buf, len);
	}

	k_mutex_unlock(&node->lock);

	if (err == 0) {
		k_poll_signal_raise(&node->signal, 0);
	}

	return err;
}
