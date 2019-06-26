/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <stdio.h>
#include <zephyr/types.h>
#include <string.h>
#include <drivers/uart.h>
#include <sys/byteorder.h>

#define BUF_MAXSIZE	256
#define SLEEP_TIME	500

static struct device *uart1_dev;
static u8_t rx_buf[BUF_MAXSIZE];
static u8_t tx_buf[BUF_MAXSIZE];
static u8_t nci_reset[] = {0x20, 0x00, 0x01, 0x00};

static void msg_dump(const char *s, u8_t *data, unsigned len)
{
	unsigned i;

	printf("%s: ", s);
	for (i = 0U; i < len; i++) {
		printf("%02x ", data[i]);
	}
	printf("(%u bytes)\n", len);
}

static void uart1_isr(struct device *x)
{
	int len = uart_fifo_read(uart1_dev, rx_buf, BUF_MAXSIZE);

	ARG_UNUSED(x);
	msg_dump(__func__, rx_buf, len);
}

static void uart1_init(void)
{
	uart1_dev = device_get_binding("UART_1");

	uart_irq_callback_set(uart1_dev, uart1_isr);
	uart_irq_rx_enable(uart1_dev);

	printf("%s() done\n", __func__);
}

void main(void)
{
	u32_t *size = (u32_t *)tx_buf;

	printf("Sample app running on: %s\n", CONFIG_ARCH);

	uart1_init();

	/* 4 bytes for the payload's length */
	UNALIGNED_PUT(sys_cpu_to_be32(sizeof(nci_reset)), size);

	/* NFC Controller Interface reset cmd */
	memcpy(tx_buf + sizeof(u32_t), nci_reset, sizeof(nci_reset));

	/*
	 * Peer will receive: 0x00 0x00 0x00 0x04 0x20 0x00 0x01 0x00
	 *	                nci_reset size   +    nci_reset cmd
	 */
	uart_fifo_fill(uart1_dev, tx_buf, sizeof(u32_t) + sizeof(nci_reset));

	while (1) {
		k_sleep(SLEEP_TIME);
	}
}
