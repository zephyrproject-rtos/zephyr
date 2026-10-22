/*
 * Copyright (c) 2024 Grant Ramsay <grant.ramsay@hotmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/byteorder.h>
#include <zephyr/random/random.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ssh, CONFIG_SSH_LOG_LEVEL);

#include "ssh_pkt.h"

/* Checks that adding the given length to the payload will not exceed
 * the payload size or overflow.
 */
static inline bool ssh_payload_len_in_range(const struct ssh_payload *payload, uint32_t len)
{
	return IN_RANGE(payload->len + len, payload->len, payload->size);
}

bool ssh_payload_write_byte(struct ssh_payload *payload, uint8_t data)
{
	if (!ssh_payload_len_in_range(payload, 1)) {
		return false;
	}

	if (payload->data != NULL) {
		payload->data[payload->len] = data;
	}

	payload->len += 1;

	return true;
}

bool ssh_payload_read_byte(struct ssh_payload *payload, uint8_t *data)
{
	if (!ssh_payload_len_in_range(payload, 1)) {
		return false;
	}

	if (data != NULL) {
		*data = payload->data[payload->len];
	}

	payload->len += 1;

	return true;
}

bool ssh_payload_write_bool(struct ssh_payload *payload, bool data)
{
	return ssh_payload_write_byte(payload, data ? 1 : 0);
}

bool ssh_payload_read_bool(struct ssh_payload *payload, bool *data)
{
	uint8_t b;

	if (!ssh_payload_read_byte(payload, &b)) {
		return false;
	}

	if (data != NULL) {
		*data = b;
	}

	return true;
}

bool ssh_payload_write_u32(struct ssh_payload *payload, uint32_t data)
{
	if (!ssh_payload_len_in_range(payload, 4)) {
		return false;
	}

	if (payload->data != NULL) {
		sys_put_be32(data, &payload->data[payload->len]);
	}

	payload->len += 4;

	return true;
}

bool ssh_payload_read_u32(struct ssh_payload *payload, uint32_t *data)
{
	if (!ssh_payload_len_in_range(payload, 4)) {
		return false;
	}

	if (data != NULL) {
		*data = sys_get_be32(&payload->data[payload->len]);
	}

	payload->len += 4;

	return true;
}

bool ssh_payload_write_u64(struct ssh_payload *payload, uint64_t data)
{
	if (!ssh_payload_len_in_range(payload, 8)) {
		return false;
	}

	if (payload->data != NULL) {
		sys_put_be64(data, &payload->data[payload->len]);
	}

	payload->len += 8;

	return true;
}

bool ssh_payload_read_u64(struct ssh_payload *payload, uint64_t *data)
{
	if (!ssh_payload_len_in_range(payload, 8)) {
		return false;
	}

	if (data != NULL) {
		*data = sys_get_be64(&payload->data[payload->len]);
	}

	payload->len += 8;

	return true;
}

bool ssh_payload_write_raw(struct ssh_payload *payload, const uint8_t *data, uint32_t len)
{
	if (!ssh_payload_len_in_range(payload, len)) {
		return false;
	}

	if (payload->data != NULL) {
		memcpy(&payload->data[payload->len], data, len);
	}

	payload->len += len;

	return true;
}

bool ssh_payload_read_raw(struct ssh_payload *payload, const uint8_t **data, uint32_t len)
{
	if (!ssh_payload_len_in_range(payload, len)) {
		return false;
	}

	if (data != NULL) {
		*data = &payload->data[payload->len];
	}

	payload->len += len;

	return true;
}

bool ssh_payload_write_string(struct ssh_payload *payload, const struct ssh_string *string)
{
	return ssh_payload_write_u32(payload, string->len) &&
	       ssh_payload_write_raw(payload, string->data, string->len);
}

bool ssh_payload_write_cstring(struct ssh_payload *payload, const char *str)
{
	struct ssh_string ssh_str = {
		.data = str,
		.len = strlen(str)
	};

	return ssh_payload_write_string(payload, &ssh_str);
}

bool ssh_payload_read_string(struct ssh_payload *payload, struct ssh_string *string)
{
	struct ssh_string tmp_string;

	if (string == NULL) {
		string = &tmp_string;
	}

	return ssh_payload_read_u32(payload, &string->len) &&
	       ssh_payload_read_raw(payload, &string->data, string->len);
}

bool ssh_payload_write_mpint(struct ssh_payload *payload, const uint8_t *data,
			     uint32_t len, bool is_signed)
{
	const uint8_t sign_byte = is_signed ? 0xFF : 0;
	uint32_t first_byte = len;
	bool prepend_sign;

	/* Drop leading zeros/0xFFs depending on the sign */

	for (uint32_t i = 0; i < len; i++) {
		if (data[i] != sign_byte) {
			first_byte = i;
			break;
		}
	}

	len -= first_byte;

	/* Prepend a byte to set the sign if needed */
	prepend_sign = false;
	if (len > 0) {
		bool data_is_signed = (data[first_byte] & 0x80) != 0;

		if (is_signed != data_is_signed) {
			prepend_sign = true;
		}
	}

	if (!ssh_payload_write_u32(payload, prepend_sign ? len + 1 : len)) {
		return false;
	}

	if (prepend_sign) {
		if (!ssh_payload_write_byte(payload, sign_byte)) {
			return false;
		}
	}

	return ssh_payload_write_raw(payload, &data[first_byte], len);
}

bool ssh_payload_read_mpint(struct ssh_payload *payload, const uint8_t **data, uint32_t *len)
{
	uint32_t tmp_len;

	if (len == NULL) {
		len = &tmp_len;
	}

	return ssh_payload_read_u32(payload, len) &&
	       ssh_payload_read_raw(payload, data, *len);
}

bool ssh_payload_write_name_list(struct ssh_payload *payload,
				 const struct ssh_string *names, uint32_t n)
{
	/* Skip the length field, write it at the end once it is known */
	uint32_t start_offset = payload->len;

	if (!ssh_payload_skip_bytes(payload, 4)) {
		return false;
	}

	while (n--) {
		if (!ssh_payload_write_raw(payload, names->data, names->len)) {
			return false;
		}

		names++;

		if (n > 0 && !ssh_payload_write_byte(payload, ',')) {
			return false;
		}
	}

	if (payload->data != NULL) {
		uint32_t len = payload->len - start_offset - 4;

		sys_put_be32(len, &payload->data[start_offset]);
	}

	return true;
}

bool ssh_payload_read_name_list(struct ssh_payload *payload, struct ssh_payload *name_list)
{
	struct ssh_string string;
	bool ret = ssh_payload_read_u32(payload, &string.len) &&
		   ssh_payload_read_raw(payload, &string.data, string.len);

	if (ret && name_list != NULL) {
		*name_list = (struct ssh_payload) {
			.size = string.len,
			.len = 0,
			.data = (void *)string.data
		};
	}

	return ret;
}

bool ssh_payload_write_csrand(struct ssh_payload *payload, uint32_t len)
{
	if (!ssh_payload_len_in_range(payload, len)) {
		return false;
	}

	if (payload->data != NULL) {
		if (sys_csrand_get(&payload->data[payload->len], len) != 0) {
			return false;
		}
	}

	payload->len += len;

	return true;
}

bool ssh_payload_skip_bytes(struct ssh_payload *payload, uint32_t len)
{
	if (!ssh_payload_len_in_range(payload, len)) {
		return false;
	}

	payload->len += len;

	return true;
}

bool ssh_payload_name_list_iter(struct ssh_payload *name_list, struct ssh_string *name_out)
{
	const uint8_t *comma;
	size_t remaining;

	if (name_list->len >= name_list->size) {
		return false;
	}

	remaining = name_list->size - name_list->len;
	name_out->data = &name_list->data[name_list->len];

	comma = memchr(name_out->data, ',', remaining);
	if (comma == NULL) {
		name_out->len = remaining;
	} else {
		name_out->len = (uintptr_t)(comma - name_out->data);
	}

	name_list->len += name_out->len + 1;

	return true;
}

struct ssh_string *ssh_payload_string_alloc(struct sys_heap *heap, const void *data, uint32_t len)
{
	struct ssh_string *str = sys_heap_alloc(heap, sizeof(*str) + len);

	if (str == NULL) {
		return NULL;
	}

	*str = (struct ssh_string) {
		.len = len,
		.data = (uint8_t *)str + sizeof(*str)
	};

	if (data != NULL) {
		memcpy((void *)str->data, data, len);
	}

	return str;
}
