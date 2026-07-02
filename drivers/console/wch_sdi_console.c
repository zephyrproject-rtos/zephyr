/*
 * Copyright (c) 2026 Michael Hope <michaelh@juju.nz>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch_sdi_console

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
#define DATA0 0

/*
 * Tracks if the host has been detected. Used to prevent spinning when the console is enabled but
 * the host is not connected.
 */
enum wch_sdi_console_state {
	WCH_SDI_CONSOLE_STATE_INITIAL,
	/* The first buffer has been written to the host. */
	WCH_SDI_CONSOLE_STATE_FIRST,
	/* The first buffer was acknowledged by the host. */
	WCH_SDI_CONSOLE_STATE_ESTABLISHED,
	/* Timeout while trying to send the second buffer to the host. */
	WCH_SDI_CONSOLE_STATE_MISSING,
};

struct wch_sdi_console_data {
	enum wch_sdi_console_state state;
};

static struct wch_sdi_console_data wch_sdi_console_data_0;

#if defined(CONFIG_STDOUT_CONSOLE)
extern void __stdout_hook_install(int (*hook)(int));
#endif

#if defined(CONFIG_PRINTK)
extern void __printk_hook_install(int (*fn)(int));
#endif

static int wch_sdi_console_putc(int ch)
{
	/*
	 * `minichlink` encodes the incoming and outgoing characters into `data0`. The least
	 * significant byte is used for control and length, while the upper bytes are used for
	 * up to three characters. The `TX_FULL` bit is used to pass ownership back and forth
	 * between the host and device.
	 */
	uint32_t regs = DT_INST_REG_ADDR(0);
	struct wch_sdi_console_data *data = &wch_sdi_console_data_0;
	int timeout;

	if (data->state != WCH_SDI_CONSOLE_STATE_MISSING) {
		/* Host might be there. Spin until the buffer empties. */
		for (timeout = 0;
		     timeout != TX_TIMEOUT && (sys_read32(regs + DATA0) & TX_FULL) != 0;
		     timeout++) {
		}
	}
	if ((sys_read32(regs + DATA0) & TX_FULL) == 0) {
		/* Buffer is empty */
		sys_write32((ch << 8) | 1 | TX_FULL | TX_VALID, regs + DATA0);
		switch (data->state) {
		case WCH_SDI_CONSOLE_STATE_INITIAL:
			data->state = WCH_SDI_CONSOLE_STATE_FIRST;
			break;
		case WCH_SDI_CONSOLE_STATE_FIRST:
			data->state = WCH_SDI_CONSOLE_STATE_ESTABLISHED;
			break;
		case WCH_SDI_CONSOLE_STATE_ESTABLISHED:
			break;
		case WCH_SDI_CONSOLE_STATE_MISSING:
			/* The host has caught up. Start spinning again. */
			data->state = WCH_SDI_CONSOLE_STATE_ESTABLISHED;
			break;
		}
	} else {
		if (data->state == WCH_SDI_CONSOLE_STATE_FIRST) {
			/* First character was written but the host didn't consume it in time. */
			data->state = WCH_SDI_CONSOLE_STATE_MISSING;
		}
	}

	return 1;
}

static int wch_sdi_console_init(void)
{
#if defined(CONFIG_STDOUT_CONSOLE)
	__stdout_hook_install(wch_sdi_console_putc);
#endif
#if defined(CONFIG_PRINTK)
	__printk_hook_install(wch_sdi_console_putc);
#endif
	return 0;
}

SYS_INIT(wch_sdi_console_init, PRE_KERNEL_1, CONFIG_CONSOLE_INIT_PRIORITY);
