/*
 * Copyright (c) 2015 Intel Corporation
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

#include <zephyr.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <board.h>
#include <uart.h>

#define BUF_MAXSIZE 256

struct device *uart1_dev;

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

static void uart1_isr(struct device *x)
{
	int len = uart_fifo_read(uart1_dev, buf, BUF_MAXSIZE);
	ARG_UNUSED(x);
	msg_dump(__func__, buf, len);
}

static void uart1_init(void)
{
	uart1_dev = device_get_binding("UART_1");

	uart_irq_callback_set(uart1_dev, uart1_isr);

	uart_irq_rx_enable(uart1_dev);

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

	uart_fifo_fill(uart1_dev, pdu, sizeof(*len) + sizeof(nci_reset));

	while (1) {
		nano_task_timer_start(&t, MSEC(500));
		nano_task_timer_test(&t, TICKS_UNLIMITED);
	}
}
