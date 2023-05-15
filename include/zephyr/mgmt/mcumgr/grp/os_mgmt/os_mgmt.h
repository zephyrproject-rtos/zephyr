/*
 * Copyright (c) 2018-2021 mcumgr authors
 * Copyright (c) 2022 Laird Connectivity
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_OS_MGMT_
#define H_OS_MGMT_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Command IDs for OS management group.
 */
#define OS_MGMT_ID_ECHO			0
#define OS_MGMT_ID_CONS_ECHO_CTRL	1
#define OS_MGMT_ID_TASKSTAT		2
#define OS_MGMT_ID_MPSTAT		3
#define OS_MGMT_ID_DATETIME_STR		4
#define OS_MGMT_ID_RESET		5
#define OS_MGMT_ID_MCUMGR_PARAMS	6
#define OS_MGMT_ID_INFO			7
#define OS_MGMT_ID_BOOTLOADER_INFO	8

/**
 * Command result codes for OS management group.
 */
enum os_mgmt_err_code_t {
	/** No error, this is implied if there is no ret value in the response */
	OS_MGMT_ERR_OK = 0,

	/** Unknown error occurred. */
	OS_MGMT_ERR_UNKNOWN,

	/** The provided format value is not valid. */
	OS_MGMT_ERR_INVALID_FORMAT,

	/** Query was not recognized. */
	OS_MGMT_ERR_QUERY_YIELDS_NO_ANSWER,
};

/* Bitmask values used by the os info command handler. Note that the width of this variable is
 * 32-bits, allowing 32 flags, custom user-level implementations should start at
 * OS_MGMT_INFO_FORMAT_USER_CUSTOM_START and reference that directly as additional format
 * specifiers might be added to this list in the future.
 */
enum os_mgmt_info_formats {
	OS_MGMT_INFO_FORMAT_KERNEL_NAME = BIT(0),
	OS_MGMT_INFO_FORMAT_NODE_NAME = BIT(1),
	OS_MGMT_INFO_FORMAT_KERNEL_RELEASE = BIT(2),
	OS_MGMT_INFO_FORMAT_KERNEL_VERSION = BIT(3),
	OS_MGMT_INFO_FORMAT_BUILD_DATE_TIME = BIT(4),
	OS_MGMT_INFO_FORMAT_MACHINE = BIT(5),
	OS_MGMT_INFO_FORMAT_PROCESSOR = BIT(6),
	OS_MGMT_INFO_FORMAT_HARDWARE_PLATFORM = BIT(7),
	OS_MGMT_INFO_FORMAT_OPERATING_SYSTEM = BIT(8),

	OS_MGMT_INFO_FORMAT_USER_CUSTOM_START = BIT(9),
};

/* Structure provided in the MGMT_EVT_OP_OS_MGMT_INFO_CHECK notification callback */
struct os_mgmt_info_check {
	/* Input format string from the mcumgr client */
	struct zcbor_string *format;
	/* Bitmask of values specifying which outputs should be present */
	uint32_t *format_bitmask;
	/* Number of valid format characters parsed, must be incremented by 1 for each valid
	 * character
	 */
	uint16_t *valid_formats;
	/* Needs to be set to true if the OS name is being provided by external code */
	bool *custom_os_name;
};

/* Structure provided in the MGMT_EVT_OP_OS_MGMT_INFO_APPEND notification callback */
struct os_mgmt_info_append {
	/* The format bitmask from the processed commands, the bits should be cleared once
	 * processed, note that if all_format_specified is specified, the corrisponding bits here
	 * will not be set
	 */
	uint32_t *format_bitmask;
	/* Will be true if the all 'a' specifier was provided */
	bool all_format_specified;
	/* The output buffer which the responses should be appended to. If prior_output is true, a
	 * space must be added prior to the output response
	 */
	uint8_t *output;
	/* The current size of the output response in the output buffer, must be updated to be the
	 * size of the output response after appending data
	 */
	uint16_t *output_length;
	/* The size of the output buffer, including null terminator character, if the output
	 * response would exceed this size, the function must abort and return false to return a
	 * memory error to the client
	 */
	uint16_t buffer_size;
	/* If there has been prior output, must be set to true if a response has been output */
	bool *prior_output;
};

#ifdef __cplusplus
}
#endif

#endif /* H_OS_MGMT_ */
