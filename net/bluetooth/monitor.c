/** @file
 *  @brief Custom logging over UART
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#include <stdbool.h>

#include <nanokernel.h>
#include <device.h>
#include <init.h>
#include <drivers/console/uart_pipe.h>
#include <misc/byteorder.h>
#include <uart.h>

#include <bluetooth/buf.h>
#include <bluetooth/log.h>

#include "monitor.h"

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

static void send_hdr(uint16_t opcode, uint16_t index, uint16_t len)
{
	struct bt_monitor_hdr hdr;

	hdr.opcode = sys_cpu_to_le16(opcode);
	hdr.index = sys_cpu_to_le16(index);
	hdr.len = sys_cpu_to_le16(len);

	monitor_send(&hdr, sizeof(hdr));
}

static int log_out(int c, void *unused)
{
	uart_poll_out(monitor_dev, c);
	return 0;
}

void bt_log(int prio, const char *fmt, ...)
{
	struct bt_monitor_user_logging log;
	const char id[] = "bt";
	va_list ap;
	int len, key;

	va_start(ap, fmt);
	len = vsnprintf(NULL, 0, fmt, ap);
	va_end(ap);

	if (len < 0) {
		return;
	}

	log.priority = prio;
	log.ident_len = sizeof(id);

	key = irq_lock();

	send_hdr(BT_MONITOR_USER_LOGGING, 0,
		 sizeof(log) + sizeof(id) + len + 1);
	monitor_send(&log, sizeof(log));
	monitor_send(id, sizeof(id));

	va_start(ap, fmt);
	_prf(log_out, NULL, fmt, ap);
	va_end(ap);

	/* Terminate the string with null */
	uart_poll_out(monitor_dev, '\0');

	irq_unlock(key);
}

void bt_monitor_send(uint16_t opcode, const void *data, size_t len)
{
	int key;

	key = irq_lock();

	send_hdr(opcode, 0, len);
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

	uart_irq_rx_disable(monitor_dev);
	uart_irq_tx_disable(monitor_dev);

#if !defined(CONFIG_UART_CONSOLE)
	__printk_hook_install(monitor_console_out);
	__stdout_hook_install(monitor_console_out);
#endif

	return 0;
}

SYS_INIT(bt_monitor_init, PRIMARY, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
