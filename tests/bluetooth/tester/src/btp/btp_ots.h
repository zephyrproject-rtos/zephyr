/* btp_ots.h - Bluetooth OTS tester headers */

/*
 * Copyright (c) 2024 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/sys/util.h>

/* OTS commands */
#define BTP_OTS_READ_SUPPORTED_COMMANDS	0x01
struct btp_ots_read_supported_commands_rp {
	FLEXIBLE_ARRAY_DECLARE(uint8_t, data);
} __packed;

#define BTP_OTS_REGISTER_OBJECT_FLAGS_SKIP_UNSUPPORTED_PROPS 0x01

#define BTP_OTS_REGISTER_OBJECT	0x02
struct btp_ots_register_object_cmd {
	uint8_t flags;
	uint32_t ots_props;
	uint32_t alloc_size;
	uint32_t current_size;
	uint8_t name_len;
	uint8_t name[];
} __packed;
struct btp_ots_register_object_rp {
	uint64_t object_id;
} __packed;
