/*
 * Copyright 2026 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Subset of command request/response formats, full list:
 * https://chromium.googlesource.com/chromiumos/platform/ec/+/refs/heads/main/include/ec_commands.h
 */

#include <stdint.h>

#define EC_CMD_GET_VERSION 0x0002

struct ec_response_get_version {
	char version_string_ro[32];
	char version_string_rw[32];
	char reserved[32]; /* Changed to cros_fwid_ro in version 1 */
	uint32_t current_image;
} __packed __aligned(4);

struct ec_response_get_version_v1 {
	char version_string_ro[32];
	char version_string_rw[32];
	char cros_fwid_ro[32]; /* Added in version 1 (Used to be reserved) */
	uint32_t current_image;
	char cros_fwid_rw[32]; /* Added in version 1 */
} __packed __aligned(4);

#define EC_CMD_GET_CMD_VERSIONS 0x0008

struct ec_params_get_cmd_versions {
	uint8_t cmd;
} __packed;

struct ec_response_get_cmd_versions {
	uint32_t version_mask;
} __packed __aligned(4);

#define EC_CMD_GET_PROTOCOL_INFO 0x000B

struct ec_response_get_protocol_info {
	/* Fields which exist if at least protocol version 3 supported */
	uint32_t protocol_versions;
	uint16_t max_request_packet_size;
	uint16_t max_response_packet_size;
	uint32_t flags;
} __packed __aligned(4);

#define EC_PROTOCOL_INFO_IN_PROGRESS_SUPPORTED BIT(0)

#define EC_CMD_GET_FEATURES 0x000D

struct ec_response_get_features {
	uint32_t flags[2];
} __packed __aligned(4);

#define EC_CMD_MKBP_INFO 0x0061

struct ec_response_mkbp_info {
	uint32_t rows;
	uint32_t cols;
	uint8_t reserved;
} __packed;

struct ec_params_mkbp_info {
	uint8_t info_type;
	uint8_t event_type;
} __packed;

enum ec_mkbp_info_type {
	EC_MKBP_INFO_SUPPORTED = 1,
};

enum ec_mkbp_event {
	EC_MKBP_EVENT_KEY_MATRIX = 0,
	EC_MKBP_EVENT_BUTTON = 3,
	EC_MKBP_EVENT_SWITCH = 4,
};

union ec_response_get_next_data {
	uint8_t key_matrix[13];
	uint32_t host_event;
	uint64_t host_event64;
	uint32_t buttons;
	uint32_t switches;
	uint32_t fp_events;
	uint32_t sysrq;
	uint32_t cec_events;
} __packed;

#define EC_CMD_GET_NEXT_EVENT 0x0067

#define EC_MKBP_HAS_MORE_EVENTS_SHIFT 7
#define EC_MKBP_HAS_MORE_EVENTS BIT(EC_MKBP_HAS_MORE_EVENTS_SHIFT)
#define EC_MKBP_EVENT_TYPE_MASK (BIT(EC_MKBP_HAS_MORE_EVENTS_SHIFT) - 1)

union ec_response_get_next_data_v3 {
	uint8_t key_matrix[18];
	uint32_t host_event;
	uint64_t host_event64;
	uint32_t buttons;
	uint32_t switches;
	uint32_t fp_events;
	uint32_t sysrq;
	uint32_t cec_events;
	uint8_t cec_message[16];
} __packed;

struct ec_response_get_next_event_v3 {
	uint8_t event_type;
	union ec_response_get_next_data_v3 data;
} __packed;

#define EC_CMD_GET_UPTIME_INFO 0x0121

struct ec_response_uptime_info {
	uint32_t time_since_ec_boot_ms;
	uint32_t ap_resets_since_ec_boot;
	uint32_t ec_reset_flags;
	struct ap_reset_log_entry {
		uint16_t reset_cause;
		uint16_t reserved;
		uint32_t reset_time_ms;
	} recent_ap_reset[4];
} __packed __aligned(4);
