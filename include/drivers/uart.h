/* uart.h - public UART driver APIs */

/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __INCuarth
#define __INCuarth

#ifdef __cplusplus
extern "C" {
#endif

#include <device.h>

/* UART device configuration */
struct uart_device_config_t {
	/**
	 * Base port number
	 * or memory mapped base address
	 * or register address
	 */
	union {
		uint32_t port;
		uint8_t *base;
		uint32_t regs;
	};
	uint8_t irq;		/**< interrupt request level */
	uint8_t int_pri;	/**< interrupt priority */

	/**< Configuration function */
	int (*config_func)(struct device *dev);
};

/** UART configuration structure */
struct uart_init_info {
	int baud_rate;		/* Baud rate */
	uint32_t sys_clk_freq;	/* System clock frequency in Hz */

	uint8_t int_pri;	/* Interrupt priority level */
	uint8_t options;	/* HW Flow Control option */

	uint32_t regs;		/* Register address */
};

int uart_platform_init(struct device *dev);

/* UART driver has to configure the device to 8n1 */
void uart_init(struct device *dev,
	       const struct uart_init_info * const pinfo);

/* console I/O functions */
int uart_poll_in(struct device *dev, unsigned char *p_char);
unsigned char uart_poll_out(struct device *dev, unsigned char out_char);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

/* interrupt driven I/O functions */
int uart_fifo_fill(struct device *dev, const uint8_t *txData, int len);
int uart_fifo_read(struct device *dev, uint8_t *rxData, const int size);
void uart_irq_tx_enable(struct device *dev);
void uart_irq_tx_disable(struct device *dev);
int uart_irq_tx_ready(struct device *dev);
void uart_irq_rx_enable(struct device *dev);
void uart_irq_rx_disable(struct device *dev);
int uart_irq_rx_ready(struct device *dev);
void uart_irq_err_enable(struct device *dev);
void uart_irq_err_disable(struct device *dev);
int uart_irq_is_pending(struct device *dev);
int uart_irq_update(struct device *dev);
unsigned int uart_irq_get(struct device *dev);
void uart_generic_info_init(struct uart_init_info *p_info);

#endif

#ifdef __cplusplus
}
#endif

#endif /* __INCuarth */
