/*
 * Copyright (c) 2024 Grant Ramsay <grant.ramsay@hotmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_NET_LIB_SSH_SSH_PKT_H_
#define ZEPHYR_SUBSYS_NET_LIB_SSH_SSH_PKT_H_

#include "ssh_defs.h"

#include <zephyr/kernel.h>

#include <string.h>

/* Allocate an ssh_payload on the thread stack */
#define SSH_PAYLOAD_STACK(_name, _size)		\
	uint8_t _##_name##_buf[(_size)];	\
	SSH_PAYLOAD_BUF(_name, _##_name##_buf)

/* Init an ssh_payload from a buffer */
#define SSH_PAYLOAD_BUF(_name, _buf)		\
	(void)IS_ARRAY(_buf);			\
	struct ssh_payload _name = {		\
		.size = sizeof(_buf), .len = 0,	\
		.data = _buf }

#define SSH_STRING_LITERAL(_data) \
	{ .len = sizeof(_data) - 1, .data = (_data) }

struct ssh_payload {
	uint32_t size; /* readonly */
	uint32_t len;
	uint8_t *data;
};

struct ssh_string {
	uint32_t len;
	const uint8_t *data;
};

bool ssh_payload_write_byte(struct ssh_payload *payload, uint8_t data);
bool ssh_payload_read_byte(struct ssh_payload *payload, uint8_t *data);
bool ssh_payload_write_bool(struct ssh_payload *payload, bool data);
bool ssh_payload_read_bool(struct ssh_payload *payload, bool *data);
bool ssh_payload_write_u32(struct ssh_payload *payload, uint32_t data);
bool ssh_payload_read_u32(struct ssh_payload *payload, uint32_t *data);
bool ssh_payload_write_u64(struct ssh_payload *payload, uint64_t data);
bool ssh_payload_read_u64(struct ssh_payload *payload, uint64_t *data);
bool ssh_payload_write_raw(struct ssh_payload *payload, const uint8_t *data, uint32_t len);
bool ssh_payload_read_raw(struct ssh_payload *payload, const uint8_t **data, uint32_t len);
bool ssh_payload_write_string(struct ssh_payload *payload, const struct ssh_string *string);
bool ssh_payload_write_cstring(struct ssh_payload *payload, const char *str);
bool ssh_payload_read_string(struct ssh_payload *payload, struct ssh_string *string);
bool ssh_payload_write_mpint(struct ssh_payload *payload, const uint8_t *data, uint32_t len,
			     bool is_signed);
bool ssh_payload_read_mpint(struct ssh_payload *payload, const uint8_t **data, uint32_t *len);
bool ssh_payload_write_name_list(struct ssh_payload *payload, const struct ssh_string *names,
				 uint32_t n);
bool ssh_payload_read_name_list(struct ssh_payload *payload, struct ssh_payload *name_list);
bool ssh_payload_write_csrand(struct ssh_payload *payload, uint32_t len);
bool ssh_payload_skip_bytes(struct ssh_payload *payload, uint32_t len);
bool ssh_payload_name_list_iter(struct ssh_payload *name_list, struct ssh_string *name_out);
struct ssh_string *ssh_payload_string_alloc(struct sys_heap *heap, const void *data,
					    uint32_t len);

/* Check that there is no more data in the packet left to read */
static inline bool ssh_payload_read_complete(struct ssh_payload *payload)
{
	return payload->len == payload->size;
}

static inline bool ssh_strings_equal(const struct ssh_string *s1, const struct ssh_string *s2)
{
	return (s1->len == s2->len && memcmp(s1->data, s2->data, s1->len) == 0);
}

#endif
