/*
 * Copyright (c) 2018 omSquare s.r.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/logging/log_output_dict.h>
#include <zephyr/logging/log_backend_std.h>
#include <SEGGER_RTT.h>

#ifndef CONFIG_LOG_BACKEND_RTT_BUFFER_SIZE
#define CONFIG_LOG_BACKEND_RTT_BUFFER_SIZE 0
#endif

#ifndef CONFIG_LOG_BACKEND_RTT_MESSAGE_SIZE
#define CONFIG_LOG_BACKEND_RTT_MESSAGE_SIZE 0
#endif

#ifndef CONFIG_LOG_BACKEND_RTT_OUTPUT_BUFFER_SIZE
#define CONFIG_LOG_BACKEND_RTT_OUTPUT_BUFFER_SIZE 0
#endif

#ifndef CONFIG_LOG_BACKEND_RTT_RETRY_DELAY_MS
/* Long enough to detect host presence */
#define CONFIG_LOG_BACKEND_RTT_RETRY_DELAY_MS 10
#endif

#ifndef CONFIG_LOG_BACKEND_RTT_RETRY_CNT
/* Big enough to detect host presence */
#define CONFIG_LOG_BACKEND_RTT_RETRY_CNT 10
#endif

#if defined(CONFIG_LOG_BACKEND_RTT_OUTPUT_DICTIONARY)
static const uint8_t LOG_HEX_SEP[10] = "##ZLOGV1##";
#endif

#define DROP_MAX 99

#define DROP_MSG "messages dropped:    \r\n"

#define DROP_MSG_LEN (sizeof(DROP_MSG) - 1)

#define MESSAGE_SIZE CONFIG_LOG_BACKEND_RTT_MESSAGE_SIZE

#define CHAR_BUF_SIZE \
	((IS_ENABLED(CONFIG_LOG_BACKEND_RTT_MODE_BLOCK) && \
	 !IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE)) ? \
		CONFIG_LOG_BACKEND_RTT_OUTPUT_BUFFER_SIZE : 1)

#define RTT_LOCK() \
	COND_CODE_0(CONFIG_LOG_BACKEND_RTT_BUFFER, (SEGGER_RTT_LOCK()), ())

#define RTT_UNLOCK() \
	COND_CODE_0(CONFIG_LOG_BACKEND_RTT_BUFFER, (SEGGER_RTT_UNLOCK()), ())

#define RTT_BUFFER_SIZE \
	COND_CODE_0(CONFIG_LOG_BACKEND_RTT_BUFFER, \
		(0), (CONFIG_LOG_BACKEND_RTT_BUFFER_SIZE))


static const char *drop_msg = DROP_MSG;
static uint8_t rtt_buf[RTT_BUFFER_SIZE];
static uint8_t line_buf[MESSAGE_SIZE + DROP_MSG_LEN];
static uint8_t *line_pos;
static uint8_t char_buf[CHAR_BUF_SIZE];
static int drop_cnt;
static int drop_warn;
static bool panic_mode;
static bool host_present;

static int data_out_block_mode(uint8_t *data, size_t length, void *ctx);
static int data_out_drop_mode(uint8_t *data, size_t length, void *ctx);

static int char_out_drop_mode(uint8_t data);
static int line_out_drop_mode(void);
static uint32_t log_format_current = CONFIG_LOG_BACKEND_RTT_OUTPUT_DEFAULT;

static inline bool is_sync_mode(void)
{
	return IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE) || panic_mode;
}

static inline bool is_panic_mode(void)
{
	return panic_mode;
}

static int data_out_drop_mode(uint8_t *data, size_t length, void *ctx)
{
	(void) ctx;
	uint8_t *pos;

	if (is_sync_mode()) {
		return data_out_block_mode(data, length, ctx);
	}

	for (pos = data; pos < data + length; pos++) {
		if (char_out_drop_mode(*pos)) {
			break;
		}
	}

	return (int) (pos - data);
}

static int char_out_drop_mode(uint8_t data)
{
	if (data == '\n') {
		if (line_out_drop_mode()) {
			return 1;
		}
		line_pos = line_buf;
		return 0;
	}

	if (line_pos < line_buf + MESSAGE_SIZE - 1) {
		*line_pos++ = data;
	}

	/* not enough space in line buffer, we have to wait for EOL */
	return 0;
}

static int line_out_drop_mode(void)
{
	/* line cannot be empty */
	__ASSERT_NO_MSG(line_pos > line_buf);

	/* Handle the case if line contains only '\n' */
	if (line_pos - line_buf == 1) {
		line_pos++;
	}

	*(line_pos - 1) = '\r';
	*line_pos++ = '\n';

	if (drop_cnt > 0 && !drop_warn) {
		int cnt = MIN(drop_cnt, DROP_MAX);

		__ASSERT_NO_MSG(line_pos - line_buf <= MESSAGE_SIZE);

		memmove(line_buf + DROP_MSG_LEN, line_buf, line_pos - line_buf);
		(void)memcpy(line_buf, drop_msg, DROP_MSG_LEN);
		line_pos += DROP_MSG_LEN;
		drop_warn = 1;


		if (cnt < 10) {
			line_buf[DROP_MSG_LEN - 2] = ' ';
			line_buf[DROP_MSG_LEN - 3] = (uint8_t) ('0' + cnt);
			line_buf[DROP_MSG_LEN - 4] = ' ';
		} else {
			line_buf[DROP_MSG_LEN - 2] = (uint8_t) ('0' + cnt % 10);
			line_buf[DROP_MSG_LEN - 3] = (uint8_t) ('0' + cnt / 10);
			line_buf[DROP_MSG_LEN - 4] = '>';
		}
	}

	int ret;

	RTT_LOCK();
	ret = SEGGER_RTT_WriteSkipNoLock(CONFIG_LOG_BACKEND_RTT_BUFFER,
					 line_buf, line_pos - line_buf);
	RTT_UNLOCK();

	if (ret == 0) {
		drop_cnt++;
	} else {
		drop_cnt = 0;
		drop_warn = 0;
	}

	return 0;
}

static void on_failed_write(int retry_cnt)
{
	if (retry_cnt == 0) {
		host_present = false;
	} else if (is_sync_mode()) {
		k_busy_wait(USEC_PER_MSEC *
				CONFIG_LOG_BACKEND_RTT_RETRY_DELAY_MS);
	} else {
		k_msleep(CONFIG_LOG_BACKEND_RTT_RETRY_DELAY_MS);
	}
}

static void on_write(int retry_cnt)
{
	host_present = true;
	if (is_panic_mode()) {
		/* In panic mode block on each write until host reads it. This
		 * way it is ensured that if system resets all messages are read
		 * by the host. While pending on data being read by the host we
		 * must also detect situation where host is disconnected.
		 */
		while (SEGGER_RTT_HasDataUp(CONFIG_LOG_BACKEND_RTT_BUFFER) &&
			host_present) {
			on_failed_write(retry_cnt--);
		}
	}

}

static int data_out_block_mode(uint8_t *data, size_t length, void *ctx)
{
	int ret = 0;
	/* This function is also called in drop mode for synchronous operation
	 * in that case retry is undesired */
	int retry_cnt = IS_ENABLED(CONFIG_LOG_BACKEND_RTT_MODE_BLOCK) ?
			 CONFIG_LOG_BACKEND_RTT_RETRY_CNT : 1;

	do {
		if (!is_sync_mode()) {
			RTT_LOCK();
			ret = SEGGER_RTT_WriteSkipNoLock(CONFIG_LOG_BACKEND_RTT_BUFFER,
							 data, length);
			RTT_UNLOCK();
		} else {
			ret = SEGGER_RTT_WriteSkipNoLock(CONFIG_LOG_BACKEND_RTT_BUFFER,
							 data, length);
		}

		if (ret) {
			on_write(retry_cnt);
		} else if (host_present) {
			retry_cnt--;
			on_failed_write(retry_cnt);
		} else {
		}
	} while ((ret == 0) && host_present);

	return ((ret == 0) && host_present) ? 0 : length;
}

static int data_out_overwrite_mode(uint8_t *data, size_t length, void *ctx)
{
	if (!is_sync_mode()) {
		RTT_LOCK();
		SEGGER_RTT_WriteWithOverwriteNoLock(CONFIG_LOG_BACKEND_RTT_BUFFER,
						    data, length);

		RTT_UNLOCK();
	} else {
		SEGGER_RTT_WriteWithOverwriteNoLock(CONFIG_LOG_BACKEND_RTT_BUFFER,
						    data, length);
	}

	return length;
}

static const log_output_func_t logging_func =
	IS_ENABLED(CONFIG_LOG_BACKEND_RTT_MODE_BLOCK)       ? data_out_block_mode
	: IS_ENABLED(CONFIG_LOG_BACKEND_RTT_MODE_OVERWRITE) ? data_out_overwrite_mode
							    : data_out_drop_mode;

static int data_out(uint8_t *data, size_t length, void *ctx)
{
#if defined(CONFIG_LOG_BACKEND_RTT_OUTPUT_DICTIONARY)
	for (size_t i = 0; i < length; i++) {
		char c[2];
		uint8_t x[2];

		/* upper 8-bit */
		x[0] = data[i] >> 4;
		(void)hex2char(x[0], &c[0]);

		/* lower 8-bit */
		x[1] = data[i] & 0x0FU;
		(void)hex2char(x[1], &c[1]);
		logging_func(c, sizeof(c), ctx);
	}
	return length;
#endif

	return logging_func(data, length, ctx);
}

LOG_OUTPUT_DEFINE(log_output_rtt, data_out, char_buf, sizeof(char_buf));

static void log_backend_rtt_cfg(void)
{
	SEGGER_RTT_ConfigUpBuffer(CONFIG_LOG_BACKEND_RTT_BUFFER, "Logger",
				  rtt_buf, sizeof(rtt_buf),
				  SEGGER_RTT_MODE_NO_BLOCK_SKIP);
}

static void log_backend_rtt_init(struct log_backend const *const backend)
{
	if (CONFIG_LOG_BACKEND_RTT_BUFFER > 0) {
		log_backend_rtt_cfg();
	}

#if defined(CONFIG_LOG_BACKEND_RTT_OUTPUT_DICTIONARY)
	logging_func((uint8_t *)LOG_HEX_SEP, sizeof(LOG_HEX_SEP), NULL);
#endif

	host_present = true;
	line_pos = line_buf;
}

static void panic(struct log_backend const *const backend)
{
	panic_mode = true;
	log_backend_std_panic(&log_output_rtt);
}

static void dropped(const struct log_backend *const backend, uint32_t cnt)
{
	ARG_UNUSED(backend);

	if (IS_ENABLED(CONFIG_LOG_BACKEND_RTT_OUTPUT_DICTIONARY)) {
		log_dict_output_dropped_process(&log_output_rtt, cnt);
	} else {
		log_backend_std_dropped(&log_output_rtt, cnt);
	}
}

static void process(const struct log_backend *const backend,
		union log_msg_generic *msg)
{
	uint32_t flags = log_backend_std_get_flags();

	log_format_func_t log_output_func = log_format_func_t_get(log_format_current);

	log_output_func(&log_output_rtt, &msg->log, flags);
}

static int format_set(const struct log_backend *const backend, uint32_t log_type)
{
	log_format_current = log_type;
	return 0;
}

const struct log_backend_api log_backend_rtt_api = {
	.process = process,
	.panic = panic,
	.init = log_backend_rtt_init,
	.dropped = IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE) ? NULL : dropped,
	.format_set = format_set,
};

LOG_BACKEND_DEFINE(log_backend_rtt, log_backend_rtt_api, true);
