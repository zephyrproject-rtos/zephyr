/** @file
 *  @brief Custom logging over UART
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stdbool.h>

#include <zephyr.h>
#include <device.h>
#include <init.h>
#include <drivers/console/uart_pipe.h>
#include <sys/byteorder.h>
#include <drivers/uart.h>

#include <logging/log_backend.h>
#include <logging/log_output.h>
#include <logging/log_ctrl.h>
#include <logging/log.h>

#include <bluetooth/buf.h>

#include "monitor.h"

/* This is the same default priority as for other console handlers,
 * except that we're not exporting it as a Kconfig variable until a
 * clear need arises.
 */
#define MONITOR_INIT_PRIORITY 60

/* These defines follow the values used by syslog(2) */
#define BT_LOG_ERR      3
#define BT_LOG_WARN     4
#define BT_LOG_INFO     6
#define BT_LOG_DBG      7

/* TS resolution is 1/10th of a millisecond */
#define MONITOR_TS_FREQ 10000

/* Maximum (string) length of a log message */
#define MONITOR_MSG_MAX 128

static const struct device *monitor_dev;

enum {
	BT_LOG_BUSY,
	BT_CONSOLE_BUSY,
};

static atomic_t flags;

static struct {
	atomic_t cmd;
	atomic_t evt;
	atomic_t acl_tx;
	atomic_t acl_rx;
#if defined(CONFIG_BT_BREDR)
	atomic_t sco_tx;
	atomic_t sco_rx;
#endif
	atomic_t other;
} drops;

static void monitor_send(const void *data, size_t len)
{
	const uint8_t *buf = data;

	while (len--) {
		uart_poll_out(monitor_dev, *buf++);
	}
}

static void encode_drops(struct bt_monitor_hdr *hdr, uint8_t type,
			 atomic_t *val)
{
	atomic_val_t count;

	count = atomic_set(val, 0);
	if (count) {
		hdr->ext[hdr->hdr_len++] = type;
		hdr->ext[hdr->hdr_len++] = MIN(count, 255);
	}
}

static uint32_t monitor_ts_get(void)
{
	return (k_cycle_get_32() /
		(sys_clock_hw_cycles_per_sec() / MONITOR_TS_FREQ));
}

static inline void encode_hdr(struct bt_monitor_hdr *hdr, uint32_t timestamp,
			      uint16_t opcode, uint16_t len)
{
	struct bt_monitor_ts32 *ts;

	hdr->opcode   = sys_cpu_to_le16(opcode);
	hdr->flags    = 0U;

	ts = (void *)hdr->ext;
	ts->type = BT_MONITOR_TS32;
	ts->ts32 = timestamp;
	hdr->hdr_len = sizeof(*ts);

	encode_drops(hdr, BT_MONITOR_COMMAND_DROPS, &drops.cmd);
	encode_drops(hdr, BT_MONITOR_EVENT_DROPS, &drops.evt);
	encode_drops(hdr, BT_MONITOR_ACL_TX_DROPS, &drops.acl_tx);
	encode_drops(hdr, BT_MONITOR_ACL_RX_DROPS, &drops.acl_rx);
#if defined(CONFIG_BT_BREDR)
	encode_drops(hdr, BT_MONITOR_SCO_TX_DROPS, &drops.sco_tx);
	encode_drops(hdr, BT_MONITOR_SCO_RX_DROPS, &drops.sco_rx);
#endif
	encode_drops(hdr, BT_MONITOR_OTHER_DROPS, &drops.other);

	hdr->data_len = sys_cpu_to_le16(4 + hdr->hdr_len + len);
}

static void drop_add(uint16_t opcode)
{
	switch (opcode) {
	case BT_MONITOR_COMMAND_PKT:
		atomic_inc(&drops.cmd);
		break;
	case BT_MONITOR_EVENT_PKT:
		atomic_inc(&drops.evt);
		break;
	case BT_MONITOR_ACL_TX_PKT:
		atomic_inc(&drops.acl_tx);
		break;
	case BT_MONITOR_ACL_RX_PKT:
		atomic_inc(&drops.acl_rx);
		break;
#if defined(CONFIG_BT_BREDR)
	case BT_MONITOR_SCO_TX_PKT:
		atomic_inc(&drops.sco_tx);
		break;
	case BT_MONITOR_SCO_RX_PKT:
		atomic_inc(&drops.sco_rx);
		break;
#endif
	default:
		atomic_inc(&drops.other);
		break;
	}
}

void bt_monitor_send(uint16_t opcode, const void *data, size_t len)
{
	struct bt_monitor_hdr hdr;

	if (atomic_test_and_set_bit(&flags, BT_LOG_BUSY)) {
		drop_add(opcode);
		return;
	}

	encode_hdr(&hdr, monitor_ts_get(), opcode, len);

	monitor_send(&hdr, BT_MONITOR_BASE_HDR_LEN + hdr.hdr_len);
	monitor_send(data, len);

	atomic_clear_bit(&flags, BT_LOG_BUSY);
}

void bt_monitor_new_index(uint8_t type, uint8_t bus, bt_addr_t *addr,
			  const char *name)
{
	struct bt_monitor_new_index pkt;

	pkt.type = type;
	pkt.bus = bus;
	memcpy(pkt.bdaddr, addr, 6);
	strncpy(pkt.name, name, sizeof(pkt.name) - 1);
	pkt.name[sizeof(pkt.name) - 1] = '\0';

	bt_monitor_send(BT_MONITOR_NEW_INDEX, &pkt, sizeof(pkt));
}

#if !defined(CONFIG_UART_CONSOLE) && !defined(CONFIG_LOG_PRINTK)
static int monitor_console_out(int c)
{
	static char buf[MONITOR_MSG_MAX];
	static size_t len;

	if (atomic_test_and_set_bit(&flags, BT_CONSOLE_BUSY)) {
		return c;
	}

	if (c != '\n' && len < sizeof(buf) - 1) {
		buf[len++] = c;
		atomic_clear_bit(&flags, BT_CONSOLE_BUSY);
		return c;
	}

	buf[len++] = '\0';

	bt_monitor_send(BT_MONITOR_SYSTEM_NOTE, buf, len);
	len = 0;

	atomic_clear_bit(&flags, BT_CONSOLE_BUSY);

	return c;
}

extern void __printk_hook_install(int (*fn)(int));
extern void __stdout_hook_install(int (*fn)(int));
#endif /* !CONFIG_UART_CONSOLE */

#if defined(CONFIG_HAS_DTS) && !defined(CONFIG_BT_MONITOR_ON_DEV_NAME)
#define CONFIG_BT_MONITOR_ON_DEV_NAME CONFIG_UART_CONSOLE_ON_DEV_NAME
#endif

#ifndef CONFIG_LOG_MINIMAL
struct monitor_log_ctx {
	size_t total_len;
	char msg[MONITOR_MSG_MAX];
};

static int monitor_log_out(uint8_t *data, size_t length, void *user_data)
{
	struct monitor_log_ctx *ctx = user_data;
	size_t i;

	for (i = 0; i < length && ctx->total_len < sizeof(ctx->msg); i++) {
		/* With CONFIG_LOG_PRINTK the line terminator will come as
		 * as part of messages.
		 */
		if (IS_ENABLED(CONFIG_LOG_PRINTK) &&
		    (data[i] == '\r' || data[i] == '\n')) {
			break;
		}

		ctx->msg[ctx->total_len++] = data[i];
	}

	return length;
}

static uint8_t buf;

LOG_OUTPUT_DEFINE(monitor_log_output, monitor_log_out, &buf, 1);

static inline uint8_t monitor_priority_get(uint8_t log_level)
{
	static const uint8_t prios[] = {
		[LOG_LEVEL_NONE]  = 0,
		[LOG_LEVEL_ERR]   = BT_LOG_ERR,
		[LOG_LEVEL_WRN]   = BT_LOG_WARN,
		[LOG_LEVEL_INF]   = BT_LOG_INFO,
		[LOG_LEVEL_DBG]   = BT_LOG_DBG,
	};

	if (log_level < ARRAY_SIZE(prios)) {
		return prios[log_level];
	}

	return BT_LOG_DBG;
}

static void monitor_log_put(const struct log_backend *const backend,
			    struct log_msg *msg)
{
	struct bt_monitor_user_logging log;
	struct monitor_log_ctx ctx;
	struct bt_monitor_hdr hdr;
	const char id[] = "bt";

	log_msg_get(msg);

	log_output_ctx_set(&monitor_log_output, &ctx);

	ctx.total_len = 0;
	log_output_msg_process(&monitor_log_output, msg,
			       LOG_OUTPUT_FLAG_CRLF_NONE);

	if (atomic_test_and_set_bit(&flags, BT_LOG_BUSY)) {
		drop_add(BT_MONITOR_USER_LOGGING);
		log_msg_put(msg);
		return;
	}

	encode_hdr(&hdr, msg->hdr.timestamp, BT_MONITOR_USER_LOGGING,
		   sizeof(log) + sizeof(id) + ctx.total_len + 1);

	log.priority = monitor_priority_get(msg->hdr.ids.level);
	log.ident_len = sizeof(id);

	log_msg_put(msg);

	monitor_send(&hdr, BT_MONITOR_BASE_HDR_LEN + hdr.hdr_len);
	monitor_send(&log, sizeof(log));
	monitor_send(id, sizeof(id));
	monitor_send(ctx.msg, ctx.total_len);

	/* Terminate the string with null */
	uart_poll_out(monitor_dev, '\0');

	atomic_clear_bit(&flags, BT_LOG_BUSY);
}

static void monitor_log_panic(const struct log_backend *const backend)
{
}

static void monitor_log_init(void)
{
	log_set_timestamp_func(monitor_ts_get, MONITOR_TS_FREQ);
}

static const struct log_backend_api monitor_log_api = {
	.put = monitor_log_put,
	.panic = monitor_log_panic,
	.init = monitor_log_init,
};

LOG_BACKEND_DEFINE(bt_monitor, monitor_log_api, true);
#endif /* CONFIG_LOG_MINIMAL */

static int bt_monitor_init(const struct device *d)
{
	ARG_UNUSED(d);

	monitor_dev = device_get_binding(CONFIG_BT_MONITOR_ON_DEV_NAME);

	__ASSERT_NO_MSG(monitor_dev);

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	uart_irq_rx_disable(monitor_dev);
	uart_irq_tx_disable(monitor_dev);
#endif

#if !defined(CONFIG_UART_CONSOLE) && !defined(CONFIG_LOG_PRINTK)
	__printk_hook_install(monitor_console_out);
	__stdout_hook_install(monitor_console_out);
#endif

	return 0;
}

SYS_INIT(bt_monitor_init, PRE_KERNEL_1, MONITOR_INIT_PRIORITY);
