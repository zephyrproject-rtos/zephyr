/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log_output.h>
#include <logging/log_ctrl.h>
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

static const char *severity[] = {
	NULL,
	"err",
	"warn",
	"info",
	"dbg"
};

static const char *colors[] = {
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

static int out_func(int c, void *ctx)
{
	struct log_output_ctx *out_ctx = (struct log_output_ctx *)ctx;

	out_ctx->p_data[out_ctx->offset] = (u8_t)c;
	out_ctx->offset++;

	if (out_ctx->offset == out_ctx->length) {
		out_ctx->func(out_ctx->p_data, out_ctx->length, out_ctx->p_ctx);
		out_ctx->offset = 0;
	}
	return 0;
}

static int print(struct log_output_ctx *ctx, const char *fmt, ...)
{
	int length;
	va_list args;

	va_start( args, fmt);
	length = _prf(out_func, ctx, (char *)fmt, args);
	va_end(args);

	return length;
}

static void flush(struct log_output_ctx *ctx)
{
	ctx->func(ctx->p_data, ctx->offset, ctx->p_ctx);
}

static int timestamp_print(struct log_msg *p_msg,
			   struct log_output_ctx *p_ctx,
			   bool format)
{
	int length;
	u32_t timestamp = log_msg_timestamp_get(p_msg);

	if (!format) {
		length = print(p_ctx, "[%08lu] ", timestamp);
	} else   {
		timestamp /= timestamp_div;
		uint32_t seconds = timestamp / freq;
		uint32_t hours = seconds / 3600;
		seconds -= hours * 3600;
		uint32_t mins = seconds / 60;
		seconds -= mins * 60;

		uint32_t reminder = timestamp % freq;
		uint32_t ms = (reminder * 1000) / freq;
		uint32_t us = (1000 * (1000 * reminder - (ms * freq))) / freq;

		length = print(p_ctx, "[%02d:%02d:%02d.%03d,%03d] ",
			       hours, mins, seconds, ms, us);
	}
	return length;
}

static void color_print(struct log_msg *p_msg,
			struct log_output_ctx *p_ctx,
			bool color,
			bool start)
{
	if (color) {
		u32_t level = log_msg_level_get(p_msg);
		if (colors[level] != NULL) {
			const char *color = start ?
					    colors[level] : LOG_COLOR_CODE_DEFAULT;
			print(p_ctx, "%s", color);
		}
	}
}

static void color_prefix(struct log_msg *p_msg,
			 struct log_output_ctx *p_ctx,
			 bool color)
{
	color_print(p_msg, p_ctx, color, true);
}

static void color_postfix(struct log_msg *p_msg,
			  struct log_output_ctx *p_ctx,
			  bool color)
{
	color_print(p_msg, p_ctx, color, false);
}


static int ids_print(struct log_msg *p_msg,
		     struct log_output_ctx *p_ctx)
{
	u32_t level = log_msg_level_get(p_msg);
	u32_t source_id = log_msg_source_id_get(p_msg);
	u32_t domain_id = log_msg_domain_id_get(p_msg);

	return print(p_ctx, "<%s> %s: ", severity[level],
		     log_source_name_get(domain_id, source_id));
}

static void newline_print(struct log_output_ctx *p_ctx)
{
	print(p_ctx, "\r\n");
}

static void std_print(struct log_msg *p_msg,
		      struct log_output_ctx *p_ctx)
{
	const char *p_str = log_msg_str_get(p_msg);

	switch (log_msg_nargs_get(p_msg)) {
	case 0:
		print(p_ctx, p_str);
		break;
	case 1:
		print(p_ctx, p_str, log_msg_arg_get(p_msg, 0));
		break;
	case 2:
		print(p_ctx, p_str,
		      log_msg_arg_get(p_msg, 0),
		      log_msg_arg_get(p_msg, 1));
		break;
	case 3:
		print(p_ctx, p_str,
		      log_msg_arg_get(p_msg, 0),
		      log_msg_arg_get(p_msg, 1),
		      log_msg_arg_get(p_msg, 2));
		break;
	case 4:
		print(p_ctx, p_str,
		      log_msg_arg_get(p_msg, 0),
		      log_msg_arg_get(p_msg, 1),
		      log_msg_arg_get(p_msg, 2),
		      log_msg_arg_get(p_msg, 3));
		break;
	case 5:
		print(p_ctx, p_str,
		      log_msg_arg_get(p_msg, 0),
		      log_msg_arg_get(p_msg, 1),
		      log_msg_arg_get(p_msg, 2),
		      log_msg_arg_get(p_msg, 3),
		      log_msg_arg_get(p_msg, 4));
		break;
	case 6:
		print(p_ctx, p_str,
		      log_msg_arg_get(p_msg, 0),
		      log_msg_arg_get(p_msg, 1),
		      log_msg_arg_get(p_msg, 2),
		      log_msg_arg_get(p_msg, 3),
		      log_msg_arg_get(p_msg, 4),
		      log_msg_arg_get(p_msg, 5));
		break;
	}
}

static u32_t hexdump_line_print(struct log_msg *p_msg,
				struct log_output_ctx *p_ctx,
				int prefix_offset,
				u32_t offset)
{
	u8_t buf[HEXDUMP_BYTES_IN_LINE];
	u32_t length = sizeof(buf);

	log_msg_hexdump_data_get(p_msg, buf, &length, offset);

	if (length > 0) {
		if (offset > 0) {
			newline_print(p_ctx);
			for (int i = 0; i < prefix_offset; i++) {
				print(p_ctx, " ");
			}
		}
		for (int i = 0; i < HEXDUMP_BYTES_IN_LINE; i++) {
			if (i < length) {
				print(p_ctx, " %02x", buf[i]);
			} else   {
				print(p_ctx, "   ");
			}
		}
		print(p_ctx, "|");

		for (int i = 0; i < HEXDUMP_BYTES_IN_LINE; i++) {
			if (i < length) {
				char c = (char)buf[i];
				print(p_ctx, "%c", isprint((int)c) ? c : '.');
			} else   {
				print(p_ctx, " ");
			}
		}
	}

	return length;
}

static void hexdump_print(struct log_msg *p_msg,
			  struct log_output_ctx *p_ctx,
			  int prefix_offset)
{
	u32_t offset = 0;
	u32_t length;

	do {
		length = hexdump_line_print(p_msg, p_ctx, prefix_offset, offset);
		if (length < HEXDUMP_BYTES_IN_LINE) {
			break;
		} else   {
			offset += length;
		}
	} while (1);
}

static void raw_string_print(struct log_msg *p_msg,
			     struct log_output_ctx *p_ctx)
{
	size_t length;
	size_t offset = 0;

	while (length > 0) {
		length = p_ctx->length;
		log_msg_hexdump_data_get(p_msg, p_ctx->p_data, &length, offset);
		offset += length;
		p_ctx->func(p_ctx->p_data, length, p_ctx->p_ctx);
	}
	print(p_ctx, "\r");
}

static int prefix_print(struct log_msg *p_msg,
			struct log_output_ctx *p_ctx,
			u32_t flags)
{
	int length = 0;

	if (!log_msg_is_raw_string(p_msg)) {
		length += timestamp_print(p_msg, p_ctx, flags & LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP);
		color_prefix(p_msg, p_ctx, (flags & LOG_OUTPUT_FLAG_COLORS));
		length += ids_print(p_msg, p_ctx);
	}
	return length;
}

static void postfix_print(struct log_msg *p_msg,
			  struct log_output_ctx *p_ctx,
			  u32_t flags)
{
	if (!log_msg_is_raw_string(p_msg)) {
		color_postfix(p_msg, p_ctx, (flags & LOG_OUTPUT_FLAG_COLORS));
		newline_print(p_ctx);
	}
}

void log_output_msg_process(struct log_msg *p_msg,
			    struct log_output_ctx *p_ctx,
			    u32_t flags)
{
	int prefix_offset = prefix_print(p_msg, p_ctx, flags);

	if (log_msg_is_std(p_msg)) {
		std_print(p_msg, p_ctx);
	} else if (log_msg_is_raw_string(p_msg)) {
		raw_string_print(p_msg, p_ctx);
	} else   {
		hexdump_print(p_msg, p_ctx, prefix_offset);
	}

	postfix_print(p_msg, p_ctx, flags);

	flush(p_ctx);
}

void log_output_timestamp_freq_set(u32_t frequency)
{
	timestamp_div = 1;
	/* There is no point to have frequency higher than 1MHz (ns are not printed) and too high
	 * frequency leads to overflows in calculations.
	 */
	while (frequency > 1000000) {
		frequency /= 2;
		timestamp_div *= 2;
	}
	freq = frequency;
}
