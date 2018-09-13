/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log_output.h>
#include <logging/log_ctrl.h>
#include <assert.h>
#include <ctype.h>

#define HEXDUMP_BYTES_IN_LINE 8

#define LOG_COLOR_CODE_DEFAULT "\x1B[0m"
#define LOG_COLOR_CODE_BLACK   "\x1B[1;30m"
#define LOG_COLOR_CODE_RED     "\x1B[1;31m"
#define LOG_COLOR_CODE_GREEN   "\x1B[1;32m"
#define LOG_COLOR_CODE_YELLOW  "\x1B[1;33m"
#define LOG_COLOR_CODE_BLUE    "\x1B[1;34m"
#define LOG_COLOR_CODE_MAGENTA "\x1B[1;35m"
#define LOG_COLOR_CODE_CYAN    "\x1B[1;36m"
#define LOG_COLOR_CODE_WHITE   "\x1B[1;37m"

static const char *const severity[] = {
	NULL,
	"err",
	"wrn",
	"inf",
	"dbg"
};

static const char *const colors[] = {
	NULL,
	LOG_COLOR_CODE_RED,     /* err */
	LOG_COLOR_CODE_YELLOW,  /* warn */
	NULL,                   /* info */
	NULL                    /* dbg */
};

static u32_t freq;
static u32_t timestamp_div;

typedef int (*out_func_t)(int c, void *ctx);

extern int _prf(int (*func)(), void *dest, char *format, va_list vargs);
extern void _vprintk(out_func_t out, void *log_output, const char *fmt, va_list ap);

static int out_func(int c, void *ctx)
{
	const struct log_output *out_ctx =
					(const struct log_output *)ctx;

	out_ctx->buf[out_ctx->control_block->offset] = (u8_t)c;
	out_ctx->control_block->offset++;

	assert(out_ctx->control_block->offset <= out_ctx->size);

	if (out_ctx->control_block->offset == out_ctx->size) {
		log_output_flush(out_ctx);
	}

	return 0;
}

static int print_formatted(const struct log_output *log_output,
			   const char *fmt, ...)
{
	va_list args;
	int length = 0;

	va_start(args, fmt);
#if !defined(CONFIG_NEWLIB_LIBC) && !defined(CONFIG_ARCH_POSIX)
	length = _prf(out_func, (void *)log_output, (char *)fmt, args);
#else
	_vprintk(out_func, (void *)log_output, fmt, args);
#endif
	va_end(args);

	return length;
}

void log_output_flush(const struct log_output *log_output)
{
	int offset = 0;
	int len = log_output->control_block->offset;
	int processed;

	do {
		processed = log_output->func(&log_output->buf[offset], len,
					     log_output->control_block->ctx);
		len -= processed;
		offset += processed;
	} while (len);

	log_output->control_block->offset = 0;
}

static int timestamp_print(struct log_msg *msg,
			   const struct log_output *log_output,
			   bool format)
{
	int length;
	u32_t timestamp = log_msg_timestamp_get(msg);

	if (!format) {
		length = print_formatted(log_output, "[%08lu] ", timestamp);
	} else if (freq) {
		u32_t remainder;
		u32_t seconds;
		u32_t hours;
		u32_t mins;
		u32_t ms;
		u32_t us;

		timestamp /= timestamp_div;
		seconds = timestamp / freq;
		hours = seconds / 3600;
		seconds -= hours * 3600;
		mins = seconds / 60;
		seconds -= mins * 60;

		remainder = timestamp % freq;
		ms = (remainder * 1000) / freq;
		us = (1000 * (1000 * remainder - (ms * freq))) / freq;

		length = print_formatted(log_output,
					 "[%02d:%02d:%02d.%03d,%03d] ",
					 hours, mins, seconds, ms, us);
	} else {
		length = 0;
	}

	return length;
}

static void color_print(struct log_msg *msg,
			const struct log_output *log_output,
			bool color,
			bool start)
{
	if (color) {
		u32_t level = log_msg_level_get(msg);

		if (colors[level] != NULL) {
			const char *color = start ?
					 colors[level] : LOG_COLOR_CODE_DEFAULT;

			print_formatted(log_output, "%s", color);
		}
	}
}

static void color_prefix(struct log_msg *msg,
			 const struct log_output *log_output,
			 bool color)
{
	color_print(msg, log_output, color, true);
}

static void color_postfix(struct log_msg *msg,
			  const struct log_output *log_output,
			  bool color)
{
	color_print(msg, log_output, color, false);
}


static int ids_print(struct log_msg *msg,
		     const struct log_output *log_output)
{
	u32_t domain_id = log_msg_domain_id_get(msg);
	u32_t source_id = log_msg_source_id_get(msg);
	u32_t level = log_msg_level_get(msg);

	return print_formatted(log_output, "<%s> %s: ", severity[level],
		     log_source_name_get(domain_id, source_id));
}

static void newline_print(const struct log_output *ctx)
{
	print_formatted(ctx, "\r\n");
}

static void std_print(struct log_msg *msg,
		      const struct log_output *log_output)
{
	const char *str = log_msg_str_get(msg);

	switch (log_msg_nargs_get(msg)) {
	case 0:
		print_formatted(log_output, str);
		break;
	case 1:
		print_formatted(log_output, str, log_msg_arg_get(msg, 0));
		break;
	case 2:
		print_formatted(log_output, str,
		      log_msg_arg_get(msg, 0),
		      log_msg_arg_get(msg, 1));
		break;
	case 3:
		print_formatted(log_output, str,
		      log_msg_arg_get(msg, 0),
		      log_msg_arg_get(msg, 1),
		      log_msg_arg_get(msg, 2));
		break;
	case 4:
		print_formatted(log_output, str,
		      log_msg_arg_get(msg, 0),
		      log_msg_arg_get(msg, 1),
		      log_msg_arg_get(msg, 2),
		      log_msg_arg_get(msg, 3));
		break;
	case 5:
		print_formatted(log_output, str,
		      log_msg_arg_get(msg, 0),
		      log_msg_arg_get(msg, 1),
		      log_msg_arg_get(msg, 2),
		      log_msg_arg_get(msg, 3),
		      log_msg_arg_get(msg, 4));
		break;
	case 6:
		print_formatted(log_output, str,
		      log_msg_arg_get(msg, 0),
		      log_msg_arg_get(msg, 1),
		      log_msg_arg_get(msg, 2),
		      log_msg_arg_get(msg, 3),
		      log_msg_arg_get(msg, 4),
		      log_msg_arg_get(msg, 5));
		break;
	case 7:
		print_formatted(log_output, str,
		      log_msg_arg_get(msg, 0),
		      log_msg_arg_get(msg, 1),
		      log_msg_arg_get(msg, 2),
		      log_msg_arg_get(msg, 3),
		      log_msg_arg_get(msg, 4),
		      log_msg_arg_get(msg, 5),
		      log_msg_arg_get(msg, 6));
		break;
	case 8:
		print_formatted(log_output, str,
		      log_msg_arg_get(msg, 0),
		      log_msg_arg_get(msg, 1),
		      log_msg_arg_get(msg, 2),
		      log_msg_arg_get(msg, 3),
		      log_msg_arg_get(msg, 4),
		      log_msg_arg_get(msg, 5),
		      log_msg_arg_get(msg, 6),
		      log_msg_arg_get(msg, 7));
		break;
	case 9:
		print_formatted(log_output, str,
		      log_msg_arg_get(msg, 0),
		      log_msg_arg_get(msg, 1),
		      log_msg_arg_get(msg, 2),
		      log_msg_arg_get(msg, 3),
		      log_msg_arg_get(msg, 4),
		      log_msg_arg_get(msg, 5),
		      log_msg_arg_get(msg, 6),
		      log_msg_arg_get(msg, 7),
		      log_msg_arg_get(msg, 8));
		break;
	default:
		/* Unsupported number of arguments. */
		assert(true);
	}
}

static u32_t hexdump_line_print(struct log_msg *msg,
				const struct log_output *log_output,
				int prefix_offset,
				u32_t offset)
{
	u8_t buf[HEXDUMP_BYTES_IN_LINE];
	size_t length = sizeof(buf);

	log_msg_hexdump_data_get(msg, buf, &length, offset);

	if (length > 0) {
		newline_print(log_output);

		for (int i = 0; i < prefix_offset; i++) {
			print_formatted(log_output, " ");
		}

		for (int i = 0; i < HEXDUMP_BYTES_IN_LINE; i++) {
			if (i < length) {
				print_formatted(log_output, "%02x ", buf[i]);
			} else {
				print_formatted(log_output, "   ");
			}
		}

		print_formatted(log_output, "|");

		for (int i = 0; i < HEXDUMP_BYTES_IN_LINE; i++) {
			if (i < length) {
				char c = (char)buf[i];

				print_formatted(log_output, "%c",
				      isprint((int)c) ? c : '.');
			} else {
				print_formatted(log_output, " ");
			}
		}
	}

	return length;
}

static void hexdump_print(struct log_msg *msg,
			  const struct log_output *log_output,
			  int prefix_offset)
{
	u32_t offset = 0;
	u32_t length;

	print_formatted(log_output, "%s", log_msg_str_get(msg));

	do {
		length = hexdump_line_print(msg, log_output, prefix_offset,
					    offset);

		if (length < HEXDUMP_BYTES_IN_LINE) {
			break;
		}

		offset += length;
	} while (1);
}

static void raw_string_print(struct log_msg *msg,
			     const struct log_output *log_output)
{
	assert(log_output->size);

	size_t offset = 0;
	size_t length;

	do {
		length = log_output->size;
		/* Sting is stored in a hexdump message. */
		log_msg_hexdump_data_get(msg, log_output->buf, &length, offset);
		log_output->control_block->offset = length;
		log_output_flush(log_output);
		offset += length;
	} while (length > 0);

	print_formatted(log_output, "\r");
}

static int prefix_print(struct log_msg *msg,
			const struct log_output *log_output,
			u32_t flags)
{
	int length = 0;

	if (!log_msg_is_raw_string(msg)) {
		bool stamp_format = flags & LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP;
		bool colors_on = flags & LOG_OUTPUT_FLAG_COLORS;

		length += timestamp_print(msg, log_output, stamp_format);
		color_prefix(msg, log_output, colors_on);
		length += ids_print(msg, log_output);
	}

	return length;
}

static void postfix_print(struct log_msg *msg,
			  const struct log_output *log_output,
			  u32_t flags)
{
	if (!log_msg_is_raw_string(msg)) {
		color_postfix(msg, log_output,
			      (flags & LOG_OUTPUT_FLAG_COLORS));
		newline_print(log_output);
	}
}

void log_output_msg_process(const struct log_output *log_output,
			    struct log_msg *msg,
			    u32_t flags)
{
	int prefix_offset = prefix_print(msg, log_output, flags);

	if (log_msg_is_std(msg)) {
		std_print(msg, log_output);
	} else if (log_msg_is_raw_string(msg)) {
		raw_string_print(msg, log_output);
	} else {
		hexdump_print(msg, log_output, prefix_offset);
	}

	postfix_print(msg, log_output, flags);

	log_output_flush(log_output);
}

void log_output_timestamp_freq_set(u32_t frequency)
{
	timestamp_div = 1;
	/* There is no point to have frequency higher than 1MHz (ns are not
	 * printed) and too high frequency leads to overflows in calculations.
	 */
	while (frequency > 1000000) {
		frequency /= 2;
		timestamp_div *= 2;
	}

	freq = frequency;
}
