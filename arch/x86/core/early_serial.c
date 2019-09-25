/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <generated_dts_board.h>
#include <arch/cpu.h>
#include <misc/util.h>

/* Super-primitive 8250/16550 serial output-only driver, 115200 8n1 */

#define PORT ((io_port_t)DT_UART_NS16550_PORT_0_BASE_ADDR)

#define REG_IER 0x01	/* Interrupt enable reg.          */
#define REG_LCR 0x03	/* Line control reg.              */
#define REG_MCR 0x04	/* Modem control reg.             */
#define REG_LSR 0x05	/* Line status reg.               */
#define REG_DL_LO 0x00	/* Divisor latch low byte	  */
#define REG_DL_HI 0x01	/* Divisor latch high byte	  */

#define IER_DISABLE		0x00
#define LCR_8N1			(BIT(0) | BIT(1))
#define LCR_DLAB_SELECT		BIT(7)
#define MCR_DTR			BIT(0)
#define MCR_RTS			BIT(1)
#define LCR_THRE		BIT(5)

static void serout(int c)
{
	while (!(sys_in8(PORT + REG_LSR) & LCR_THRE)) {
	}
	sys_out8(c, PORT);
}

static int console_out(int c)
{
	if (c == '\n') {
		serout('\r');
	}
	serout(c);
	return c;
}

extern void __printk_hook_install(int (*fn)(int));

void z_x86_early_serial_init(void)
{
	/* In fact Qemu already has most of this set up and works by
	 * default
	 */
	sys_out8(IER_DISABLE, PORT + REG_IER);		/* Disable interrupts */
	sys_out8(LCR_DLAB_SELECT, PORT + REG_LCR);	/* DLAB select */
	sys_out8(1, PORT + REG_DL_LO);			/* Baud divisor = 1 */
	sys_out8(0, PORT + REG_DL_HI);
	sys_out8(LCR_8N1, PORT + REG_LCR);	/* LCR = 8n1 + DLAB off */
	sys_out8(MCR_DTR | MCR_RTS, PORT + REG_MCR);

	/* Will be replaced later when a real serial driver comes up */
	__printk_hook_install(console_out);
}
