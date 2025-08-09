/*
 * Copyright (c) 2025 Michael Hope <michaelh@juju.nz>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch_qingke_debug

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/winstream.h>

#define TX_TIMEOUT   1000000
#define TX_FULL      BIT(7)
#define TX_VALID     BIT(2)
#define TX_SIZE_MASK 0x03

/* Offset of the data0 register */
#define DATA0 0x04

/*
 * Tracks if the host has been detected. Used to prevent spinning when the console is enabled but
 * the host is not connected.
 */
enum qingke_debug_state {
	QINGKE_DEBUG_STATE_INITIAL,
	/* The first buffer has been written to the host. */
	QINGKE_DEBUG_STATE_FIRST,
	/* The first buffer was acknowledged by the host. */
	QINGKE_DEBUG_STATE_ESTABLISHED,
	/* Timeout while trying to send the second buffer to the host. */
	QINGKE_DEBUG_STATE_MISSING,
};

struct qingkey_debug_data {
	/* Encoded text to be written to the host. */
	uint32_t buffer;
	enum qingke_debug_state state;
};

static struct qingkey_debug_data qingkey_debug_data_0;

#if defined(CONFIG_STDOUT_CONSOLE)
extern void __stdout_hook_install(int (*hook)(int));
#endif

#if defined(CONFIG_PRINTK)
extern void __printk_hook_install(int (*fn)(int));
#endif

static int qingke_debug_console_putc(int ch)
{
	/*
	 * `minichlink` encodes the incoming and outgoing characters into `data0`. The least
	 * significant byte is used for control and length, while the upper bytes are used for
	 * up to three characters. The `TX_FULL` bit is used to pass ownership back and forth
	 * between the host and device.
	 */
	uint32_t regs = DT_INST_REG_ADDR(0);
	struct qingkey_debug_data *data = &qingkey_debug_data_0;
	int count = data->buffer & TX_SIZE_MASK;
	int timeout;

	data->buffer |= (ch << (++count * 8));
	data->buffer++;

	/*
	 * Try to send if the buffer is full, or the character is a space or a control character, or
	 * the host is ready.
	 */
	if (count == 3 || ch <= ' ' || (sys_read32(regs + DATA0) & TX_FULL) == 0) {
		if (data->state != QINGKE_DEBUG_STATE_MISSING) {
			/* Host might be there. Spin until the buffer empties. */
			for (timeout = 0;
			     timeout != TX_TIMEOUT && (sys_read32(regs + DATA0) & TX_FULL) != 0;
			     timeout++) {
			}
		}
		if ((sys_read32(regs + DATA0) & TX_FULL) == 0) {
			sys_write32(data->buffer | TX_FULL | TX_VALID, regs + DATA0);
			if (data->state < QINGKE_DEBUG_STATE_ESTABLISHED) {
				/* Transitions from INITIAL -> FIRST -> ESTABLISHED */
				data->state++;
			} else if (data->state == QINGKE_DEBUG_STATE_MISSING) {
				/* The host has caught up. */
				data->state = QINGKE_DEBUG_STATE_ESTABLISHED;
			}
		} else {
			if (data->state == QINGKE_DEBUG_STATE_FIRST) {
				data->state = QINGKE_DEBUG_STATE_MISSING;
			}
		}
		data->buffer = 0;
	}

	return 1;
}

static int qingke_debug_console_init(void)
{
#if defined(CONFIG_STDOUT_CONSOLE)
	__stdout_hook_install(qingke_debug_console_putc);
#endif
#if defined(CONFIG_PRINTK)
	__printk_hook_install(qingke_debug_console_putc);
#endif
	return 0;
}

SYS_INIT(qingke_debug_console_init, PRE_KERNEL_1, CONFIG_CONSOLE_INIT_PRIORITY);
