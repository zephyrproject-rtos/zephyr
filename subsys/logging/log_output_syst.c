/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <kernel.h>
#include <mipi_syst.h>
#include <logging/log.h>
#include <logging/log_ctrl.h>
#include <logging/log_output.h>

static struct mipi_syst_header log_syst_header;
static struct mipi_syst_handle log_syst_handle;

#if defined(MIPI_SYST_PCFG_ENABLE_PLATFORM_STATE_DATA)
static const char pattern[] = "SYS-T RAW DATA: ";
static const char valToHex[] = "0123456789ABCDEF";

static int out_func(int c, void *ctx)
{
	const struct log_output *out_ctx = (const struct log_output *)ctx;

	out_ctx->buf[out_ctx->control_block->offset] = (uint8_t)c;
	out_ctx->control_block->offset++;

	__ASSERT_NO_MSG(out_ctx->control_block->offset <= out_ctx->size);

	if (out_ctx->control_block->offset == out_ctx->size) {
		log_output_flush(out_ctx);
	}

	return 0;
}

static void write_raw(struct mipi_syst_handle *systh, const void *p, int n)
{
	int i;
	uint8_t c;

#if defined(MIPI_SYST_BIG_ENDIAN)
	for (i = n - 1; i >= 0; --i) {
#else
	for (i = 0; i < n; ++i) {
#endif
		c = ((const uint8_t *)p)[i];
		out_func(valToHex[c >> 0x4], systh->systh_platform.log_output);
		out_func(valToHex[c & 0xF], systh->systh_platform.log_output);
	}
}

static void write_d8(struct mipi_syst_handle *systh, uint8_t v)
{
	write_raw(systh, &v, sizeof(v));
}

static void write_d16(struct mipi_syst_handle *systh, uint16_t v)
{
	write_raw(systh, &v, sizeof(v));
}

static void write_d32(struct mipi_syst_handle *systh, uint32_t v)
{
	write_raw(systh, &v, sizeof(v));
}

#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_IO)
static void write_d64(struct mipi_syst_handle *systh, uint64_t v)
{
	write_raw(systh, &v, sizeof(v));
}
#endif

static void write_d32ts(struct mipi_syst_handle *systh, uint32_t v)
{
	for (int i = 0; i < strlen(pattern); i++) {
		out_func(pattern[i], systh->systh_platform.log_output);
	}

	write_raw(systh, &v, sizeof(v));
}

static void write_flag(struct mipi_syst_handle *systh)
{
	uint32_t flag = systh->systh_platform.flag;

	if ((flag & LOG_OUTPUT_FLAG_CRLF_NONE) != 0U) {
		return;
	}

	if ((flag & LOG_OUTPUT_FLAG_CRLF_LFONLY) != 0U) {
		out_func('\n', systh->systh_platform.log_output);
	} else {
		out_func('\r', systh->systh_platform.log_output);
		out_func('\n', systh->systh_platform.log_output);
	}
}
#endif

#if defined(MIPI_SYST_PCFG_ENABLE_TIMESTAMP)
mipi_syst_u64 mipi_syst_get_epoch(void)
{
	return k_uptime_ticks();
}
#endif

static void update_systh_platform_data(struct mipi_syst_handle *handle,
				       const struct log_output *log_output,
				       uint32_t flag)
{
#if defined(MIPI_SYST_PCFG_ENABLE_PLATFORM_STATE_DATA)
	handle->systh_platform.flag = (mipi_syst_u32)flag;
	handle->systh_platform.log_output = (struct log_output *)log_output;
#endif
}

#if defined(MIPI_SYST_PCFG_ENABLE_PLATFORM_HANDLE_DATA)
/*
 * Platform specific SyS-T handle initialization hook function
 *
 * @param systh pointer to the SyS-T handle structure
 */
static void platform_handle_init(struct mipi_syst_handle *systh)
{
#if defined(MIPI_SYST_PCFG_LENGTH_FIELD)
	MIPI_SYST_ENABLE_HANDLE_LENGTH(systh, 1);
#endif

#if defined(MIPI_SYST_PCFG_ENABLE_TIMESTAMP)
	MIPI_SYST_ENABLE_HANDLE_TIMESTAMP(systh, 1);
#endif
}

static void platform_handle_release(struct mipi_syst_handle *systh)
{
	ARG_UNUSED(systh);
}
#endif

/*
 * Platform specific global state initialization hook function
 *
 * @param systh pointer to the new SyS-T handle structure
 * @param platform_data user defined data for the init function.
 *
 */
static void mipi_syst_platform_init(struct mipi_syst_header *systh,
				    const void *platform_data)
{
#if defined(MIPI_SYST_PCFG_ENABLE_PLATFORM_HANDLE_DATA)
	systh->systh_inith = platform_handle_init;
	systh->systh_releaseh = platform_handle_release;
#endif
#if defined(MIPI_SYST_PCFG_ENABLE_PLATFORM_STATE_DATA)
	systh->systh_platform.write_d8 = write_d8;
	systh->systh_platform.write_d16 = write_d16;
	systh->systh_platform.write_d32 = write_d32;
#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_IO)
	systh->systh_platform.write_d64 = write_d64;
#endif
	systh->systh_platform.write_d32ts = write_d32ts;
	systh->systh_platform.write_flag = write_flag;
#endif
}

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
static uint32_t level_to_syst_severity(uint32_t level)
{
	uint32_t ret;

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
	uint32_t nargs = log_msg_nargs_get(msg);
	uint32_t *args = alloca(sizeof(uint32_t)*nargs);
	uint32_t severity = level_to_syst_severity(log_msg_level_get(msg));

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
	uint32_t severity = level_to_syst_severity(log_msg_level_get(msg));

	log_msg_hexdump_data_get(msg, buf, &length, 0);

	buf[length] = '\0';

	MIPI_SYST_PRINTF(&log_syst_handle, severity, buf);
}

static void hexdump_print(struct log_msg *msg,
			  const struct log_output *log_output)
{
	char buf[CONFIG_LOG_STRDUP_MAX_STRING + 1];
	size_t length = CONFIG_LOG_STRDUP_MAX_STRING;
	uint32_t severity = level_to_syst_severity(log_msg_level_get(msg));

	log_msg_hexdump_data_get(msg, buf, &length, 0);

	MIPI_SYST_WRITE(&log_syst_handle, severity, 0x1A, buf, length);
}

void log_output_msg_syst_process(const struct log_output *log_output,
				struct log_msg *msg, uint32_t flag)
{
	uint8_t level = (uint8_t)log_msg_level_get(msg);
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
				const char *fmt, va_list ap, uint32_t flag)
{
	uint8_t str[CONFIG_LOG_STRDUP_MAX_STRING];
	size_t length = CONFIG_LOG_STRDUP_MAX_STRING;
	uint32_t severity = level_to_syst_severity((uint32_t)src_level.level);

	length = vsnprintk(str, length, fmt, ap);
	str[length] = '\0';

	update_systh_platform_data(&log_syst_handle, log_output, flag);

	MIPI_SYST_PRINTF(&log_syst_handle, severity, str);
}

void log_output_hexdump_syst_process(const struct log_output *log_output,
				struct log_msg_ids src_level,
				const uint8_t *data, uint32_t length, uint32_t flag)
{
	uint32_t severity = level_to_syst_severity((uint32_t)src_level.level);

	update_systh_platform_data(&log_syst_handle, log_output, flag);

	MIPI_SYST_WRITE(&log_syst_handle, severity, 0x1A, data, length);
}

static int syst_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	MIPI_SYST_INIT_STATE(&log_syst_header,
			     mipi_syst_platform_init, (void *)0);

	MIPI_SYST_INIT_HANDLE_STATE(&log_syst_header,
				    &log_syst_handle, NULL);

	return 0;
}

SYS_INIT(syst_init, POST_KERNEL, 0);
