/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <zephyr/logging/log_output.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_output_custom.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/cbprintf.h>
#include <ctype.h>
#include <time.h>
#include <stdio.h>
#include <stdbool.h>

#define LOG_COLOR_CODE_DEFAULT "\x1B[0m"
#define LOG_COLOR_CODE_RED     "\x1B[1;31m"
#define LOG_COLOR_CODE_GREEN   "\x1B[1;32m"
#define LOG_COLOR_CODE_YELLOW  "\x1B[1;33m"
#define LOG_COLOR_CODE_BLUE    "\x1B[1;34m"

#define HEXDUMP_BYTES_IN_LINE 16

#define  DROPPED_COLOR_PREFIX \
	COND_CODE_1(CONFIG_LOG_BACKEND_SHOW_COLOR, (LOG_COLOR_CODE_RED), ())

#define DROPPED_COLOR_POSTFIX \
	COND_CODE_1(CONFIG_LOG_BACKEND_SHOW_COLOR, (LOG_COLOR_CODE_DEFAULT), ())

static const char *const severity[] = {
	NULL,
	"err",
	"wrn",
	"inf",
	"dbg"
};

static const char *const colors[] = {
	NULL,
	IS_ENABLED(CONFIG_LOG_BACKEND_SHOW_COLOR) ? LOG_COLOR_CODE_RED : NULL,    /* err */
	IS_ENABLED(CONFIG_LOG_BACKEND_SHOW_COLOR) ? LOG_COLOR_CODE_YELLOW : NULL, /* warn */
	IS_ENABLED(CONFIG_LOG_INFO_COLOR_GREEN) ? LOG_COLOR_CODE_GREEN : NULL,   /* info */
	IS_ENABLED(CONFIG_LOG_DBG_COLOR_BLUE) ? LOG_COLOR_CODE_BLUE : NULL,   /* dbg */
};

static uint32_t freq;
static log_timestamp_t timestamp_div;

#define SECONDS_IN_DAY			86400U

static uint32_t days_in_month[12] = {31, 28, 31, 30, 31, 30, 31,
									31, 30, 31, 30, 31};

struct YMD_date {
	uint32_t year;
	uint32_t month;
	uint32_t day;
};

/* The RFC 5424 allows very flexible mapping and suggest the value 0 being the
 * highest severity and 7 to be the lowest (debugging level) severity.
 *
 *    0   Emergency      System is unusable
 *    1   Alert          Action must be taken immediately
 *    2   Critical       Critical conditions
 *    3   Error          Error conditions
 *    4   Warning        Warning conditions
 *    5   Notice         Normal but significant condition
 *    6   Informational  Informational messages
 *    7   Debug          Debug-level messages
 */
static int level_to_rfc5424_severity(uint32_t level)
{
	uint8_t ret;

	switch (level) {
	case LOG_LEVEL_NONE:
		ret = 7U;
		break;
	case LOG_LEVEL_ERR:
		ret =  3U;
		break;
	case LOG_LEVEL_WRN:
		ret =  4U;
		break;
	case LOG_LEVEL_INF:
		ret =  6U;
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

static int out_func(int c, void *ctx)
{
	const struct log_output *out_ctx = (const struct log_output *)ctx;
	int idx;

	if (IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE)) {
		/* Backend must be thread safe in synchronous operation. */
		/* Need that step for big endian */
		char x = (char)c;

		out_ctx->func((uint8_t *)&x, 1, out_ctx->control_block->ctx);
		return 0;
	}

	if (out_ctx->control_block->offset == out_ctx->size) {
		log_output_flush(out_ctx);
	}

	idx = atomic_inc(&out_ctx->control_block->offset);
	out_ctx->buf[idx] = (uint8_t)c;

	__ASSERT_NO_MSG(out_ctx->control_block->offset <= out_ctx->size);

	return 0;
}

static int cr_out_func(int c, void *ctx)
{
	if (c == '\n') {
		out_func((int)'\r', ctx);
	}
	out_func(c, ctx);

	return 0;
}

static int print_formatted(const struct log_output *output,
			   const char *fmt, ...)
{
	va_list args;
	int length = 0;

	va_start(args, fmt);
	length = cbvprintf(out_func, (void *)output, fmt, args);
	va_end(args);

	return length;
}

static inline bool is_leap_year(uint32_t year)
{
	return (((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0));
}

static void __attribute__((unused)) get_YMD_from_seconds(uint64_t seconds,
			struct YMD_date *output_date)
{
	uint64_t tmp;
	int i;

	output_date->year = 1970;
	output_date->month = 1;
	output_date->day = 1;

	/* compute the proper year */
	while (1) {
		tmp = (is_leap_year(output_date->year)) ?
					366*SECONDS_IN_DAY : 365*SECONDS_IN_DAY;
		if (tmp > seconds) {
			break;
		}
		seconds -= tmp;
		output_date->year++;
	}
	/* compute the proper month */
	for (i = 0; i < ARRAY_SIZE(days_in_month); i++) {
		tmp = ((i == 1) && is_leap_year(output_date->year)) ?
					((uint64_t)days_in_month[i] + 1) * SECONDS_IN_DAY :
					(uint64_t)days_in_month[i] * SECONDS_IN_DAY;
		if (tmp > seconds) {
			output_date->month += i;
			break;
		}
		seconds -= tmp;
	}

	output_date->day += seconds / SECONDS_IN_DAY;
}

static int timestamp_print(const struct log_output *output,
			   uint32_t flags, log_timestamp_t timestamp)
{
	int length;
	bool format =
		(flags & LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP) |
		(flags & LOG_OUTPUT_FLAG_FORMAT_SYSLOG) |
		IS_ENABLED(CONFIG_LOG_OUTPUT_FORMAT_LINUX_TIMESTAMP) |
		IS_ENABLED(CONFIG_LOG_OUTPUT_FORMAT_CUSTOM_TIMESTAMP);


	if (!format) {
#ifndef CONFIG_LOG_TIMESTAMP_64BIT
		length = print_formatted(output, "[%08lu] ", timestamp);
#else
		length = print_formatted(output, "[%016llu] ", timestamp);
#endif
	} else if (freq != 0U) {
#ifndef CONFIG_LOG_TIMESTAMP_64BIT
		uint32_t total_seconds;
#else
		uint64_t total_seconds;
#endif
		uint32_t remainder;
		uint32_t seconds;
		uint32_t hours;
		uint32_t mins;
		uint32_t ms;
		uint32_t us;

		timestamp /= timestamp_div;
		total_seconds = timestamp / freq;
		seconds = total_seconds;
		hours = seconds / 3600U;
		seconds -= hours * 3600U;
		mins = seconds / 60U;
		seconds -= mins * 60U;

		remainder = timestamp % freq;
		ms = (remainder * 1000U) / freq;
		us = (1000 * (remainder * 1000U - (ms * freq))) / freq;

		if (IS_ENABLED(CONFIG_LOG_BACKEND_NET) && flags & LOG_OUTPUT_FLAG_FORMAT_SYSLOG) {
#if defined(CONFIG_REQUIRES_FULL_LIBC)
			char time_str[sizeof("1970-01-01T00:00:00")];
			struct tm tm_timestamp = {0};
			time_t time_seconds = total_seconds;

			gmtime_r(&time_seconds, &tm_timestamp);

			strftime(time_str, sizeof(time_str), "%FT%T", &tm_timestamp);

			length = print_formatted(output, "%s.%06uZ ",
						 time_str, ms * 1000U + us);
#else
			struct YMD_date date;

			get_YMD_from_seconds(total_seconds, &date);
			hours = hours % 24;
			length = print_formatted(output,
					"%04u-%02u-%02uT%02u:%02u:%02u.%06uZ ",
					date.year, date.month, date.day,
					hours, mins, seconds, ms * 1000U + us);
#endif
		} else if (IS_ENABLED(CONFIG_LOG_OUTPUT_FORMAT_CUSTOM_TIMESTAMP)) {
			length = log_custom_timestamp_print(output, timestamp, print_formatted);
		} else {
			if (IS_ENABLED(CONFIG_LOG_OUTPUT_FORMAT_LINUX_TIMESTAMP)) {
				length = print_formatted(output,
#if defined(CONFIG_LOG_TIMESTAMP_64BIT)
							"[%5llu.%06d] ",
#else
							"[%5lu.%06d] ",
#endif
							total_seconds, ms * 1000U + us);
			} else if (IS_ENABLED(CONFIG_LOG_OUTPUT_FORMAT_DATE_TIMESTAMP)) {
#if defined(CONFIG_REQUIRES_FULL_LIBC)
				char time_str[sizeof("1970-01-01 00:00:00")];
				struct tm tm_timestamp = {0};
				time_t time_seconds = total_seconds;

				gmtime_r(&time_seconds, &tm_timestamp);

				strftime(time_str, sizeof(time_str), "%F %T", &tm_timestamp);

				length = print_formatted(output, "[%s.%03u,%03u] ", time_str, ms,
							 us);
#else
				struct YMD_date date;

				get_YMD_from_seconds(total_seconds, &date);
				hours = hours % 24;
				length = print_formatted(
					output, "[%04u-%02u-%02u %02u:%02u:%02u.%03u,%03u] ",
					date.year, date.month, date.day, hours, mins, seconds, ms,
					us);
#endif
			} else if (IS_ENABLED(CONFIG_LOG_OUTPUT_FORMAT_ISO8601_TIMESTAMP)) {
#if defined(CONFIG_REQUIRES_FULL_LIBC)
				char time_str[sizeof("1970-01-01T00:00:00")];
				struct tm tm_timestamp = {0};
				time_t time_seconds = total_seconds;

				gmtime_r(&time_seconds, &tm_timestamp);

				strftime(time_str, sizeof(time_str), "%FT%T", &tm_timestamp);

				length = print_formatted(output, "[%s,%06uZ] ", time_str,
							 ms * 1000U + us);
#else
				struct YMD_date date;

				get_YMD_from_seconds(total_seconds, &date);
				hours = hours % 24;
				length = print_formatted(output,
							 "[%04u-%02u-%02uT%02u:%02u:%02u,%06uZ] ",
							 date.year, date.month, date.day, hours,
							 mins, seconds, ms * 1000U + us);
#endif
			} else {
				length = print_formatted(output,
							"[%02u:%02u:%02u.%03u,%03u] ",
							hours, mins, seconds, ms, us);
			}
		}
	} else {
		length = 0;
	}

	return length;
}

static void color_print(const struct log_output *output,
			bool color, bool start, uint32_t level)
{
	if (color) {
		const char *log_color = start && (colors[level] != NULL) ?
				colors[level] : LOG_COLOR_CODE_DEFAULT;
		print_formatted(output, "%s", log_color);
	}
}

static void color_prefix(const struct log_output *output,
			 bool color, uint32_t level)
{
	color_print(output, color, true, level);
}

static void color_postfix(const struct log_output *output,
			  bool color, uint32_t level)
{
	color_print(output, color, false, level);
}


static int ids_print(const struct log_output *output,
		     bool level_on,
		     bool func_on,
		     bool thread_on,
		     const char *domain,
		     const char *source,
		     k_tid_t tid,
		     uint32_t level)
{
	int total = 0;

	if (level_on) {
		total += print_formatted(output, "<%s> ", severity[level]);
	}

	if (IS_ENABLED(CONFIG_LOG_THREAD_ID_PREFIX) && thread_on) {
		if (IS_ENABLED(CONFIG_THREAD_NAME)) {
			total += print_formatted(output, "[%s] ",
				tid == NULL ? "irq" : k_thread_name_get(tid));
		} else {
			total += print_formatted(output, "[%p] ", tid);
		}
	}

	if (domain) {
		total += print_formatted(output, "%s/", domain);
	}

	if (source) {
		total += print_formatted(output,
				(func_on &&
				((1 << level) & LOG_FUNCTION_PREFIX_MASK)) ?
				"%s." : "%s: ",
				source);
	}

	return total;
}

static void newline_print(const struct log_output *ctx, uint32_t flags)
{
	if (IS_ENABLED(CONFIG_LOG_BACKEND_NET) &&
	    flags & LOG_OUTPUT_FLAG_FORMAT_SYSLOG) {
		return;
	}

	if ((flags & LOG_OUTPUT_FLAG_CRLF_NONE) != 0U) {
		return;
	}

	if ((flags & LOG_OUTPUT_FLAG_CRLF_LFONLY) != 0U) {
		print_formatted(ctx, "\n");
	} else {
		print_formatted(ctx, "\r\n");
	}
}

static void hexdump_line_print(const struct log_output *output,
			       const uint8_t *data, uint32_t length,
			       int prefix_offset, uint32_t flags)
{
	newline_print(output, flags);

	for (int i = 0; i < prefix_offset; i++) {
		print_formatted(output, " ");
	}

	for (int i = 0; i < HEXDUMP_BYTES_IN_LINE; i++) {
		if (i > 0 && !(i % 8)) {
			print_formatted(output, " ");
		}

		if (i < length) {
			print_formatted(output, "%02x ", data[i]);
		} else {
			print_formatted(output, "   ");
		}
	}

	print_formatted(output, "|");

	for (int i = 0; i < HEXDUMP_BYTES_IN_LINE; i++) {
		if (i > 0 && !(i % 8)) {
			print_formatted(output, " ");
		}

		if (i < length) {
			unsigned char c = (unsigned char)data[i];

			print_formatted(output, "%c",
			      isprint((int)c) != 0 ? c : '.');
		} else {
			print_formatted(output, " ");
		}
	}
}

static void log_msg_hexdump(const struct log_output *output,
			    uint8_t *data, uint32_t len,
			    int prefix_offset, uint32_t flags)
{
	size_t length;

	do {
		length = MIN(len, HEXDUMP_BYTES_IN_LINE);

		hexdump_line_print(output, data, length,
				   prefix_offset, flags);
		data += length;
		len -= length;
	} while (len);
}

#if defined(CONFIG_LOG_BACKEND_NET_RFC5424_SDATA_SEQID)
static int32_t get_sequence_id(void)
{
	static int32_t id;

	if (++id < 0) {
		id = 1;
	}

	return id;
}
#endif

#if defined(CONFIG_LOG_BACKEND_NET_RFC5424_SDATA_TZKNOWN)
static bool is_tzknown(void)
{
	/* TODO: use proper implementation */
	return false;
}
#endif

#if defined(CONFIG_LOG_BACKEND_NET_RFC5424_SDATA_ISSYNCED)
static bool is_synced(void)
{
	/* TODO: use proper implementation */
	return IS_ENABLED(CONFIG_SNTP);
}
#endif

static int syslog_print(const struct log_output *output,
			bool level_on,
			bool func_on,
			bool *thread_on,
			const char *domain,
			const char *source,
			k_tid_t tid,
			uint32_t level,
			uint32_t length)
{
	uint32_t len = length;

	/* The syslog output format is:
	 * HOSTNAME SP APP-NAME SP PROCID SP MSGID SP STRUCTURED-DATA
	 */

	/* First HOSTNAME */
	len += print_formatted(output, "%s ",
			       output->control_block->hostname ?
			       output->control_block->hostname :
			       "zephyr");

	/* Then APP-NAME. We use the thread name here. It should not
	 * contain any space characters.
	 */
	if (*thread_on) {
		if (IS_ENABLED(CONFIG_THREAD_NAME)) {
			if (strstr(k_thread_name_get(tid), " ") != NULL) {
				goto do_not_print_name;
			}

			len += print_formatted(output, "%s ",
					       tid == NULL ?
					       "irq" :
					       k_thread_name_get(tid));
		} else {
do_not_print_name:
			len += print_formatted(output, "%p ", tid);
		}

		/* Do not print thread id in the message as it was already
		 * printed above.
		 */
		*thread_on = false;
	} else {
		/* No APP-NAME */
		len += print_formatted(output, "- ");
	}

	if (!IS_ENABLED(CONFIG_LOG_BACKEND_NET_RFC5424_STRUCTURED_DATA)) {
		/* No PROCID, MSGID or STRUCTURED-DATA */
		len += print_formatted(output, "- - - ");

		return len;
	}


#if defined(CONFIG_LOG_BACKEND_NET_RFC5424_SDATA_SOFTWARE) || \
	defined(CONFIG_LOG_BACKEND_NET_RFC5424_SDATA_SOFTWARE_VERSION)
#define STRUCTURED_DATA_ORIGIN_START      "[origin"
#define STRUCTURED_DATA_ORIGIN_SW         " software=\"%s\""
#define STRUCTURED_DATA_ORIGIN_SW_VERSION " swVersion=\"%u\""
#define STRUCTURED_DATA_ORIGIN_END        "]"
#define STRUCTURED_DATA_ORIGIN						      \
	    STRUCTURED_DATA_ORIGIN_START				      \
	    COND_CODE_1(CONFIG_LOG_BACKEND_NET_RFC5424_SDATA_SOFTWARE,	      \
			(STRUCTURED_DATA_ORIGIN_SW), ("%s"))		      \
	    COND_CODE_1(CONFIG_LOG_BACKEND_NET_RFC5424_SDATA_SOFTWARE_VERSION,\
			(STRUCTURED_DATA_ORIGIN_SW_VERSION), ("%s"))	      \
	    STRUCTURED_DATA_ORIGIN_END
#else
#define STRUCTURED_DATA_ORIGIN "%s%s"
#endif

#if defined(CONFIG_LOG_BACKEND_NET_RFC5424_SDATA_SEQID) || \
	defined(CONFIG_LOG_BACKEND_NET_RFC5424_SDATA_UPTIME)
#define STRUCTURED_DATA_META_START  "[meta"
#define STRUCTURED_DATA_META_SEQID  " sequenceId=\"%d\""
#define STRUCTURED_DATA_META_UPTIME " sysUpTime=\"%d\""
#define STRUCTURED_DATA_META_END    "]"
#define STRUCTURED_DATA_META						\
	    STRUCTURED_DATA_META_START					\
	    COND_CODE_1(CONFIG_LOG_BACKEND_NET_RFC5424_SDATA_SEQID,	\
			(STRUCTURED_DATA_META_SEQID), ("%s"))		\
	    COND_CODE_1(CONFIG_LOG_BACKEND_NET_RFC5424_SDATA_UPTIME,	\
			(STRUCTURED_DATA_META_UPTIME), ("%s"))		\
	    STRUCTURED_DATA_META_END
#else
#define STRUCTURED_DATA_META "%s%s"
#endif

#if defined(CONFIG_LOG_BACKEND_NET_RFC5424_SDATA_TZKNOWN) || \
	defined(CONFIG_LOG_BACKEND_NET_RFC5424_SDATA_ISSYNCED)
#define STRUCTURED_DATA_TIMEQUALITY_START    "[timeQuality"
#define STRUCTURED_DATA_TIMEQUALITY_TZKNOWN  " tzKnown=\"%d\""
#define STRUCTURED_DATA_TIMEQUALITY_ISSYNCED " isSynced=\"%d\""
#define STRUCTURED_DATA_TIMEQUALITY_END      "]"
#define STRUCTURED_DATA_TIMEQUALITY					\
	    STRUCTURED_DATA_TIMEQUALITY_START				\
	    COND_CODE_1(CONFIG_LOG_BACKEND_NET_RFC5424_SDATA_TZKNOWN,	\
			(STRUCTURED_DATA_TIMEQUALITY_TZKNOWN), ("%s"))	\
	    COND_CODE_1(CONFIG_LOG_BACKEND_NET_RFC5424_SDATA_ISSYNCED,	\
			(STRUCTURED_DATA_TIMEQUALITY_ISSYNCED), ("%s"))	\
	    STRUCTURED_DATA_TIMEQUALITY_END
#else
#define STRUCTURED_DATA_TIMEQUALITY "%s%s"
#endif

	/* No PROCID or MSGID, but there is structured data.
	 */
	len += print_formatted(output,
		"- - "
		STRUCTURED_DATA_META
		STRUCTURED_DATA_ORIGIN
		STRUCTURED_DATA_TIMEQUALITY,
		COND_CODE_1(CONFIG_LOG_BACKEND_NET_RFC5424_SDATA_SEQID,
			    (get_sequence_id()), ("")),
		COND_CODE_1(CONFIG_LOG_BACKEND_NET_RFC5424_SDATA_UPTIME,
			    /* in hundredths of a sec */
			    (k_uptime_get_32() / 10), ("")),
		COND_CODE_1(CONFIG_LOG_BACKEND_NET_RFC5424_SDATA_SOFTWARE,
			    (CONFIG_LOG_BACKEND_NET_RFC5424_SDATA_SOFTWARE_VALUE),
			    ("")),
		COND_CODE_1(CONFIG_LOG_BACKEND_NET_RFC5424_SDATA_SOFTWARE_VERSION,
			    (sys_kernel_version_get()), ("")),
		COND_CODE_1(CONFIG_LOG_BACKEND_NET_RFC5424_SDATA_TZKNOWN,
			    (is_tzknown()), ("")),
		COND_CODE_1(CONFIG_LOG_BACKEND_NET_RFC5424_SDATA_ISSYNCED,
			    (is_synced()), (""))
		);

	return len;
}

static uint32_t prefix_print(const struct log_output *output,
			     uint32_t flags,
			     bool func_on,
			     log_timestamp_t timestamp,
			     const char *domain,
			     const char *source,
			     k_tid_t tid,
			     uint8_t level)
{
	__ASSERT_NO_MSG(level <= LOG_LEVEL_DBG);
	uint32_t length = 0U;

	bool stamp = flags & LOG_OUTPUT_FLAG_TIMESTAMP;
	bool colors_on = flags & LOG_OUTPUT_FLAG_COLORS;
	bool level_on = flags & LOG_OUTPUT_FLAG_LEVEL;
	bool thread_on = IS_ENABLED(CONFIG_LOG_THREAD_ID_PREFIX) &&
			 (flags & LOG_OUTPUT_FLAG_THREAD);
	bool source_off = flags & LOG_OUTPUT_FLAG_SKIP_SOURCE;
	const char *tag = IS_ENABLED(CONFIG_LOG) ? z_log_get_tag() : NULL;

	if (IS_ENABLED(CONFIG_LOG_BACKEND_NET) &&
	    flags & LOG_OUTPUT_FLAG_FORMAT_SYSLOG) {
		/* TODO: As there is no way to figure out the
		 * facility at this point, use a pre-defined value.
		 * Change this to use the real facility of the
		 * logging call when that info is available.
		 */
		static const int facility = 16; /* local0 */

		length += print_formatted(
			output,
			 /* <PRI>VERSION */
			"<%d>1 ",
			facility * 8 +
			level_to_rfc5424_severity(level));
	}

	if (tag) {
		length += print_formatted(output, "%s ", tag);
	}

	if (stamp) {
		length += timestamp_print(output, flags, timestamp);
	}

	if (IS_ENABLED(CONFIG_LOG_BACKEND_NET) &&
	    flags & LOG_OUTPUT_FLAG_FORMAT_SYSLOG) {
		length += syslog_print(output, level_on, func_on, &thread_on, domain,
				       source_off ? NULL : source, tid, level, length);
	} else {
		color_prefix(output, colors_on, level);
	}

	length += ids_print(output, level_on, func_on, thread_on, domain,
			    source_off ? NULL : source, tid, level);

	return length;
}

static void postfix_print(const struct log_output *output,
			  uint32_t flags, uint8_t level)
{
	color_postfix(output, (flags & LOG_OUTPUT_FLAG_COLORS),
			      level);
	newline_print(output, flags);
}

void log_output_process(const struct log_output *output,
			log_timestamp_t timestamp,
			const char *domain,
			const char *source,
			k_tid_t tid,
			uint8_t level,
			const uint8_t *package,
			const uint8_t *data,
			size_t data_len,
			uint32_t flags)
{
	bool raw_string = (level == LOG_LEVEL_INTERNAL_RAW_STRING);
	uint32_t prefix_offset;
	cbprintf_cb cb;

	if (!raw_string) {
		prefix_offset = prefix_print(output, flags, 0, timestamp,
					     domain, source, tid, level);
		cb = out_func;
	} else {
		prefix_offset = 0;
		/* source set to 1 indicates raw string and contrary to printk
		 * case it should not append anything to the output (printk is
		 * appending <CR> to the new line character).
		 */
		cb = ((uintptr_t)source == 1) ? out_func : cr_out_func;
	}

	if (package) {
		int err = cbpprintf(cb, (void *)output, (void *)package);

		(void)err;
		__ASSERT_NO_MSG(err >= 0);
	}

	if (data_len) {
		log_msg_hexdump(output, (uint8_t *)data, data_len, prefix_offset, flags);
	}

	if (!raw_string) {
		postfix_print(output, flags, level);
	}

	log_output_flush(output);
}

void log_output_msg_process(const struct log_output *output,
			    struct log_msg *msg, uint32_t flags)
{
	log_timestamp_t timestamp = log_msg_get_timestamp(msg);
	uint8_t level = log_msg_get_level(msg);
	uint8_t domain_id = log_msg_get_domain(msg);
	int16_t source_id = log_msg_get_source_id(msg);

	const char *sname = source_id >= 0 ? log_source_name_get(domain_id, source_id) : NULL;
	size_t plen, dlen;
	uint8_t *package = log_msg_get_package(msg, &plen);
	uint8_t *data = log_msg_get_data(msg, &dlen);

	log_output_process(output, timestamp, NULL, sname, (k_tid_t)log_msg_get_tid(msg), level,
			   plen > 0 ? package : NULL, data, dlen, flags);
}

void log_output_dropped_process(const struct log_output *output, uint32_t cnt)
{
	char buf[5];
	int len;
	static const char prefix[] = DROPPED_COLOR_PREFIX "--- ";
	static const char postfix[] =
			" messages dropped ---\r\n" DROPPED_COLOR_POSTFIX;
	log_output_func_t outf = output->func;

	cnt = MIN(cnt, 9999);
	len = snprintk(buf, sizeof(buf), "%d", cnt);

	log_output_write(outf, (uint8_t *)prefix, sizeof(prefix) - 1, output->control_block->ctx);
	log_output_write(outf, buf, len, output->control_block->ctx);
	log_output_write(outf, (uint8_t *)postfix, sizeof(postfix) - 1, output->control_block->ctx);
}

void log_output_timestamp_freq_set(uint32_t frequency)
{
	timestamp_div = 1U;
	/* There is no point to have frequency higher than 1MHz (ns are not
	 * printed) and too high frequency leads to overflows in calculations.
	 */
	while (frequency > 1000000) {
		frequency /= 2U;
		timestamp_div *= 2U;
	}

	freq = frequency;
}

uint64_t log_output_timestamp_to_us(log_timestamp_t timestamp)
{
	timestamp /= timestamp_div;

	return ((uint64_t) timestamp * 1000000U) / freq;
}
