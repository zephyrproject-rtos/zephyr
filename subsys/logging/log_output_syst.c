/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <stdio.h>
#include <ctype.h>
#include <zephyr/kernel.h>
#include <mipi_syst.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/check.h>
#include <zephyr/linker/utils.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_output.h>

static struct mipi_syst_header log_syst_header;
static struct mipi_syst_handle log_syst_handle;

#define HEXDUMP_BYTES_IN_LINE 16

#define STRING_BUF_MAX_LEN 128

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

#if defined(CONFIG_LOG_MIPI_SYST_OUTPUT_LOG_MSG_SRC_ID)
/**
 * @brief Set module ID in the origin unit of Sys-T message
 *
 * Note that this only sets the module ID if
 * CONFIG_LOG_MIPI_SYST_OUTPUT_LOG_MSG_SRC_ID is enabled.
 * Otherwise, this is a no-op as the module ID is set to
 * default at boot time, and no need to be set again.
 *
 * @param handle Pointer to mipi_syst_handle struct
 * @param module_id Module ID to be set (range 0x00 - 0x7F)
 */
static void update_handle_origin_unit(struct mipi_syst_handle *handle,
				      int16_t module_id)
{
	handle->systh_tag.et_modunit =
		_MIPI_SYST_MK_MODUNIT_ORIGIN(
			module_id,
			CONFIG_LOG_MIPI_SYST_MSG_DEFAULT_UNIT_ID
		);
}
#endif

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

static int mipi_vprintf_formatter(cbprintf_cb out, void *ctx,
			  const char *fmt, va_list ap)
{
	struct log_msg *msg = ctx;
	uint32_t severity = level_to_syst_severity(log_msg_get_level(msg));

	MIPI_SYST_VPRINTF(&log_syst_handle, severity, fmt, ap);

	return 0;
}

#ifdef CONFIG_LOG_MIPI_SYST_USE_CATALOG

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

static int mipi_catalog_formatter(cbprintf_cb out, void *ctx,
				  const char *fmt, va_list ap)
{
	struct log_msg *msg = ctx;
	uint32_t severity = level_to_syst_severity(log_msg_get_level(msg));
	k_spinlock_key_t key;

	union {
		mipi_syst_u64 v64;
		mipi_syst_u32 v32;

		unsigned int u;
		unsigned long lu;
		unsigned long long llu;

		double d;

		void *p;
	} val;

	const char *s;
	size_t arg_sz;

	uint8_t *argp = payload_buf;
	const uint8_t * const argEob = payload_buf + sizeof(payload_buf);

	size_t payload_sz;

	key = k_spin_lock(&payload_lock);

	for (int arg_tag = va_arg(ap, int);
	     arg_tag != CBPRINTF_PACKAGE_ARG_TYPE_END;
	     arg_tag = va_arg(ap, int)) {

		switch (arg_tag) {
		case CBPRINTF_PACKAGE_ARG_TYPE_CHAR:
			__fallthrough;
		case CBPRINTF_PACKAGE_ARG_TYPE_UNSIGNED_CHAR:
			__fallthrough;
		case CBPRINTF_PACKAGE_ARG_TYPE_SHORT:
			__fallthrough;
		case CBPRINTF_PACKAGE_ARG_TYPE_UNSIGNED_SHORT:
			__fallthrough;
		case CBPRINTF_PACKAGE_ARG_TYPE_INT:
			__fallthrough;
		case CBPRINTF_PACKAGE_ARG_TYPE_UNSIGNED_INT:
			val.u = (unsigned int)va_arg(ap, unsigned int);
			arg_sz = sizeof(unsigned int);
			break;

		case CBPRINTF_PACKAGE_ARG_TYPE_LONG:
			__fallthrough;
		case CBPRINTF_PACKAGE_ARG_TYPE_UNSIGNED_LONG:
			val.lu = (unsigned long)va_arg(ap, unsigned long);
			arg_sz = sizeof(unsigned long);
			break;

		case CBPRINTF_PACKAGE_ARG_TYPE_LONG_LONG:
			__fallthrough;
		case CBPRINTF_PACKAGE_ARG_TYPE_UNSIGNED_LONG_LONG:
			val.llu = (unsigned long long)va_arg(ap, unsigned long long);
			arg_sz = sizeof(unsigned long long);
			break;

		case CBPRINTF_PACKAGE_ARG_TYPE_FLOAT:
			__fallthrough;
		case CBPRINTF_PACKAGE_ARG_TYPE_DOUBLE:
			val.d = (double)va_arg(ap, double);
			arg_sz = sizeof(double);
			break;

		case CBPRINTF_PACKAGE_ARG_TYPE_LONG_DOUBLE:
			/* Handle long double as double */
			val.d = (double)va_arg(ap, long double);
			arg_sz = sizeof(double);
			break;

		case CBPRINTF_PACKAGE_ARG_TYPE_PTR_VOID:
			val.p = (void *)va_arg(ap, void *);
			arg_sz = sizeof(void *);
			break;

		case CBPRINTF_PACKAGE_ARG_TYPE_PTR_CHAR:
			s = va_arg(ap, char *);
			while (argp < argEob) {
				*argp++ = *s;
				if (*s == 0) {
					break;
				}
				s++;

				if (argp == argEob) {
					goto no_space;
				}
			}
			continue;

		default:
			k_spin_unlock(&payload_lock, key);
			return -EINVAL;
		}

		if (argp + arg_sz >= argEob) {
			goto no_space;
		}

		if (arg_sz == sizeof(mipi_syst_u64)) {
			*((mipi_syst_u64 *)argp) =
				(mipi_syst_u64)MIPI_SYST_HTOLE64(val.v64);
		} else {
			*((mipi_syst_u32 *)argp) =
				(mipi_syst_u32)MIPI_SYST_HTOLE32(val.v32);
		}
		argp += arg_sz;
	}

	/* Calculate how much buffer has been used */
	payload_sz = argp - payload_buf;

	MIPI_SYST_CATMSG_ARGS_COPY(&log_syst_handle, severity,
				   (uintptr_t)fmt,
				   payload_buf,
				   payload_sz);

	k_spin_unlock(&payload_lock, key);

	return 0;

no_space:
	k_spin_unlock(&payload_lock, key);
	return -ENOSPC;
}
#endif /* CONFIG_LOG_MIPI_SYST_USE_CATALOG */

void log_output_msg_syst_process(const struct log_output *output,
				 struct log_msg *msg, uint32_t flag)
{
	size_t len, hexdump_len;

	update_systh_platform_data(&log_syst_handle, output, flag);

#ifdef CONFIG_LOG_MIPI_SYST_OUTPUT_LOG_MSG_SRC_ID
	uint8_t level = log_msg_get_level(msg);
	bool raw_string = (level == LOG_LEVEL_INTERNAL_RAW_STRING);
	int16_t source_id = CONFIG_LOG_MIPI_SYST_MSG_DEFAULT_MODULE_ID;

	/* Set the log source ID as Sys-T message module ID */
	if (!raw_string) {
		void *source = (void *)log_msg_get_source(msg);

		if (source != NULL) {
			source_id = IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING) ?
					log_dynamic_source_id(source) :
					log_const_source_id(source);
		}
	}

	update_handle_origin_unit(&log_syst_handle, source_id);
#endif

	uint8_t *data = log_msg_get_package(msg, &len);

	if (len) {
#ifdef CONFIG_LOG_MIPI_SYST_USE_CATALOG
		struct cbprintf_package_hdr_ext *pkg_hdr = (void *)data;
		bool is_cat_msg = false, skip = false;

		if (is_in_log_strings_section(pkg_hdr->fmt)) {
			if ((pkg_hdr->hdr.desc.pkg_flags & CBPRINTF_PACKAGE_ARGS_ARE_TAGGED) ==
			    CBPRINTF_PACKAGE_ARGS_ARE_TAGGED) {
				/*
				 * Only if the package has tagged argument and
				 * the format string is in the log strings section,
				 * then we treat it as catalog message, because:
				 *
				 * 1. mipi_catalog_formatter() can only deal with
				 *    tagged arguments; and,
				 * 2. the collateral XML file only contains strings
				 *    in the log strings section.
				 */
				is_cat_msg = true;
			} else {
				/*
				 * The format string is in log strings section
				 * but the package does not have tagged argument.
				 * This cannot be processed as a catalog message,
				 * and also means we cannot print the message as
				 * it is highly likely that the log strings section
				 * has been stripped from binary and cannot be
				 * accessed.
				 */
				skip = true;
			}
		}

		if (is_cat_msg) {
			(void)cbpprintf_external(NULL,
						 mipi_catalog_formatter,
						 msg, data);
		} else if (!skip)
#endif
		{
#ifdef CONFIG_CBPRINTF_PACKAGE_HEADER_STORE_CREATION_FLAGS
			struct cbprintf_package_desc *pkg_hdr = (void *)data;

			CHECKIF((pkg_hdr->pkg_flags & CBPRINTF_PACKAGE_ARGS_ARE_TAGGED) ==
				CBPRINTF_PACKAGE_ARGS_ARE_TAGGED) {
				/*
				 * Tagged arguments are to be used with catalog messages,
				 * and should not be used for non-tagged ones.
				 */
				return;
			}
#endif


			(void)cbpprintf_external(NULL,
						 mipi_vprintf_formatter,
						 msg, data);
		}
	}

	data = log_msg_get_data(msg, &hexdump_len);
	if (hexdump_len) {
		uint32_t severity = level_to_syst_severity(log_msg_get_level(msg));

		hexdump2_print(data, hexdump_len, severity);
	}
}

static int syst_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	MIPI_SYST_INIT_STATE(&log_syst_header,
			     mipi_syst_platform_init, (void *)0);

	MIPI_SYST_INIT_HANDLE_STATE(&log_syst_header,
				    &log_syst_handle, NULL);

	log_syst_handle.systh_tag.et_guid = 0;

#ifndef CONFIG_LOG_MIPI_SYST_OUTPUT_LOG_MSG_SRC_ID
	/* Set the default here once as it won't be modified anymore. */
	log_syst_handle.systh_tag.et_modunit =
		_MIPI_SYST_MK_MODUNIT_ORIGIN(
			CONFIG_LOG_MIPI_SYST_MSG_DEFAULT_MODULE_ID,
			CONFIG_LOG_MIPI_SYST_MSG_DEFAULT_UNIT_ID
		);
#endif

	return 0;
}

SYS_INIT(syst_init, POST_KERNEL, 0);
