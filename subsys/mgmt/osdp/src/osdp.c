/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/logging/log.h>

#include "osdp_common.h"

LOG_MODULE_REGISTER(osdp, CONFIG_OSDP_LOG_LEVEL);

#ifdef CONFIG_OSDP_SC_ENABLED
#ifdef CONFIG_OSDP_MODE_PD
#define OSDP_KEY_STRING CONFIG_OSDP_PD_SCBK
#else
#define OSDP_KEY_STRING CONFIG_OSDP_MASTER_KEY
#endif
#else
#define OSDP_KEY_STRING ""
#endif	/* CONFIG_OSDP_SC_ENABLED */

struct osdp_device {
	struct ring_buf rx_buf;
	struct ring_buf tx_buf;
#ifdef CONFIG_OSDP_MODE_PD
	int rx_event_data;
	struct k_fifo rx_event_fifo;
#endif
	uint8_t rx_fbuf[CONFIG_OSDP_UART_BUFFER_LENGTH];
	uint8_t tx_fbuf[CONFIG_OSDP_UART_BUFFER_LENGTH];
	struct uart_config dev_config;
	const struct device *dev;
	int wait_for_mark;
	uint8_t last_byte;
};

static struct osdp osdp_ctx;
static struct osdp_pd osdp_pd_ctx[CONFIG_OSDP_NUM_CONNECTED_PD];
static struct osdp_device osdp_device;
static struct k_thread osdp_refresh_thread;
static K_THREAD_STACK_DEFINE(osdp_thread_stack, CONFIG_OSDP_THREAD_STACK_SIZE);

static void osdp_handle_in_byte(struct osdp_device *p, uint8_t *buf, int len)
{
	if (p->wait_for_mark) {
		/* Check for new packet beginning with [FF,53,...] sequence */
		if (p->last_byte == 0xFF && buf[0] == 0x53) {
			buf[0] = 0xFF;
			ring_buf_put(&p->rx_buf, buf, 1);   /* put last byte */
			buf[0] = 0x53;
			ring_buf_put(&p->rx_buf, buf, len); /* put rest */
			p->wait_for_mark = 0; /* Mark found. Clear flag */
		}
		p->last_byte = buf[0];
		return;
	}
	ring_buf_put(&p->rx_buf, buf, len);
}

static void osdp_uart_isr(const struct device *dev, void *user_data)
{
	size_t len;
	uint8_t buf[64];
	struct osdp_device *p = user_data;

	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {

		if (uart_irq_rx_ready(dev)) {
			len = uart_fifo_read(dev, buf, sizeof(buf));
			if (len > 0) {
				osdp_handle_in_byte(p, buf, len);
			}
		}

		if (uart_irq_tx_ready(dev)) {
			len = ring_buf_get(&p->tx_buf, buf, 1);
			if (!len) {
				uart_irq_tx_disable(dev);
			} else {
				uart_fifo_fill(dev, buf, 1);
			}
		}
	}
#ifdef CONFIG_OSDP_MODE_PD
	if (p->wait_for_mark == 0) {
		/* wake osdp_refresh thread */
		k_fifo_put(&p->rx_event_fifo, &p->rx_event_data);
	}
#endif
}

static int osdp_uart_receive(void *data, uint8_t *buf, int len)
{
	struct osdp_device *p = data;

	return (int)ring_buf_get(&p->rx_buf, buf, len);
}

static int osdp_uart_send(void *data, uint8_t *buf, int len)
{
	int sent = 0;
	struct osdp_device *p = data;

	sent = (int)ring_buf_put(&p->tx_buf, buf, len);
	uart_irq_tx_enable(p->dev);
	return sent;
}

static void osdp_uart_flush(void *data)
{
	struct osdp_device *p = data;

	p->wait_for_mark = 1;
	ring_buf_reset(&p->tx_buf);
	ring_buf_reset(&p->rx_buf);
}

struct osdp *osdp_get_ctx()
{
	return &osdp_ctx;
}

static struct osdp *osdp_build_ctx(struct osdp_channel *channel)
{
	int i;
	struct osdp *ctx;
	struct osdp_pd *pd;
	int pd_adddres[CONFIG_OSDP_NUM_CONNECTED_PD] = {0};

#ifdef CONFIG_OSDP_MODE_PD
	pd_adddres[0] = CONFIG_OSDP_PD_ADDRESS;
#else
	if (osdp_extract_address(pd_adddres)) {
		return NULL;
	}
#endif
	ctx = &osdp_ctx;
	ctx->num_pd = CONFIG_OSDP_NUM_CONNECTED_PD;
	ctx->pd = &osdp_pd_ctx[0];
	SET_CURRENT_PD(ctx, 0);

	for (i = 0; i < CONFIG_OSDP_NUM_CONNECTED_PD; i++) {
		pd = osdp_to_pd(ctx, i);
		pd->idx = i;
		pd->seq_number = -1;
		pd->osdp_ctx = ctx;
		pd->address = pd_adddres[i];
		pd->baud_rate = CONFIG_OSDP_UART_BAUD_RATE;
		if (IS_ENABLED(CONFIG_OSDP_SKIP_MARK_BYTE)) {
			SET_FLAG(pd, PD_FLAG_PKT_SKIP_MARK);
		}
		memcpy(&pd->channel, channel, sizeof(struct osdp_channel));
		k_mem_slab_init(&pd->cmd.slab,
				pd->cmd.slab_buf, sizeof(struct osdp_cmd),
				CONFIG_OSDP_PD_COMMAND_QUEUE_SIZE);
	}
	return ctx;
}

void osdp_refresh(void *arg1, void *arg2, void *arg3)
{
	struct osdp *ctx = osdp_get_ctx();

	while (1) {
#ifdef CONFIG_OSDP_MODE_PD
		k_fifo_get(&osdp_device.rx_event_fifo, K_FOREVER);
#else
		k_msleep(50);
#endif
		osdp_update(ctx);
	}
}

static int osdp_init(const struct device *arg)
{
	ARG_UNUSED(arg);
	int len;
	uint8_t c, *key = NULL, key_buf[16];
	struct osdp *ctx;
	struct osdp_device *p = &osdp_device;
	struct osdp_channel channel = {
		.send = osdp_uart_send,
		.recv = osdp_uart_receive,
		.flush = osdp_uart_flush,
		.data = &osdp_device,
	};

#ifdef CONFIG_OSDP_MODE_PD
	k_fifo_init(&p->rx_event_fifo);
#endif

	ring_buf_init(&p->rx_buf, sizeof(p->rx_fbuf), p->rx_fbuf);
	ring_buf_init(&p->tx_buf, sizeof(p->tx_fbuf), p->tx_fbuf);

	/* init OSDP uart device */
	p->dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_osdp_uart));
	if (!device_is_ready(p->dev)) {
		LOG_ERR("UART dev is not ready");
		k_panic();
	}

	/* configure uart device to 8N1 */
	p->dev_config.baudrate = CONFIG_OSDP_UART_BAUD_RATE;
	p->dev_config.data_bits = UART_CFG_DATA_BITS_8;
	p->dev_config.parity = UART_CFG_PARITY_NONE;
	p->dev_config.stop_bits = UART_CFG_STOP_BITS_1;
	p->dev_config.flow_ctrl = UART_CFG_FLOW_CTRL_NONE;
	uart_configure(p->dev, &p->dev_config);

	uart_irq_rx_disable(p->dev);
	uart_irq_tx_disable(p->dev);
	uart_irq_callback_user_data_set(p->dev, osdp_uart_isr, p);

	/* Drain UART fifo and set channel to wait for mark byte */
	while (uart_irq_rx_ready(p->dev)) {
		uart_fifo_read(p->dev, &c, 1);
	}
	p->wait_for_mark = 1;

	/* Both TX and RX are interrupt driven */
	uart_irq_rx_enable(p->dev);

	/* setup OSDP */
	ctx = osdp_build_ctx(&channel);
	if (ctx == NULL) {
		LOG_ERR("OSDP build ctx failed!");
		k_panic();
	}

	if (IS_ENABLED(CONFIG_OSDP_SC_ENABLED)) {
		if (strcmp(OSDP_KEY_STRING, "NONE") != 0) {
			len = strlen(OSDP_KEY_STRING);
			if (len != 32) {
				LOG_ERR("Key string length must be 32");
				k_panic();
			}
			len = hex2bin(OSDP_KEY_STRING, 32, key_buf, 16);
			if (len != 16) {
				LOG_ERR("Failed to parse key buffer");
				k_panic();
			}
			key = key_buf;
		}
	}

	if (osdp_setup(ctx, key)) {
		LOG_ERR("Failed to setup OSDP device!");
		k_panic();
	}

	LOG_INF("OSDP init okay!");

	/* kick off refresh thread */
	k_thread_create(&osdp_refresh_thread, osdp_thread_stack,
			CONFIG_OSDP_THREAD_STACK_SIZE, osdp_refresh,
			NULL, NULL, NULL, K_PRIO_COOP(2), 0, K_NO_WAIT);
	return 0;
}

SYS_INIT(osdp_init, POST_KERNEL, 10);
