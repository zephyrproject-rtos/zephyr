/*
 * Copyright (c) 2018 omSquare s.r.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log_backend.h>
#include <logging/log_core.h>
#include <logging/log_msg.h>
#include <logging/log_output.h>
#include "log_backend_std.h"
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

#define DROP_MAX 99

#define DROP_MSG "messages dropped:    \r\n"

#define DROP_MSG_LEN (sizeof(DROP_MSG) - 1)

#define MESSAGE_SIZE CONFIG_LOG_BACKEND_RTT_MESSAGE_SIZE

#define CHAR_BUF_SIZE \
	((IS_ENABLED(CONFIG_LOG_BACKEND_RTT_MODE_BLOCK) && \
	 !IS_ENABLED(CONFIG_LOG_IMMEDIATE)) ? \
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

static inline bool is_sync_mode(void)
{
	return IS_ENABLED(CONFIG_LOG_IMMEDIATE) || panic_mode;
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

	RTT_LOCK();
	int ret = SEGGER_RTT_WriteSkipNoLock(CONFIG_LOG_BACKEND_RTT_BUFFER,
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
		}

		ret = SEGGER_RTT_WriteSkipNoLock(CONFIG_LOG_BACKEND_RTT_BUFFER,
						 data, length);
		if (!is_sync_mode()) {
			RTT_UNLOCK();
		}

		if (ret) {
			on_write(retry_cnt);
		} else if (host_present) {
			retry_cnt--;
			on_failed_write(retry_cnt);
		}
	} while ((ret == 0) && host_present);

	return ((ret == 0) && host_present) ? 0 : length;
}

LOG_OUTPUT_DEFINE(log_output_rtt,
		  IS_ENABLED(CONFIG_LOG_BACKEND_RTT_MODE_BLOCK) ?
			  data_out_block_mode : data_out_drop_mode,
		  char_buf, sizeof(char_buf));

static void put(const struct log_backend *const backend,
		struct log_msg *msg)
{
	uint32_t flag = IS_ENABLED(CONFIG_LOG_BACKEND_RTT_SYST_ENABLE) ?
		LOG_OUTPUT_FLAG_FORMAT_SYST : 0;

	log_backend_std_put(&log_output_rtt, flag, msg);
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

	log_backend_std_dropped(&log_output_rtt, cnt);
}

static void sync_string(const struct log_backend *const backend,
		     struct log_msg_ids src_level, uint32_t timestamp,
		     const char *fmt, va_list ap)
{
	uint32_t flag = IS_ENABLED(CONFIG_LOG_BACKEND_RTT_SYST_ENABLE) ?
		LOG_OUTPUT_FLAG_FORMAT_SYST : 0;

	log_backend_std_sync_string(&log_output_rtt, flag, src_level,
				    timestamp, fmt, ap);
}

static void sync_hexdump(const struct log_backend *const backend,
			 struct log_msg_ids src_level, uint32_t timestamp,
			 const char *metadata, const uint8_t *data, uint32_t length)
{
	uint32_t flag = IS_ENABLED(CONFIG_LOG_BACKEND_RTT_SYST_ENABLE) ?
		LOG_OUTPUT_FLAG_FORMAT_SYST : 0;

	log_backend_std_sync_hexdump(&log_output_rtt, flag, src_level,
				     timestamp, metadata, data, length);
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
