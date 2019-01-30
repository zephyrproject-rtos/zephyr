/*
 * Copyright (c) 2018 omSquare s.r.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log_backend.h>
#include <logging/log_core.h>
#include <logging/log_msg.h>
#include <logging/log_output.h>
#include <SEGGER_RTT.h>

#define DROP_MAX 99

#if CONFIG_LOG_BACKEND_RTT_MODE_DROP

#define DROP_MSG "\nmessages dropped:    \r"
#define DROP_MSG_LEN (sizeof(DROP_MSG) - 1)
#define MESSAGE_SIZE CONFIG_LOG_BACKEND_RTT_MESSAGE_SIZE
#define CHAR_BUF_SIZE 1
#define RETRY_DELAY_MS 10 /* Long enough to detect host presence */
#define RETRY_CNT 10      /* Big enough to detect host presence */
#else

#define DROP_MSG NULL
#define DROP_MSG_LEN 0
#define MESSAGE_SIZE 0
#define CHAR_BUF_SIZE CONFIG_LOG_BACKEND_RTT_OUTPUT_BUFFER_SIZE
#define RETRY_DELAY_MS CONFIG_LOG_BACKEND_RTT_RETRY_DELAY_MS
#define RETRY_CNT CONFIG_LOG_BACKEND_RTT_RETRY_CNT
#endif /* CONFIG_LOG_BACKEND_RTT_MODE_DROP */

#if CONFIG_LOG_BACKEND_RTT_BUFFER > 0

#define RTT_LOCK()
#define RTT_UNLOCK()
#define RTT_BUFFER_SIZE CONFIG_LOG_BACKEND_RTT_BUFFER_SIZE

#else

#define RTT_LOCK() SEGGER_RTT_LOCK()
#define RTT_UNLOCK() SEGGER_RTT_UNLOCK()
#define RTT_BUFFER_SIZE 0

#endif /* CONFIG_LOG_BACKEND_RTT_BUFFER > 0 */

static const char *drop_msg = DROP_MSG;
static u8_t rtt_buf[RTT_BUFFER_SIZE];
static u8_t line_buf[MESSAGE_SIZE + DROP_MSG_LEN];
static u8_t *line_pos;
static u8_t char_buf[CHAR_BUF_SIZE];
static int drop_cnt;
static int drop_warn;
static int panic_mode;

static bool host_present;

static int data_out_block_mode(u8_t *data, size_t length, void *ctx);
static int data_out_drop_mode(u8_t *data, size_t length, void *ctx);

static int char_out_drop_mode(u8_t data);
static int line_out_drop_mode(void);

static int data_out_drop_mode(u8_t *data, size_t length, void *ctx)
{
	(void) ctx;
	u8_t *pos;

	if (panic_mode) {
		return data_out_block_mode(data, length, ctx);
	}

	for (pos = data; pos < data + length; pos++) {
		if (char_out_drop_mode(*pos)) {
			break;
		}
	}

	return (int) (pos - data);
}

static int char_out_drop_mode(u8_t data)
{
	if (data == '\r') {
		if (line_out_drop_mode()) {
			return 1;
		}
		line_pos = drop_cnt > 0 ? line_buf + DROP_MSG_LEN : line_buf;
		return 0;
	}

	if (line_pos < line_buf + sizeof(line_buf) - 1) {
		*line_pos++ = data;
	}

	/* not enough space in line buffer, we have to wait for EOL */
	return 0;
}

static int line_out_drop_mode(void)
{
	*line_pos = '\r';

	if (drop_cnt > 0 && !drop_warn) {
		memmove(line_buf + DROP_MSG_LEN, line_buf,
			line_pos - line_buf);
		(void)memcpy(line_buf, drop_msg, DROP_MSG_LEN);
		line_pos += DROP_MSG_LEN;
		drop_warn = 1;
	}

	if (drop_warn) {
		int cnt = min(drop_cnt, DROP_MAX);

		if (cnt < 10) {
			line_buf[DROP_MSG_LEN - 2] = ' ';
			line_buf[DROP_MSG_LEN - 3] = (u8_t) ('0' + cnt);
			line_buf[DROP_MSG_LEN - 4] = ' ';
		} else {
			line_buf[DROP_MSG_LEN - 2] = (u8_t) ('0' + cnt % 10);
			line_buf[DROP_MSG_LEN - 3] = (u8_t) ('0' + cnt / 10);
			line_buf[DROP_MSG_LEN - 4] = '>';
		}
	}

	RTT_LOCK();
	int ret = SEGGER_RTT_WriteSkipNoLock(CONFIG_LOG_BACKEND_RTT_BUFFER,
					     line_buf, line_pos - line_buf + 1);
	RTT_UNLOCK();

	if (ret == 0) {
		drop_cnt++;
		return 0;
	}

	drop_cnt = 0;
	drop_warn = 0;
	return 0;
}

static void on_failed_write(int retry_cnt)
{
	if (retry_cnt == 0) {
		host_present = false;
	} else if (panic_mode) {
		k_busy_wait(USEC_PER_MSEC * RETRY_DELAY_MS);
	} else {
		k_sleep(RETRY_DELAY_MS);
	}
}

static void on_write(int retry_cnt)
{
	host_present = true;
	if (panic_mode) {
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

static int data_out_block_mode(u8_t *data, size_t length, void *ctx)
{
	int ret;
	int retry_cnt = RETRY_CNT;

	do {
		if (!panic_mode) {
			RTT_LOCK();
		}

		ret = SEGGER_RTT_WriteSkipNoLock(CONFIG_LOG_BACKEND_RTT_BUFFER,
						 data, length);

		if (!panic_mode) {
			RTT_UNLOCK();
		}

		if (ret) {
			on_write(retry_cnt);
		} else {
			retry_cnt--;
			on_failed_write(retry_cnt);
		}
	} while ((ret == 0) && host_present);

	return length;
}

LOG_OUTPUT_DEFINE(log_output, IS_ENABLED(CONFIG_LOG_BACKEND_RTT_MODE_BLOCK) ?
		  data_out_block_mode : data_out_drop_mode,
		  char_buf, sizeof(char_buf));

static void put(const struct log_backend *const backend,
		struct log_msg *msg)
{
	log_msg_get(msg);

	u32_t flags = LOG_OUTPUT_FLAG_LEVEL | LOG_OUTPUT_FLAG_TIMESTAMP;

	if (IS_ENABLED(CONFIG_LOG_BACKEND_SHOW_COLOR)) {
		flags |= LOG_OUTPUT_FLAG_COLORS;
	}

	if (IS_ENABLED(CONFIG_LOG_BACKEND_FORMAT_TIMESTAMP)) {
		flags |= LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP;
	}

	log_output_msg_process(&log_output, msg, flags);

	log_msg_put(msg);
}

static void log_backend_rtt_cfg(void)
{
	SEGGER_RTT_ConfigUpBuffer(CONFIG_LOG_BACKEND_RTT_BUFFER, "Logger",
				  rtt_buf, sizeof(rtt_buf),
				  SEGGER_RTT_MODE_NO_BLOCK_SKIP);
}

static void log_backend_rtt_init(void)
{
	if (CONFIG_LOG_BACKEND_RTT_BUFFER > 0) {
		log_backend_rtt_cfg();
	}

	host_present = true;
	panic_mode = 0;
	line_pos = line_buf;
}

static void panic(struct log_backend const *const backend)
{
	log_output_flush(&log_output);
	panic_mode = 1;
}

static void dropped(const struct log_backend *const backend, u32_t cnt)
{
	ARG_UNUSED(backend);

	log_output_dropped_process(&log_output, cnt);
}

static void sync_string(const struct log_backend *const backend,
		     struct log_msg_ids src_level, u32_t timestamp,
		     const char *fmt, va_list ap)
{
	u32_t flags = LOG_OUTPUT_FLAG_LEVEL | LOG_OUTPUT_FLAG_TIMESTAMP;
	u32_t key;

	if (IS_ENABLED(CONFIG_LOG_BACKEND_SHOW_COLOR)) {
		flags |= LOG_OUTPUT_FLAG_COLORS;
	}

	if (IS_ENABLED(CONFIG_LOG_BACKEND_FORMAT_TIMESTAMP)) {
		flags |= LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP;
	}

	key = irq_lock();
	log_output_string(&log_output, src_level, timestamp, fmt, ap, flags);
	irq_unlock(key);
}

static void sync_hexdump(const struct log_backend *const backend,
			 struct log_msg_ids src_level, u32_t timestamp,
			 const char *metadata, const u8_t *data, u32_t length)
{
	u32_t flags = LOG_OUTPUT_FLAG_LEVEL | LOG_OUTPUT_FLAG_TIMESTAMP;
	u32_t key;

	if (IS_ENABLED(CONFIG_LOG_BACKEND_SHOW_COLOR)) {
		flags |= LOG_OUTPUT_FLAG_COLORS;
	}

	if (IS_ENABLED(CONFIG_LOG_BACKEND_FORMAT_TIMESTAMP)) {
		flags |= LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP;
	}

	key = irq_lock();
	log_output_hexdump(&log_output, src_level, timestamp,
			metadata, data, length, flags);
	irq_unlock(key);
}

const struct log_backend_api log_backend_rtt_api = {
	.put = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ? NULL : put,
	.put_sync_string = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ?
			sync_string : NULL,
	.put_sync_hexdump = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ?
			sync_hexdump : NULL,
	.panic = panic,
	.init = log_backend_rtt_init,
	.dropped = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ? NULL : dropped,
};

LOG_BACKEND_DEFINE(log_backend_rtt, log_backend_rtt_api, true);
