/** @file
 *  @brief Custom logging over UART
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
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

extern int _prf(int (*func)(), void *dest,
		const char *format, va_list vargs);

static void monitor_send(const void *data, size_t len)
{
	const uint8_t *buf = data;

	while (len--) {
		uart_poll_out(monitor_dev, *buf++);
	}
}

static inline void encode_hdr(struct bt_monitor_hdr *hdr, uint16_t opcode,
			      uint16_t len)
{
	uint32_t ts32;

	hdr->hdr_len  = sizeof(hdr->type) + sizeof(hdr->ts32);
	hdr->data_len = sys_cpu_to_le16(4 + hdr->hdr_len + len);
	hdr->opcode   = sys_cpu_to_le16(opcode);
	hdr->flags    = 0;

	/* Extended header */
	hdr->type = BT_MONITOR_TS32;
	ts32 = k_uptime_get() * 10;
	hdr->ts32 = sys_cpu_to_le32(ts32);
}

static int log_out(int c, void *unused)
{
	uart_poll_out(monitor_dev, c);
	return 0;
}

void bt_log(int prio, const char *fmt, ...)
{
	struct bt_monitor_user_logging log;
	struct bt_monitor_hdr hdr;
	const char id[] = "bt";
	va_list ap;
	int len, key;

	va_start(ap, fmt);
	len = vsnprintk(NULL, 0, fmt, ap);
	va_end(ap);

	if (len < 0) {
		return;
	}

	log.priority = prio;
	log.ident_len = sizeof(id);

	encode_hdr(&hdr, BT_MONITOR_USER_LOGGING,
		   sizeof(log) + sizeof(id) + len + 1);

	key = irq_lock();

	monitor_send(&hdr, sizeof(hdr));
	monitor_send(&log, sizeof(log));
	monitor_send(id, sizeof(id));

	va_start(ap, fmt);
	_vprintk(log_out, NULL, fmt, ap);
	va_end(ap);

	/* Terminate the string with null */
	uart_poll_out(monitor_dev, '\0');

	irq_unlock(key);
}

void bt_monitor_send(uint16_t opcode, const void *data, size_t len)
{
	struct bt_monitor_hdr hdr;
	int key;

	encode_hdr(&hdr, opcode, len);

	key = irq_lock();

	monitor_send(&hdr, sizeof(hdr));
	monitor_send(data, len);

	irq_unlock(key);
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

#if !defined(CONFIG_UART_CONSOLE)
static int monitor_console_out(int c)
{
	static char buf[128];
	static size_t len;
	int key;

	key = irq_lock();

	if (c != '\n' && len < sizeof(buf) - 1) {
		buf[len++] = c;
		irq_unlock(key);
		return c;
	}

	buf[len++] = '\0';

	bt_monitor_send(BT_MONITOR_SYSTEM_NOTE, buf, len);
	len = 0;

	irq_unlock(key);

	return c;
}

extern void __printk_hook_install(int (*fn)(int));
extern void __stdout_hook_install(int (*fn)(int));
#endif /* !CONFIG_UART_CONSOLE */

static int bt_monitor_init(struct device *d)
{
	ARG_UNUSED(d);

	monitor_dev = device_get_binding(CONFIG_BLUETOOTH_MONITOR_ON_DEV_NAME);

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
