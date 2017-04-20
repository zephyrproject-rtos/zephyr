/* util.h - Common helpers for Bluetooth drivers */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

static inline void bt_uart_drain(struct device *dev)
{
	u8_t c;

	while (uart_fifo_read(dev, &c, 1)) {
		continue;
	}
}
