/*
 * Copyright (c) 2017 RDA
 * Copyright (c) 2016-2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _RDA5981A_UART_H_
#define _RDA5981A_UART_H_

typedef enum {
    PN = 0,
    PO = 1,
    PE = 2,
    PF1 = 3,
    PF0 = 4
} serial_parity;

/* device config */
struct uart_rda5981a_config {
	uint32_t ahb_bus_clk;
	struct uart_device_config uconf;
};

/* driver data */
struct uart_rda5981a_data {
	/* which uart port to use */
	uint32_t uart_index;
	/* current baud rate */
	uint32_t baud_rate;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_t user_cb;
#endif
};

#endif	/* _RDA5981A_UART_H_ */
