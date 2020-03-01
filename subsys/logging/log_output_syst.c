/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <mipi_syst.h>
#include <logging/log.h>
#include <logging/log_ctrl.h>
#include <logging/log_output.h>

extern struct mipi_syst_handle log_syst_handle;
extern void update_systh_platform_data(
		struct mipi_syst_handle *handle,
		const struct log_output *log_output, u32_t flag);

/*
 *    0   MIPI_SYST_SEVERITY_MAX      no assigned severity
 *    1   MIPI_SYST_SEVERITY_FATAL    critical error level
 *    2   MIPI_SYST_SEVERITY_ERROR    error message level
 *    3   MIPI_SYST_SEVERITY_WARNING  warning message level
 *    4   MIPI_SYST_SEVERITY_INFO     information message level
 *    5   MIPI_SYST_SEVERITY_USER1    user defined level 5
 *    6   MIPI_SYST_SEVERITY_USER2    user defined level 6
 *    7   MIPI_SYST_SEVERITY_DEBUG    debug information level
 */
static u32_t level_to_syst_severity(u32_t level)
{
	u32_t ret;

	switch (level) {
	case LOG_LEVEL_NONE:
		ret = 0U;
		break;
	case LOG_LEVEL_ERR:
		ret = 2U;
		break;
	case LOG_LEVEL_WRN:
		ret = 3U;
		break;
	case LOG_LEVEL_INF:
		ret = 4U;
		break;
	case LOG_LEVEL_DBG:
		ret = 7U;
		break;
	default:
		ret = 7U;
		break;
	}

	return ret;
}

static void std_print(struct log_msg *msg,
		const struct log_output *log_output)
{
	const char *str = log_msg_str_get(msg);
	u32_t nargs = log_msg_nargs_get(msg);
	u32_t *args = alloca(sizeof(u32_t)*nargs);
	u32_t severity = level_to_syst_severity(log_msg_level_get(msg));

	for (int i = 0; i < nargs; i++) {
		args[i] = log_msg_arg_get(msg, i);
	}

	switch (log_msg_nargs_get(msg)) {
	case 0:
		MIPI_SYST_PRINTF(&log_syst_handle, severity, str);
		break;
	case 1:
		MIPI_SYST_PRINTF(&log_syst_handle, severity, str, args[0]);
		break;
	case 2:
		MIPI_SYST_PRINTF(&log_syst_handle, severity, str, args[0],
				args[1]);
		break;
	case 3:
		MIPI_SYST_PRINTF(&log_syst_handle, severity, str, args[0],
				args[1], args[2]);
		break;
	case 4:
		MIPI_SYST_PRINTF(&log_syst_handle, severity, str, args[0],
				args[1], args[2], args[3]);
		break;
	case 5:
		MIPI_SYST_PRINTF(&log_syst_handle, severity, str, args[0],
				args[1], args[2], args[3], args[4]);
		break;
	case 6:
		MIPI_SYST_PRINTF(&log_syst_handle, severity, str, args[0],
				args[1], args[2], args[3], args[4], args[5]);
		break;
	case 7:
		MIPI_SYST_PRINTF(&log_syst_handle, severity, str, args[0],
				args[1], args[2], args[3], args[4], args[5],
				args[6]);
		break;
	case 8:
		MIPI_SYST_PRINTF(&log_syst_handle, severity, str, args[0],
				args[1], args[2], args[3], args[4], args[5],
				args[6], args[7]);
		break;
	case 9:
		MIPI_SYST_PRINTF(&log_syst_handle, severity, str, args[0],
				args[1], args[2], args[3], args[4], args[5],
				args[6], args[7], args[8]);
		break;
	case 10:
		MIPI_SYST_PRINTF(&log_syst_handle, severity, str, args[0],
				args[1], args[2], args[3], args[4], args[5],
				args[6], args[7], args[8], args[9]);
		break;
	case 11:
		MIPI_SYST_PRINTF(&log_syst_handle, severity, str, args[0],
				args[1], args[2], args[3], args[4], args[5],
				args[6], args[7], args[8], args[9], args[10]);
		break;
	case 12:
		MIPI_SYST_PRINTF(&log_syst_handle, severity, str, args[0],
				args[1], args[2], args[3], args[4], args[5],
				args[6], args[7], args[8], args[9], args[10],
				args[11]);
		break;
	case 13:
		MIPI_SYST_PRINTF(&log_syst_handle, severity, str, args[0],
				args[1], args[2], args[3], args[4], args[5],
				args[6], args[7], args[8], args[9], args[10],
				args[11], args[12]);
		break;
	case 14:
		MIPI_SYST_PRINTF(&log_syst_handle, severity, str, args[0],
				args[1], args[2], args[3], args[4], args[5],
				args[6], args[7], args[8], args[9], args[10],
				args[11], args[12], args[13]);
		break;
	case 15:
		MIPI_SYST_PRINTF(&log_syst_handle, severity, str, args[0],
				args[1], args[2], args[3], args[4], args[5],
				args[6], args[7], args[8], args[9], args[10],
				args[11], args[12], args[13], args[14]);
		break;
	default:
		/* Unsupported number of arguments. */
		__ASSERT_NO_MSG(true);
		break;
	}
}

static void raw_string_print(struct log_msg *msg,
			const struct log_output *log_output)
{
	char buf[CONFIG_LOG_STRDUP_MAX_STRING + 1];
	size_t length = CONFIG_LOG_STRDUP_MAX_STRING;
	u32_t severity = level_to_syst_severity(log_msg_level_get(msg));

	log_msg_hexdump_data_get(msg, buf, &length, 0);

	buf[length] = '\0';

	MIPI_SYST_PRINTF(&log_syst_handle, severity, buf);
}

static void hexdump_print(struct log_msg *msg,
			  const struct log_output *log_output)
{
	char buf[CONFIG_LOG_STRDUP_MAX_STRING + 1];
	size_t length = CONFIG_LOG_STRDUP_MAX_STRING;
	u32_t severity = level_to_syst_severity(log_msg_level_get(msg));

	log_msg_hexdump_data_get(msg, buf, &length, 0);

	MIPI_SYST_WRITE(&log_syst_handle, severity, 0x1A, buf, length);
}

void log_output_msg_syst_process(const struct log_output *log_output,
				struct log_msg *msg, u32_t flag)
{
	u8_t level = (u8_t)log_msg_level_get(msg);
	bool raw_string = (level == LOG_LEVEL_INTERNAL_RAW_STRING);

	update_systh_platform_data(&log_syst_handle, log_output, flag);

	if (log_msg_is_std(msg)) {
		std_print(msg, log_output);
	} else if (raw_string) {
		raw_string_print(msg, log_output);
	} else {
		hexdump_print(msg, log_output);
	}
}

void log_output_string_syst_process(const struct log_output *log_output,
				struct log_msg_ids src_level,
				const char *fmt, va_list ap, u32_t flag)
{
	u8_t str[CONFIG_LOG_STRDUP_MAX_STRING];
	size_t length = CONFIG_LOG_STRDUP_MAX_STRING;
	u32_t severity = level_to_syst_severity((u32_t)src_level.level);

	length = vsnprintk(str, length, fmt, ap);
	str[length] = '\0';

	update_systh_platform_data(&log_syst_handle, log_output, flag);

	MIPI_SYST_PRINTF(&log_syst_handle, severity, str);
}

void log_output_hexdump_syst_process(const struct log_output *log_output,
				struct log_msg_ids src_level,
				const u8_t *data, u32_t length, u32_t flag)
{
	u32_t severity = level_to_syst_severity((u32_t)src_level.level);

	update_systh_platform_data(&log_syst_handle, log_output, flag);

	MIPI_SYST_WRITE(&log_syst_handle, severity, 0x1A, data, length);
}
