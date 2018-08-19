/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "x86_64-hw.h"

/* Super-primitive 8250 serial output-only driver, 115200 8n1 */

#define _PORT 0x3f8

static inline void _serout(int c)
{
	while (!(ioport_in8(_PORT + 5) & 0x20)) {
	}
	ioport_out8(_PORT, c);
}

static inline void serial_putc(int c)
{
	if (c == '\n') {
		_serout('\r');
	}
	_serout(c);
}

static inline void serial_puts(const char *s)
{
	while (*s) {
		serial_putc(*s++);
	}
}

static inline void serial_init(void)
{
	/* In fact Qemu already has most of this set up and works by
	 * default
	 */
	ioport_out8(_PORT+1, 0);    /* IER = 0 */
	ioport_out8(_PORT+3, 0x80); /* LCR = 8n1 + DLAB select */
	ioport_out8(_PORT,   1);    /* Divisor Latch low byte */
	ioport_out8(_PORT+1, 0);    /* Divisor Latch high byte */
	ioport_out8(_PORT+3, 0x03); /* LCR = 8n1 + DLAB off */
	ioport_out8(_PORT+4, 0x03); /* MCR = DTR & RTS asserted */
}
