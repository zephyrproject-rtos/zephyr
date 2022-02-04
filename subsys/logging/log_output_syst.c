/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <stdio.h>
#include <ctype.h>
#include <kernel.h>
#include <mipi_syst.h>
#include <spinlock.h>
#include <toolchain.h>
#include <sys/__assert.h>
#include <linker/utils.h>
#include <logging/log.h>
#include <logging/log_ctrl.h>
#include <logging/log_output.h>

static struct mipi_syst_header log_syst_header;
static struct mipi_syst_handle log_syst_handle;

#define HEXDUMP_BYTES_IN_LINE 16

#ifdef CONFIG_LOG_STRDUP_MAX_STRING
#define STRING_BUF_MAX_LEN CONFIG_LOG_STRDUP_MAX_STRING
#else
#define STRING_BUF_MAX_LEN 128
#endif

#if defined(MIPI_SYST_PCFG_ENABLE_PLATFORM_STATE_DATA)
#if defined(CONFIG_MIPI_SYST_STP)
static mipi_syst_u16 master = 128;
static mipi_syst_u16 channel = 1;

static struct stp_writer_data writer_state;
#elif !defined(CONFIG_MIPI_SYST_RAW_DATA)
static const char pattern[] = "SYS-T RAW DATA: ";
static const char valToHex[] = "0123456789ABCDEF";
#endif

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

#if defined(CONFIG_MIPI_SYST_STP)
static void stp_write_putNibble(struct mipi_syst_handle *systh,
				struct stp_writer_data *p, mipi_syst_u8 n)
{
	p->current |= (n << 4);
	p->byteDone = !p->byteDone;

	if (p->byteDone) {
		out_func(p->current, systh->systh_platform.log_output);
		p->current = 0;
	} else {
		p->current >>= 4;
	}
}

static void stp_write_flush(struct mipi_syst_handle *systh,
			    struct stp_writer_data *p)
{
	if (!p->byteDone) {
		stp_write_putNibble(systh, p, 0);
	}
}

static void stp_write_d4(struct mipi_syst_handle *systh,
			 struct stp_writer_data *p, mipi_syst_u8 v)
{
	stp_write_putNibble(systh, p, v);
}

static void stp_write_payload8(struct mipi_syst_handle *systh,
			       struct stp_writer_data *p, mipi_syst_u8 v)
{
	stp_write_d4(systh, p, v);
	stp_write_d4(systh, p, v>>4);
}

static void stp_write_payload16(struct mipi_syst_handle *systh,
				struct stp_writer_data *p, mipi_syst_u16 v)
{
	stp_write_payload8(systh, p, (mipi_syst_u8)v);
	stp_write_payload8(systh, p, (mipi_syst_u8)(v>>8));
}

static void stp_write_payload32(struct mipi_syst_handle *systh,
				struct stp_writer_data *p, mipi_syst_u32 v)
{
	stp_write_payload16(systh, p, (mipi_syst_u16)v);
	stp_write_payload16(systh, p, (mipi_syst_u16)(v>>16));
}

static void stp_write_payload64(struct mipi_syst_handle *systh,
				struct stp_writer_data *p, mipi_syst_u64 v)
{
	stp_write_payload32(systh, p, (mipi_syst_u32)v);
	stp_write_payload32(systh, p, (mipi_syst_u32)(v>>32));
}

static mipi_syst_u64 deltaTime(struct stp_writer_data *p)
{
	mipi_syst_u64 delta;

	delta = mipi_syst_get_epoch() - p->timestamp;
	return delta * 60;
}

static void stp_write_d32mts(struct mipi_syst_handle *systh,
			     struct stp_writer_data *p, mipi_syst_u32 v)
{
	stp_write_d4(systh, p, 0xA);
	stp_write_payload32(systh, p, v);

	stp_write_d4(systh, p, 0xE);
	stp_write_payload64(systh, p, deltaTime(p));
}

static void stp_write_d64mts(struct mipi_syst_handle *systh,
			     struct stp_writer_data *p, mipi_syst_u64 v)
{
	stp_write_d4(systh, p, 0xB);
	stp_write_payload64(systh, p, v);

	stp_write_d4(systh, p, 0xE);
	stp_write_payload64(systh, p, deltaTime(p));
}

static void stp_write_d32ts(struct mipi_syst_handle *systh,
			    struct stp_writer_data *p, mipi_syst_u32 v)
{
	stp_write_d4(systh, p, 0xF);
	stp_write_d4(systh, p, 0x6);

	stp_write_payload32(systh, p, v);

	stp_write_d4(systh, p, 0xE);
	stp_write_payload64(systh, p, deltaTime(p));
}

static void stp_write_d8(struct mipi_syst_handle *systh,
			 struct stp_writer_data *p, mipi_syst_u8 v)
{
	stp_write_d4(systh, p, 0x4);
	stp_write_payload8(systh, p, v);
}

static void stp_write_d16(struct mipi_syst_handle *systh,
			  struct stp_writer_data *p, mipi_syst_u16 v)
{
	stp_write_d4(systh, p, 0x5);
	stp_write_payload16(systh, p, v);
}

static void stp_write_d32(struct mipi_syst_handle *systh,
			  struct stp_writer_data *p, mipi_syst_u32 v)
{
	stp_write_d4(systh, p, 0x6);
	stp_write_payload32(systh, p, v);
}

#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_IO)
static void stp_write_d64(struct mipi_syst_handle *systh,
			  struct stp_writer_data *p, mipi_syst_u64 v)
{
	stp_write_d4(systh, p, 0x7);
	stp_write_payload64(systh, p, v);
}
#endif

static void stp_write_flag(struct mipi_syst_handle *systh,
			   struct stp_writer_data *p)
{
	stp_write_d4(systh, p, 0xF);
	stp_write_d4(systh, p, 0xE);
}

static void stp_write_async(struct mipi_syst_handle *systh,
			    struct stp_writer_data *p)
{
	for (int i = 0; i < 21; ++i) {
		stp_write_d4(systh, p, 0xF);
	}

	stp_write_d4(systh, p, 0x0);
}

static void stp_write_version(struct mipi_syst_handle *systh,
			      struct stp_writer_data *p)
{
	stp_write_d4(systh, p, 0xF);
	stp_write_d4(systh, p, 0x0);
	stp_write_d4(systh, p, 0x0);

	stp_write_d4(systh, p, 0x3);

	p->master = 0;
	p->channel = 0;
}

static void stp_write_freq(struct mipi_syst_handle *systh,
			   struct stp_writer_data *p)
{
	stp_write_d4(systh, p, 0xF);
	stp_write_d4(systh, p, 0x0);
	stp_write_d4(systh, p, 0x8);
	stp_write_payload32(systh, p,  60 * 1000 * 1000);
}

static void stp_write_setMC(struct mipi_syst_handle *systh,
			    struct stp_writer_data *p,
			    mipi_syst_u16 master,
			    mipi_syst_u16 channel)
{
	if (!(p->recordCount++ % 20)) {
		stp_write_async(systh, p);
		stp_write_version(systh, p);
		stp_write_freq(systh, p);
	}

	if (p->master != master) {
		stp_write_d4(systh, p, 0xF);
		stp_write_d4(systh, p, 0x1);
		stp_write_payload16(systh, p, master);

		p->master = master;
		p->channel = 0;
	}

	if (p->channel != channel) {
		stp_write_d4(systh, p, 0xF);
		stp_write_d4(systh, p, 0x3);
		stp_write_payload16(systh, p, channel);

		p->channel = channel;
	}
}
#else
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
#if defined(CONFIG_MIPI_SYST_RAW_DATA)
		out_func(c, systh->systh_platform.log_output);
#else
		out_func(valToHex[c >> 0x4], systh->systh_platform.log_output);
		out_func(valToHex[c & 0xF], systh->systh_platform.log_output);
#endif
	}
}
#endif

static void write_d8(struct mipi_syst_handle *systh, uint8_t v)
{
#if defined(CONFIG_MIPI_SYST_STP)
	struct stp_writer_data *writer =
		systh->systh_header->systh_platform.stpWriter;

	stp_write_d8(systh, writer, v);
#else
	write_raw(systh, &v, sizeof(v));
#endif
}

static void write_d16(struct mipi_syst_handle *systh, uint16_t v)
{
#if defined(CONFIG_MIPI_SYST_STP)
	struct stp_writer_data *writer =
		systh->systh_header->systh_platform.stpWriter;

	stp_write_d16(systh, writer, v);
#else
	write_raw(systh, &v, sizeof(v));
#endif
}

static void write_d32(struct mipi_syst_handle *systh, uint32_t v)
{
#if defined(CONFIG_MIPI_SYST_STP)
	struct stp_writer_data *writer =
		systh->systh_header->systh_platform.stpWriter;

	stp_write_d32(systh, writer, v);
#else
	write_raw(systh, &v, sizeof(v));
#endif
}

#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_IO)
static void write_d64(struct mipi_syst_handle *systh, uint64_t v)
{
#if defined(CONFIG_MIPI_SYST_STP)
	struct stp_writer_data *writer =
		systh->systh_header->systh_platform.stpWriter;

	stp_write_d64(systh, writer, v);
#else
	write_raw(systh, &v, sizeof(v));
#endif
}
#endif

static void write_d32ts(struct mipi_syst_handle *systh, uint32_t v)
{
#if defined(CONFIG_MIPI_SYST_STP)
	struct stp_writer_data *writer =
		systh->systh_header->systh_platform.stpWriter;

	stp_write_setMC(systh, writer,
			systh->systh_platform.master,
			systh->systh_platform.channel);
	stp_write_d32ts(systh, writer, v);
#elif defined(CONFIG_MIPI_SYST_RAW_DATA)
	ARG_UNUSED(systh);

	write_raw(systh, &v, sizeof(v));
#else
	for (int i = 0; i < strlen(pattern); i++) {
		out_func(pattern[i], systh->systh_platform.log_output);
	}

	write_raw(systh, &v, sizeof(v));
#endif
}

static void write_d32mts(struct mipi_syst_handle *systh, mipi_syst_u32 v)
{
#if defined(CONFIG_MIPI_SYST_STP)
	struct stp_writer_data *writer =
		systh->systh_header->systh_platform.stpWriter;

	stp_write_setMC(systh, writer,
			systh->systh_platform.master,
			systh->systh_platform.channel);
	stp_write_d32mts(systh, writer, v);
#else
	ARG_UNUSED(systh);
	ARG_UNUSED(v);
#endif
}

static void write_d64mts(struct mipi_syst_handle *systh, mipi_syst_u64 v)
{
#if defined(CONFIG_MIPI_SYST_STP)
	struct stp_writer_data *writer =
		systh->systh_header->systh_platform.stpWriter;

	stp_write_setMC(systh, writer,
			systh->systh_platform.master,
			systh->systh_platform.channel);
	stp_write_d64mts(systh, writer, v);
#else
	ARG_UNUSED(systh);
	ARG_UNUSED(v);
#endif
}

static void write_flag(struct mipi_syst_handle *systh)
{
#if defined(CONFIG_MIPI_SYST_STP)
	struct stp_writer_data *writer =
		systh->systh_header->systh_platform.stpWriter;

	stp_write_flag(systh, writer);
	stp_write_flush(systh, writer);
#elif defined(CONFIG_MIPI_SYST_RAW_DATA)
	ARG_UNUSED(systh);
#else
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
#endif
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
#if defined(CONFIG_MIPI_SYST_STP)
	if (channel > 127) {
		++master;
		channel = 1;
	}

	systh->systh_platform.channel = channel++;
	systh->systh_platform.master  = master;
#endif

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
#if defined(CONFIG_MIPI_SYST_STP)
	writer_state.byteDone = 0;
	writer_state.current = 0;
	writer_state.master = 0;
	writer_state.channel = 0;
	writer_state.recordCount = 0;
	writer_state.timestamp = mipi_syst_get_epoch();

	systh->systh_platform.stpWriter = &writer_state;
#endif

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
	systh->systh_platform.write_d32mts = write_d32mts;
	systh->systh_platform.write_d64mts = write_d64mts;
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

static void hexdump_line_print(const uint8_t *data, uint32_t length,
			       uint32_t severity)
{
	char hexdump_buf[HEXDUMP_BYTES_IN_LINE * 4 + 4];

	hexdump_buf[sizeof(hexdump_buf) - 1] = '\0';
	char *buf = &hexdump_buf[0];

	for (int i = 0; i < HEXDUMP_BYTES_IN_LINE; i++) {
		if (i > 0 && !(i % 8)) {
			*buf = ' ';
			buf++;
		}

		if (i < length) {
			sprintf(buf, "%02x ", data[i]);
		} else {
			sprintf(buf, "   ");
		}

		buf += 3;
	}

	*buf = '|';
	buf++;

	for (int i = 0; i < HEXDUMP_BYTES_IN_LINE; i++) {
		if (i > 0 && !(i % 8)) {
			*buf = ' ';
			buf++;
		}

		if (i < length) {
			char c = (char)data[i];

			*buf = isprint((int)c) ? c : '.';
		} else {
			*buf = ' ';
		}

		buf++;
	}

	MIPI_SYST_PRINTF(&log_syst_handle, severity, "%s", hexdump_buf);
}

#ifndef CONFIG_LOG2
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
	char buf[STRING_BUF_MAX_LEN + 1];
	size_t length = STRING_BUF_MAX_LEN;
	uint32_t severity = level_to_syst_severity(log_msg_level_get(msg));

	log_msg_hexdump_data_get(msg, buf, &length, 0);

	buf[length] = '\0';

	MIPI_SYST_PRINTF(&log_syst_handle, severity, buf);
}

static void hexdump_print(struct log_msg *msg,
			  const struct log_output *log_output)
{
	uint32_t offset = 0U;
	uint8_t buf[HEXDUMP_BYTES_IN_LINE];
	size_t length;
	uint32_t severity = level_to_syst_severity(log_msg_level_get(msg));

	MIPI_SYST_PRINTF(&log_syst_handle, severity, "%s", log_msg_str_get(msg));

	do {
		length = sizeof(buf);
		log_msg_hexdump_data_get(msg, buf, &length, offset);

		if (length) {
			hexdump_line_print(buf, length, severity);
			offset += length;
		} else {
			break;
		}
	} while (true);
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
	uint32_t severity = level_to_syst_severity((uint32_t)src_level.level);

	update_systh_platform_data(&log_syst_handle, log_output, flag);

	MIPI_SYST_VPRINTF(&log_syst_handle, severity, fmt, ap);
}

void log_output_hexdump_syst_process(const struct log_output *log_output,
				     struct log_msg_ids src_level,
				     const char *metadata,
				     const uint8_t *data, uint32_t length,
				     uint32_t flag)
{
	uint32_t severity = level_to_syst_severity((uint32_t)src_level.level);

	update_systh_platform_data(&log_syst_handle, log_output, flag);

	MIPI_SYST_PRINTF(&log_syst_handle, severity, "%s", metadata);

	while (length != 0U) {
		uint32_t part_len = MIN(length, HEXDUMP_BYTES_IN_LINE);

		hexdump_line_print(data, part_len, severity);

		data += part_len;
		length -= part_len;
	}
}

#else /* !CONFIG_LOG2 */
static void hexdump2_print(const uint8_t *data, uint32_t length,
			   uint32_t severity)
{
	while (length != 0U) {
		uint32_t part_len = MIN(length, HEXDUMP_BYTES_IN_LINE);

		hexdump_line_print(data, part_len, severity);

		data += part_len;
		length -= part_len;
	}
}

#ifndef CONFIG_LOG_MIPI_SYST_USE_CATALOG
static int mipi_vprintf_formatter(cbprintf_cb out, void *ctx,
			  const char *fmt, va_list ap)
{
	struct log_msg2 *msg = ctx;
	uint32_t severity = level_to_syst_severity(log_msg2_get_level(msg));

	MIPI_SYST_VPRINTF(&log_syst_handle, severity, fmt, ap);

	return 0;
}
#else

/*
 * TODO: Big endian support.
 *
 * MIPI Sys-T catalog messages require arguments to be in little endian.
 * Currently, if the format strings are removed (which is very highly
 * probable with usage of catalog messages), there is no way to
 * distinguish arguments in va_list anymore, and thus no longer able
 * to convert endianness. So assert that we only support little endian
 * machines for now.
 */
BUILD_ASSERT(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__,
	     "Does not support big endian machines at the moment!");

/*
 * TODO: Support x86-32 and long double.
 *
 * The argument of long double on x86 32-bit is 3 bytes. However,
 * MIPI Sys-T requires 4 bytes for this. Currently, since we are
 * copying the argument list as-is, and no way to know which
 * argument is long double, a long double argument cannot be
 * expanded to 4 bytes. So error out here to prevent it being
 * used at all.
 */
#if defined(CONFIG_X86) && !defined(CONFIG_X86_64) && defined(CONFIG_CBPRINTF_PACKAGE_LONGDOUBLE)
#error "x86-32 and CONFIG_CBPRINTF_PACKAGE_LONGDOUBLE is not supported!"
#endif

/*
 * TODO: Support integer arguments under 64-bit systems.
 *
 * On 64-bit systems, an integer argument is of 8 bytes even though
 * sizeof(int) == 4. MIPI Sys-T expects an integer argument to be 4 bytes.
 * Currently, since we are copying argument list as-is, and no way to
 * know which argument is integer, a 32-bit integer cannot be
 * shrunk to 4 bytes for output. So error out here to prevent it
 * being used at all.
 */
#ifdef CONFIG_64BIT
#error "64-bit is not support at the moment!"
#endif

#ifdef CONFIG_64BIT
#define MIPI_SYST_CATMSG_ARGS_COPY MIPI_SYST_CATALOG64_ARGS_COPY
#else
#define MIPI_SYST_CATMSG_ARGS_COPY MIPI_SYST_CATALOG32_ARGS_COPY
#endif

static inline bool is_in_log_strings_section(const void *addr)
{
	extern const char __log_strings_start[];
	extern const char __log_strings_end[];

	if (((const char *)addr >= (const char *)__log_strings_start) &&
	    ((const char *)addr < (const char *)__log_strings_end)) {
		return true;
	}

	return false;
}

static struct k_spinlock payload_lock;
static uint8_t payload_buf[CONFIG_LOG_MIPI_SYST_CATALOG_ARGS_BUFFER_SIZE];
static const uint8_t * const payload_buf_end =
	&payload_buf[CONFIG_LOG_MIPI_SYST_CATALOG_ARGS_BUFFER_SIZE];

enum string_list {
	NO_STRING_LIST,
	RO_STR_IDX_LIST,
	RW_STR_IDX_LIST,
	APPENDED_STR_LIST,
};

/*
 * @brief Figure out where is the next string argument.
 *
 * @param[out] pos Offset in byte of string argument from beginning of package.
 *
 * @param[in] ros_remaining How many read-only strings left
 * @param[in] ros_str_pos Pointer to the read-only string indexes
 *
 * @param[in] rws_remaining How many read-write strings left
 * @param[in] rws_str_pos Pointer to the read-write string indexes
 *
 * @param[in] s_remaining How many appended strings left
 * @param[in] str_pos Pointer to the appended string list
 *
 * @retval NO_STRING_LIST No string picked. Usually means there is
 *                        no more strings remaining in lists.
 * @retval RO_STR_IDX_LIST Position coming from read-only string list.
 * @retval RW_STR_IDX_LIST Position coming from read-write string list.
 * @retval APPENDED_STR_LIST Position coming from appended string list.
 */
static inline
enum string_list get_next_string_arg(uint16_t *pos,
				     uint8_t ros_remaining, uint8_t *ro_str_pos,
				     uint8_t rws_remaining, uint8_t *rw_str_pos,
				     uint8_t s_remaining, uint8_t *str_pos)
{
	enum string_list which_list = NO_STRING_LIST;

	if (ros_remaining > 0) {
		*pos = ro_str_pos[0];
		which_list = RO_STR_IDX_LIST;
	}

	if (rws_remaining > 0) {
		if ((which_list == NO_STRING_LIST) || (rw_str_pos[0] < *pos)) {
			*pos = rw_str_pos[0];
			which_list = RW_STR_IDX_LIST;
		}
	}

	if (s_remaining > 0) {
		/*
		 * The first uint8_t in the appended string list for
		 * each string is its supposed position in
		 * the argument list.
		 */
		if ((which_list == NO_STRING_LIST) || (str_pos[0] < *pos)) {
			*pos = str_pos[0];
			which_list = APPENDED_STR_LIST;
		}
	}

	if (which_list != NO_STRING_LIST) {
		/*
		 * Note that the stored position value is in
		 * multiple of uint32_t. So need to convert it
		 * back to number of bytes.
		 */
		*pos *= sizeof(uint32_t);
	}

	return which_list;
}

/**
 * @brief Build the catalog message payload and send it out.
 *
 * This builds the catalog message payload to be sent through MIPI Sys-T
 * interface.
 *
 * For format strings without any string arguments, the argument list
 * is provided to the MIPI library as-is without any processing.
 * Otherwise, the strings replace the arguments in the argument list
 * by replacing the string arguments with the full strings.
 *
 * @param[in] severity Severity of log message.
 * @param[in] fmt Format string.
 * @param[in] pkg Pointer to log message package.
 * @param[in] pkg_sz Log message package size in bytes.
 * @param[in] arg Pointer to argument list inside log message package.
 * @param[in] arg_sz Argument list size in bytes inside log message package.
 * @param[in] s_nbr Number of appended strings in log message package.
 * @param[in] ros_nbr Number of read-only string indexes in log message package.
 * @param[in] rws_nbr Number of read-write string indexes in log message package.
 *
 * @retval 0 Success
 * @retval -ENOSPC Payload buffer size is too small.
 */
static int build_catalog_payload(uint32_t severity, const char *fmt,
				 uint8_t *pkg, size_t pkg_sz,
				 uint8_t *arg, size_t arg_sz,
				 uint8_t s_nbr, uint8_t ros_nbr, uint8_t rws_nbr)
{
	uint8_t *cur_str_arg_pos = NULL;
	uint8_t *payload = payload_buf;
	enum string_list which_str_list = NO_STRING_LIST;
	int ret = 0;

	/* End of argument list. */
	uint8_t * const arg_end = arg + arg_sz;

	/*
	 * Start of read-only strings indexes, which is
	 * after the argument list. Skip the first ro-string
	 * index as it points to the format string.
	 */
	uint8_t *ro_str_pos = arg_end + 1;

	/*
	 * Start of read-write strings indexes, which is
	 * after the argument list, and read-only string
	 * indexes.
	 */
	uint8_t *rw_str_pos = arg_end + ros_nbr;

	/* Start of appended strings in package */
	uint8_t *str_pos = arg_end + ros_nbr + rws_nbr;

	/* Number of strings remaining to be processed */
	uint8_t ros_remaining = ros_nbr - 1;
	uint8_t rws_remaining = rws_nbr;
	uint8_t s_remaining = s_nbr;

	k_spinlock_key_t key = k_spin_lock(&payload_lock);

	do {
		if (payload == payload_buf_end) {
			/*
			 * No space left in payload buffer but there are
			 * still arguments left. So bail out.
			 */
			ret = -ENOSPC;
			goto out;
		}

		if (cur_str_arg_pos == NULL) {
			uint16_t str_arg_pos;

			which_str_list = get_next_string_arg(&str_arg_pos,
							     ros_remaining, ro_str_pos,
							     rws_remaining, rw_str_pos,
							     s_remaining, str_pos);

			if (which_str_list != NO_STRING_LIST) {
				cur_str_arg_pos = pkg + str_arg_pos;
			} else {
				/*
				 * No more strings remaining from the lists.
				 * Set the pointer to the end of argument list,
				 * which it will never match the incrementally
				 * moving arg, and also string selection will
				 * no longer be carried out.
				 */
				cur_str_arg_pos = arg + arg_sz;
			}
		}

		if (arg == cur_str_arg_pos) {
			/*
			 * The current argument is a string pointer.
			 * So need to copy the string into the payload.
			 */

			size_t str_sz = 0;

			/* Extract the string pointer from package */
			uint8_t *s = *((uint8_t **)arg);

			/* Skip the string pointer for next argument */
			arg += sizeof(void *);

			/* Copy the string over. */
			while (s[0] != '\0') {
				payload[0] = s[0];
				payload++;
				str_sz++;
				s++;

				if (payload == payload_buf_end) {
					/*
					 * No space left in payload buffer but there are
					 * still characters to be copied. So bail out.
					 */
					ret = -ENOSPC;
					goto out;
				}
			}

			/* Need to terminate the string */
			payload[0] = '\0';
			payload++;

			if (which_str_list == RO_STR_IDX_LIST) {
				/* Move to next read-only string index */
				ro_str_pos++;
				ros_remaining--;
			} else if (which_str_list == RW_STR_IDX_LIST) {
				/* Move to next read-write string index */
				rw_str_pos++;
				rws_remaining--;
			} else if (which_str_list == APPENDED_STR_LIST) {
				/*
				 * str_pos needs to skip the string index,
				 * the string itself, and the NULL character.
				 * So next time it points to another position
				 * index.
				 */
				str_pos += str_sz + 2;
				s_remaining--;
			}

			cur_str_arg_pos = NULL;
		} else {
			/*
			 * Copy until the next string argument
			 * (or until the end of argument list).
			 */
			while (arg < cur_str_arg_pos) {
				payload[0] = arg[0];
				payload++;
				arg++;

				if (payload == payload_buf_end) {
					/*
					 * No space left in payload buffer but there are
					 * still characters to be copied. So bail out.
					 */
					ret = -ENOSPC;
					goto out;
				}
			}
		}
	} while (arg < arg_end);

	MIPI_SYST_CATMSG_ARGS_COPY(&log_syst_handle, severity,
				   (uintptr_t)fmt,
				   payload_buf,
				   (size_t)(payload - payload_buf));
out:
	k_spin_unlock(&payload_lock, key);
	return ret;
}

static int mipi_catalog_formatter(cbprintf_cb out, void *ctx,
			  const char *fmt, va_list ap)
{
	int ret = 0;
	struct log_msg2 *msg = ctx;
	uint32_t severity = level_to_syst_severity(log_msg2_get_level(msg));

	if (is_in_log_strings_section(fmt)) {
		/*
		 * Note that only format strings that are in
		 * the log_strings_section are processed as
		 * catalog messages because only these strings
		 * are in the collateral XML file.
		 */

		size_t pkg_sz, arg_sz;
		uint8_t s_nbr, ros_nbr, rws_nbr, total_str;
		uint8_t *arg;

		uint8_t *pkg = log_msg2_get_package(msg, &pkg_sz);

		/*
		 * Need to skip the package header (of pointer size),
		 * and the pointer to format string to get to
		 * the argument list.
		 */
		arg = pkg + 2 * sizeof(void *);
		arg_sz = pkg[0] * sizeof(uint32_t) - 2 * sizeof(void *);

		/* Number of appended strings already in the package. */
		s_nbr = pkg[1];

		/* Number of string indexes, for both read-only and read-write strings. */
		ros_nbr = pkg[2];
		rws_nbr = pkg[3];

		total_str = s_nbr + rws_nbr;
		if (ros_nbr > 0) {
			/*
			 * If there are read-only string indexes, the first
			 * is the format string. So we can ignore that as
			 * we are not copying it to the catalog message
			 * payload.
			 */
			total_str += ros_nbr - 1;
		}

		if (total_str == 0) {
			/*
			 * There are no string arguments so the argument list
			 * inside the package can be used as-is. The first
			 * read-only string pointer is the the format string so
			 * we can ignore that.
			 */
			MIPI_SYST_CATMSG_ARGS_COPY(&log_syst_handle, severity,
						   (uintptr_t)fmt,
						   arg,
						   arg_sz);
		} else {
			ret = build_catalog_payload(severity, fmt, pkg, pkg_sz,
						    arg, arg_sz,
						    s_nbr, ros_nbr, rws_nbr);
		}

	} else {
		MIPI_SYST_VPRINTF(&log_syst_handle, severity, fmt, ap);
	}

	return ret;
}
#endif /* !CONFIG_LOG_MIPI_SYST_USE_CATALOG */

void log_output_msg2_syst_process(const struct log_output *output,
				struct log_msg2 *msg, uint32_t flag)
{
	size_t len, hexdump_len;

	update_systh_platform_data(&log_syst_handle, output, flag);

	uint8_t *data = log_msg2_get_package(msg, &len);

	if (len) {
		(void)cbpprintf_external(NULL,
#ifdef CONFIG_LOG_MIPI_SYST_USE_CATALOG
					 mipi_catalog_formatter,
#else
					 mipi_vprintf_formatter,
#endif
					 msg, data);
	}

	data = log_msg2_get_data(msg, &hexdump_len);
	if (hexdump_len) {
		uint32_t severity = level_to_syst_severity(log_msg2_get_level(msg));

		hexdump2_print(data, hexdump_len, severity);
	}
}
#endif /* !CONFIG_LOG2 */

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
