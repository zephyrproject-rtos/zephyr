/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_st67w611m1

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(st67w611m1_spi, CONFIG_WIFI_LOG_LEVEL);

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/types.h>
#include <zephyr/toolchain.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/net_buf.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys/byteorder.h>

#include "st67w611m1_spi.h"

struct st67_config {
	const struct gpio_dt_spec chip_select;
	const struct gpio_dt_spec ready;
	const struct gpio_dt_spec chip_en;
	const struct spi_dt_spec spi;
};

#define ST67_SPI_NODE DT_DRV_INST(0)

static const struct st67_config st67_config = {
	.chip_select = GPIO_DT_SPEC_GET(ST67_SPI_NODE, chip_select_gpios),
	.ready = GPIO_DT_SPEC_GET(ST67_SPI_NODE, ready_gpios),
	.chip_en = GPIO_DT_SPEC_GET(ST67_SPI_NODE, chip_en_gpios),
	.spi = SPI_DT_SPEC_GET(ST67_SPI_NODE, SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_LOCK_ON),
};

#define ST67_SPI_SYNC_WORD 0x55AA

struct st67_spi_context {
	bool initialized;
	bool ready_pin_previous_state;

	struct k_event event;
	struct k_fifo tx_fifo;

	drv_rx_iface_cb_t drv_rx_iface_cb[FRAME_TYPE_MAX];
};

static struct st67_spi_context spi_context = {
	.initialized = false,
	.ready_pin_previous_state = false,
};

K_KERNEL_STACK_DEFINE(st67_spi_processor_stack, CONFIG_ST67W611M1_SPI_PROCESSOR_THREAD_STACK_SIZE);

static struct k_thread st67_spi_processor_thread;

K_MEM_SLAB_DEFINE_TYPE(st67_spi_pkt_mem_slab, struct st67_spi_pkt,
		       CONFIG_ST67W611M1_SPI_PKT_MEM_SLAB_COUNT);

enum {
	/* Ready pin rising edge. ST67 has something to send AND/OR is ready to receive. */
	SPI_EVT_ST67_READY = BIT(1),
	/* Ready pin falling edge. ST67 acks what it received. */
	SPI_EVT_ST67_ACK = BIT(2),
	SPI_EVT_TX_PENDING = BIT(3),
};

#define ST67_SPI_HEADER_VERSION_SHIFT 0
#define ST67_SPI_HEADER_VERSION_MASK  BIT_MASK(2)
#define ST67_SPI_HEADER_VERSION_GET(x)                                                             \
	(((x) >> ST67_SPI_HEADER_VERSION_SHIFT) & ST67_SPI_HEADER_VERSION_MASK)

#define ST67_SPI_HEADER_RX_STALL_SHIFT 2
#define ST67_SPI_HEADER_RX_STALL_MASK  BIT_MASK(1)
#define ST67_SPI_HEADER_RX_STALL_GET(x)                                                            \
	(((x) >> ST67_SPI_HEADER_RX_STALL_SHIFT) & ST67_SPI_HEADER_RX_STALL_MASK)

#define ST67_SPI_HEADER_FLAGS_SHIFT 3
#define ST67_SPI_HEADER_FLAGS_MASK  BIT_MASK(5)
#define ST67_SPI_HEADER_FLAGS_GET(x)                                                               \
	(((x) >> ST67_SPI_HEADER_FLAGS_SHIFT) & ST67_SPI_HEADER_FLAGS_MASK)

static uint8_t spi_buffer[ST67_SPI_BUFFER_SIZE_BYTES] __nocache;

int st67_spi_send(struct st67_spi_pkt *pkt)
{
	if (!spi_context.initialized) {
		LOG_ERR("SPI not initialized");
		return -EINVAL;
	}

	if ((pkt == NULL) || (pkt->net_pkt == NULL)) {
		LOG_ERR("Got empty pkt");
		return -EINVAL;
	}
	k_fifo_put(&spi_context.tx_fifo, pkt);

	k_event_post(&spi_context.event, SPI_EVT_TX_PENDING);
	return 0;
}

static void write_spi_header_to_buffer(size_t len, uint8_t type)
{
	sys_put_le16(ST67_SPI_SYNC_WORD, &spi_buffer[0]);
	sys_put_le16((uint16_t)len, &spi_buffer[2]);
	spi_buffer[4] = 0; /* frame */
	spi_buffer[5] = type;
	sys_put_le16(0, &spi_buffer[6]); /* reserved */
}

struct rx_header {
	uint16_t data_length;
	uint8_t type;
	bool rx_stall;
};

static int parse_spi_header(struct rx_header *out)
{
	if (!out) {
		return -EINVAL;
	}

	uint16_t sync_word = sys_get_le16(&spi_buffer[0]);

	if (sync_word != ST67_SPI_SYNC_WORD) {
		return -EBADMSG;
	}

	out->data_length = sys_get_le16(&spi_buffer[2]);
	out->rx_stall = ST67_SPI_HEADER_RX_STALL_GET(spi_buffer[4]);
	out->type = spi_buffer[5];

	return 0;
}

static int st67_spi_txrx(size_t xfer_len, size_t offset)
{
	struct spi_buf tx_buf = {
		.buf = &spi_buffer[offset],
		.len = xfer_len,
	};
	struct spi_buf rx_buf = {
		.buf = &spi_buffer[offset],
		.len = xfer_len,
	};
	struct spi_buf_set tx_buf_set = {
		.buffers = &tx_buf,
		.count = 1,
	};
	struct spi_buf_set rx_buf_set = {
		.buffers = &rx_buf,
		.count = 1,
	};

	return spi_transceive_dt(&st67_config.spi, &tx_buf_set, &rx_buf_set);
}

#define ST67_SPI_BUF_ALIGN_MASK 0x3

static int st67_spi_process(void)
{
	int ret = 0;
	size_t pkt_len = 0;
	size_t first_xfer_len = 0;
	struct rx_header rx_header = {0};
	bool rx_header_valid = false;

	/* Will be removed from FIFO if rx_stall was not set in the RX header. */
	struct st67_spi_pkt *spi_pkt = k_fifo_peek_head(&spi_context.tx_fifo);

	if (spi_pkt == NULL) { /* Nothing to say */
		write_spi_header_to_buffer(0, 0);
	} else {
		pkt_len = net_pkt_get_len(spi_pkt->net_pkt);
		if (pkt_len + sizeof(struct st67_spi_header) > sizeof(spi_buffer)) {
			ret = -EMSGSIZE;
			goto out;
		}
		write_spi_header_to_buffer(pkt_len, spi_pkt->type);
		/* Drop invalid pkt. */
		if ((spi_pkt->net_pkt == NULL) || net_pkt_is_empty(spi_pkt->net_pkt)) {
			ret = -EINVAL;
			k_fifo_get(&spi_context.tx_fifo, K_NO_WAIT);
			if (spi_pkt->net_pkt != NULL) {
				net_pkt_unref(spi_pkt->net_pkt);
			}
			k_mem_slab_free(&st67_spi_pkt_mem_slab, spi_pkt);
			spi_pkt = NULL;
			goto out;
		}
		size_t copied =
			net_buf_linearize(spi_buffer + sizeof(struct st67_spi_header),
					  sizeof(spi_buffer), spi_pkt->net_pkt->buffer, 0, pkt_len);
		if (copied != pkt_len) {
			ret = -EIO;
			goto out;
		}
	}

	/* Add mod4 payload padding to transferred length. */
	first_xfer_len = (pkt_len + sizeof(struct st67_spi_header) + ST67_SPI_BUF_ALIGN_MASK) &
			 ~ST67_SPI_BUF_ALIGN_MASK;

	/* Actual SPI transfer */
	ret = st67_spi_txrx(first_xfer_len, 0);
	if (ret < 0) {
		goto out;
	}

	ret = parse_spi_header(&rx_header);
	if (ret < 0) {
		goto out;
	}
	rx_header_valid = true;

	/* 2nd transaction can be necessary if RX len > TX len. */
	size_t second_xfer_len = 0;
	size_t rx_data_length = (size_t)rx_header.data_length;
	enum st67_spi_frame_type rx_type = rx_header.type;
	size_t first_payload_len = first_xfer_len - sizeof(struct st67_spi_header);

	if (rx_data_length > first_payload_len) {
		second_xfer_len = (rx_data_length - first_payload_len + ST67_SPI_BUF_ALIGN_MASK) &
				  ~ST67_SPI_BUF_ALIGN_MASK;
		if (first_xfer_len + second_xfer_len > sizeof(spi_buffer)) {
			ret = -EMSGSIZE;
			goto out;
		}
		ret = st67_spi_txrx(second_xfer_len, first_xfer_len);
		if (ret < 0) {
			goto out;
		}
	}

	if (rx_data_length > 0) {
		if (rx_type >= FRAME_TYPE_MAX) {
			LOG_DBG("Unexpected frame type %d", rx_type);
			goto out;
		}
		if (!spi_context.drv_rx_iface_cb[rx_type]) {
			LOG_DBG("NULL cb");
			goto out;
		}

		spi_context.drv_rx_iface_cb[rx_type](spi_buffer + sizeof(struct st67_spi_header),
						     rx_data_length);
	}

out:
	spi_release_dt(&st67_config.spi);
	if ((spi_pkt != NULL) && rx_header_valid) {
		/* If rx_stall is 1, the pkt stays in the FIFO to be sent once again. */
		if (!rx_header.rx_stall) {
			k_fifo_get(&spi_context.tx_fifo, K_NO_WAIT);
			net_pkt_unref(spi_pkt->net_pkt);
			k_mem_slab_free(&st67_spi_pkt_mem_slab, spi_pkt);
		} else {
			LOG_ERR("Received rx_stall. Sending pkt once again.");
		}
	}
	return ret;
}

/* Processing SPI on its own thread. */
static void st67_spi_processor(void *p1, void *p2, void *p3)
{
	int ret;
	uint32_t received_event;

	while (true) {
		received_event = k_event_wait_safe(&spi_context.event,
						   SPI_EVT_ST67_READY | SPI_EVT_TX_PENDING, false,
						   K_FOREVER);

		ret = gpio_pin_set_dt(&st67_config.chip_select, 1);
		if (ret < 0) {
			LOG_ERR("Cannot set CS");
		}

		/* If ST67 does not have anything to transfer */
		if (!(received_event & SPI_EVT_ST67_READY)) {
			k_event_wait_safe(&spi_context.event, SPI_EVT_ST67_READY, false, K_FOREVER);
		}

		ret = st67_spi_process();
		if (ret < 0) {
			LOG_ERR("Failed to process SPI: %d", ret);
		}

		k_event_wait_safe(&spi_context.event, SPI_EVT_ST67_ACK, false, K_FOREVER);

		ret = gpio_pin_set_dt(&st67_config.chip_select, 0);
		if (ret < 0) {
			LOG_ERR("Cannot unset CS");
		}

		/* Process SPI TX once again if TX pkt has been pushed to FIFO during SPI transceive
		 * or if RX stall has been received.
		 */
		if (!k_fifo_is_empty(&spi_context.tx_fifo)) {
			k_event_post(&spi_context.event, SPI_EVT_TX_PENDING);
		}
	}
}

static void spi_ready_cb(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins)
{
	LOG_DBG("Got ready");
	if (!spi_context.initialized) {
		LOG_DBG("ready dropped because st67 not initialized");
		return;
	}
	bool pin_current_state = gpio_pin_get_dt(&st67_config.ready);

	if (pin_current_state != spi_context.ready_pin_previous_state) {
		if (pin_current_state) {
			k_event_post(&spi_context.event, SPI_EVT_ST67_READY);
		} else {
			k_event_post(&spi_context.event, SPI_EVT_ST67_ACK);
		}
	} else {
		LOG_DBG("reached current_pin == previous");
	}
	spi_context.ready_pin_previous_state = pin_current_state;
}

int st67_spi_init(void)
{
	int ret;

	if (!gpio_is_ready_dt(&st67_config.chip_select)) {
		LOG_ERR_DEVICE_NOT_READY(st67_config.chip_select.port);
		return -ENODEV;
	}
	if (!gpio_is_ready_dt(&st67_config.ready)) {
		LOG_ERR_DEVICE_NOT_READY(st67_config.ready.port);
		return -ENODEV;
	}
	if (!gpio_is_ready_dt(&st67_config.chip_en)) {
		LOG_ERR_DEVICE_NOT_READY(st67_config.chip_en.port);
		return -ENODEV;
	}
	if (!spi_is_ready_dt(&st67_config.spi)) {
		LOG_ERR_DEVICE_NOT_READY(st67_config.spi.bus);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&st67_config.ready, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure the ready pin: %d", ret);
		return ret;
	}
	ret = gpio_pin_interrupt_configure_dt(&st67_config.ready, GPIO_INT_EDGE_BOTH);
	if (ret < 0) {
		LOG_ERR("Could not configure the interrupt for the ready pin: %d", ret);
		return ret;
	}
	static struct gpio_callback cb;

	gpio_init_callback(&cb, spi_ready_cb, BIT(st67_config.ready.pin));
	ret = gpio_add_callback_dt(&st67_config.ready, &cb);
	if (ret < 0) {
		LOG_ERR("Could not add the interrupt to the ready pin: %d", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&st67_config.chip_select, GPIO_OUTPUT_LOW);
	if (ret < 0) {
		LOG_ERR("Could not configure the CS pin: %d", ret);
		return ret;
	}
	ret = gpio_pin_configure_dt(&st67_config.chip_en, GPIO_OUTPUT_LOW);
	if (ret < 0) {
		LOG_ERR("Could not configure the CHIP_EN pin: %d", ret);
		return ret;
	}

	/* event, fifos, sem etc. */
	k_event_init(&spi_context.event);
	k_fifo_init(&spi_context.tx_fifo);

	/* SPI processing thread */
	k_thread_create(&st67_spi_processor_thread, st67_spi_processor_stack,
			K_THREAD_STACK_SIZEOF(st67_spi_processor_stack), st67_spi_processor, NULL,
			NULL, NULL, K_PRIO_COOP(CONFIG_ST67W611M1_SPI_PROCESSOR_THREAD_PRIORITY), 0,
			K_NO_WAIT);
	k_thread_name_set(&st67_spi_processor_thread, "st67_spi_process_thread");

	/* Give some time to ST67 before power on. */
	k_sleep(K_MSEC(1));

	/* Start ST67W611M1. */
	ret = gpio_pin_set_dt(&st67_config.chip_en, 1);
	if (ret < 0) {
		return ret;
	}

	spi_context.initialized = true;

	return 0;
}

void st67_spi_register_drv_rx_iface_cb(drv_rx_iface_cb_t cb, enum st67_spi_frame_type type)
{
	if (type >= FRAME_TYPE_MAX) {
		LOG_DBG("Unexpected frame type %d", type);
		return;
	}

	spi_context.drv_rx_iface_cb[type] = cb;
}
