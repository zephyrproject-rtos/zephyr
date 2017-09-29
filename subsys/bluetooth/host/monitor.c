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
#include <misc/byteorder.h>
#include <misc/printk.h>
#include <uart.h>

#include <bluetooth/buf.h>

#include "monitor.h"

/* This is the same default priority as for other console handlers,
 * except that we're not exporting it as a Kconfig variable until a
 * clear need arises.
 */
#define MONITOR_INIT_PRIORITY 60

static struct device *monitor_dev;

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

extern int _prf(int (*func)(), void *dest,
		const char *format, va_list vargs);

static void monitor_send(const void *data, size_t len)
{
	const u8_t *buf = data;

	while (len--) {
		uart_poll_out(monitor_dev, *buf++);
	}
}

static void encode_drops(struct bt_monitor_hdr *hdr, u8_t type,
			 atomic_t *val)
{
	atomic_val_t count;

	count = atomic_set(val, 0);
	if (count) {
		hdr->ext[hdr->hdr_len++] = type;
		hdr->ext[hdr->hdr_len++] = min(count, 255);
	}
}

static inline void encode_hdr(struct bt_monitor_hdr *hdr, u16_t opcode,
			      u16_t len)
{
	struct bt_monitor_ts32 *ts;

	hdr->opcode   = sys_cpu_to_le16(opcode);
	hdr->flags    = 0;

	ts = (void *)hdr->ext;
	ts->type = BT_MONITOR_TS32;
	ts->ts32 = sys_cpu_to_le32(k_uptime_get() * 10);
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

static int log_out(int c, void *unused)
{
	uart_poll_out(monitor_dev, c);
	return 0;
}

static void drop_add(u16_t opcode)
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

void bt_log(int prio, const char *fmt, ...)
{
	struct bt_monitor_user_logging log;
	struct bt_monitor_hdr hdr;
	const char id[] = "bt";
	va_list ap;
	int len;

	va_start(ap, fmt);
	len = vsnprintk(NULL, 0, fmt, ap);
	va_end(ap);

	if (len < 0) {
		return;
	}

	log.priority = prio;
	log.ident_len = sizeof(id);

	if (atomic_test_and_set_bit(&flags, BT_LOG_BUSY)) {
		drop_add(BT_MONITOR_USER_LOGGING);
		return;
	}

	encode_hdr(&hdr, BT_MONITOR_USER_LOGGING,
		   sizeof(log) + sizeof(id) + len + 1);

	monitor_send(&hdr, BT_MONITOR_BASE_HDR_LEN + hdr.hdr_len);
	monitor_send(&log, sizeof(log));
	monitor_send(id, sizeof(id));

	va_start(ap, fmt);
	_vprintk(log_out, NULL, fmt, ap);
	va_end(ap);

	/* Terminate the string with null */
	uart_poll_out(monitor_dev, '\0');

	atomic_clear_bit(&flags, BT_LOG_BUSY);
}

void bt_monitor_send(u16_t opcode, const void *data, size_t len)
{
	struct bt_monitor_hdr hdr;

	if (atomic_test_and_set_bit(&flags, BT_LOG_BUSY)) {
		drop_add(opcode);
		return;
	}

	encode_hdr(&hdr, opcode, len);

	monitor_send(&hdr, BT_MONITOR_BASE_HDR_LEN + hdr.hdr_len);
	monitor_send(data, len);

	atomic_clear_bit(&flags, BT_LOG_BUSY);
}

void bt_monitor_new_index(u8_t type, u8_t bus, bt_addr_t *addr,
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

#if !defined(CONFIG_UART_CONSOLE)
static int monitor_console_out(int c)
{
	static char buf[128];
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

static int bt_monitor_init(struct device *d)
{
	ARG_UNUSED(d);

	monitor_dev = device_get_binding(CONFIG_BT_MONITOR_ON_DEV_NAME);

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	uart_irq_rx_disable(monitor_dev);
	uart_irq_tx_disable(monitor_dev);
#endif

#if !defined(CONFIG_UART_CONSOLE)
	__printk_hook_install(monitor_console_out);
	__stdout_hook_install(monitor_console_out);
#endif

	return 0;
}

SYS_INIT(bt_monitor_init, PRE_KERNEL_1, MONITOR_INIT_PRIORITY);
