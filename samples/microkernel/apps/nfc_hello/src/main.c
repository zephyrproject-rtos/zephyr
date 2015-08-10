/*
 * Copyright (c) 2015 Intel Corporation
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
 * 3) Neither the name of Intel Corporation nor the names of its contributors
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

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <nanokernel.h>
#include <arch/cpu.h>
#include <board.h>
#include <drivers/uart.h>

#define UART1 (uart_devs[1])
#define UART1_IRQ COM2_INT_LVL
#define UART1_INT_PRI COM2_INT_PRI
#define BUF_MAXSIZE 256
#define MSEC(_ms) ((_ms) * sys_clock_ticks_per_sec / 1000)

#define D(fmt, args...)					\
do {							\
	printf("%s() " fmt "\n", __func__, ## args);	\
} while (0)


static uint8_t buf[BUF_MAXSIZE];
static uint8_t nci_reset[] = { 0x20, 0x00, 0x01, 0x00 };


static inline const uint32_t htonl(uint32_t x)
{
	__asm__ __volatile__ ("bswap %0" : "=r" (x) : "0" (x));
	return x;
}

static void msg_dump(const char *s, uint8_t *data, unsigned len)
{
	unsigned i;
	printf("%s: ", s);
	for (i = 0; i < len; i++)
		printf("%02x ", data[i]);
	printf("(%u bytes)\n", len);
}

static void uart1_isr(void *x)
{
	int len = uart_fifo_read(UART1, buf, BUF_MAXSIZE);
	ARG_UNUSED(x);
	msg_dump(__func__, buf, len);
}

static void uart1_init(void)
{
	struct uart_init_info uart1 = {
		.baud_rate = CONFIG_UART_BAUDRATE,
		.sys_clk_freq = UART_XTAL_FREQ,
		.int_pri = UART1_INT_PRI,
	};

	uart_init(UART1, &uart1);

	irq_connect(UART1_IRQ, uart1.int_pri, uart1_isr, 0);

	irq_enable(UART1_IRQ);

	uart_irq_rx_enable(UART1);

	D("done");
}

void main(void)
{
	struct nano_timer t;
	void *t_data;
	uint8_t *pdu = buf;
	uint32_t *len = (void *) pdu;

	nano_timer_init(&t, &t_data);

	uart1_init();

	*len = htonl(sizeof(nci_reset));

	memcpy(pdu + sizeof(*len), nci_reset, sizeof(nci_reset));

	uart_fifo_fill(UART1, pdu, sizeof(*len) + sizeof(nci_reset));

	while (1) {
		nano_task_timer_start(&t, MSEC(500));
                nano_task_timer_wait(&t);
	}
}
