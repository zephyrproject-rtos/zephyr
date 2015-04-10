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

/* generic UART info structure */
struct uart_init_info {
	int baud_rate;
	uint32_t regs; /* base port number or MM base address */
	uint32_t sys_clk_freq; /* in Hz */
	uint8_t options; /* HW Flow Control option */
	uint8_t irq; /* interrupt request number */
	uint8_t int_pri; /* interrupt priority level */
};
/* UART driver has to configure the device to 8n1 */

void uart_init(int port, const struct uart_init_info *const pinfo);

/* console I/O functions */
int uart_poll_in(int port, unsigned char *pChar);
unsigned char uart_poll_out(int which, unsigned char outChar);

/* interrupt driven I/O functions */
int uart_fifo_fill(int port, const uint8_t *txData, int len);
int uart_fifo_read(int port, uint8_t *rxData, const int size);
void uart_irq_tx_enable(int port);
void uart_irq_tx_disable(int port);
int uart_irq_tx_ready(int port);
void uart_irq_rx_enable(int port);
void uart_irq_rx_disable(int port);
int uart_irq_rx_ready(int port);
void uart_irq_err_enable(int port);
void uart_irq_err_disable(int port);
int uart_irq_is_pending(int port);
int uart_irq_update(int port);
void uart_int_connect(int port, void (*isr)(void *), void *arg, void *stub);

#ifdef __cplusplus
}
#endif

#endif /* __INCuarth */
