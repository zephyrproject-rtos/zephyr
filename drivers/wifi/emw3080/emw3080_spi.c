/*
 * Copyright (c) 2026 Witekio
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * EMW3080 SPI transport.
 *
 * One transaction: NSS low, wait FLOW rising edge, exchange 8-byte headers,
 * wait FLOW rising edge, exchange max(controller len, peripheral len) bytes,
 * NSS high. Both directions are full duplex: a frame can be sent and another
 * received in the same transaction.
 */

#include "emw3080_spi.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(emw3080_spi, CONFIG_WIFI_LOG_LEVEL);

#define EMW3080_SPI_ERR_BACKOFF_MS 10

#define EMW3080_SPI_HDR_CONTROLLER 0x0A
#define EMW3080_SPI_HDR_PERIPHERAL 0x0B

struct __packed emw3080_spi_hdr {
	uint8_t type;
	uint16_t len;
	uint16_t lenx;
	uint8_t pad[3];
};

struct emw3080_spi_tx {
	const uint8_t *data;
	uint16_t len;
};

struct emw3080_spi_rx {
	uint8_t *data;
	uint16_t size;
	uint16_t len;
};

struct emw3080_spi_config {
	struct gpio_dt_spec resetn;
	struct gpio_dt_spec nss;
	struct gpio_dt_spec notify;
	struct gpio_dt_spec flow;
	struct spi_dt_spec spi;
};

#define EMW3080_SPI_NODE DT_INST(0, mxchip_emw3080)

static const struct emw3080_spi_config config = {
	.resetn = GPIO_DT_SPEC_GET(EMW3080_SPI_NODE, resetn_gpios),
	.nss = GPIO_DT_SPEC_GET(EMW3080_SPI_NODE, nss_gpios),
	.notify = GPIO_DT_SPEC_GET(EMW3080_SPI_NODE, notify_gpios),
	.flow = GPIO_DT_SPEC_GET(EMW3080_SPI_NODE, flow_gpios),
	.spi = SPI_DT_SPEC_GET(EMW3080_SPI_NODE,
			       SPI_OP_MODE_CONTROLLER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB),
};

static struct gpio_callback flow_cb;
static struct gpio_callback notify_cb;

static K_SEM_DEFINE(flow_sem, 0, 1);
static K_SEM_DEFINE(wake_sem, 0, 1);
static K_SEM_DEFINE(tx_done, 0, 1);

/* Serializes spi_send() callers. */
static K_MUTEX_DEFINE(send_lock);
/* Held while the bus is in use; protects tx_frame, tx_pending and tx_result. */
static K_MUTEX_DEFINE(xfer_lock);

static struct emw3080_spi_tx tx_frame;
static bool tx_pending;
static int tx_result;

static uint8_t rx_buf[EMW3080_SPI_MAX_FRAME_LEN];
static struct emw3080_spi_rx spi_rx = {.data = rx_buf, .size = sizeof(rx_buf)};

static K_THREAD_STACK_DEFINE(transport_stack, CONFIG_WIFI_EMW3080_SPI_THREAD_STACK_SIZE);
static struct k_thread transport_thread_data;

static emw3080_spi_rx_cb_t rx_cb;
static void *rx_cb_user;

static struct emw3080_spi_hdr hdr_make_controller(uint16_t len)
{
	return (struct emw3080_spi_hdr){
		.type = EMW3080_SPI_HDR_CONTROLLER,
		.len = sys_cpu_to_le16(len),
		.lenx = sys_cpu_to_le16((uint16_t)~len),
	};
}

static int hdr_parse_peripheral(const struct emw3080_spi_hdr *hdr, uint16_t *len)
{
	uint16_t l = sys_le16_to_cpu(hdr->len);
	uint16_t lx = sys_le16_to_cpu(hdr->lenx);

	if (hdr->type != EMW3080_SPI_HDR_PERIPHERAL || (uint16_t)(l ^ lx) != 0xFFFFU ||
	    l > EMW3080_SPI_MAX_FRAME_LEN) {
		LOG_HEXDUMP_ERR(hdr, sizeof(*hdr), "bad peripheral header");
		return -EPROTO;
	}

	*len = l;
	return 0;
}

static void flow_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);
	k_sem_give(&flow_sem);
}

static void notify_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);
	k_sem_give(&wake_sem);
}

/* FLOW must rise for every phase: a level already high is not a new handshake. */
static int wait_flow_rise(void)
{
	if (k_sem_take(&flow_sem, K_MSEC(CONFIG_WIFI_EMW3080_FLOW_TIMEOUT_MS)) != 0) {
		return -EIO;
	}
	return (gpio_pin_get_dt(&config.flow) == 1) ? 0 : -EIO;
}

static bool notify_pending(void)
{
	return gpio_pin_get_dt(&config.notify) > 0;
}

static int xfer(const uint8_t *tx, uint16_t tx_len, uint8_t *rx, uint16_t rx_len)
{
	uint16_t total = MAX(tx_len, rx_len);

	if (total == 0U) {
		return 0;
	}

	const struct spi_buf txb[] = {
		{.buf = (void *)tx, .len = tx_len},
		{.buf = NULL, .len = total - tx_len},
	};
	const struct spi_buf rxb[] = {
		{.buf = rx, .len = rx_len},
		{.buf = NULL, .len = total - rx_len},
	};
	const struct spi_buf_set txs = {.buffers = txb, .count = ARRAY_SIZE(txb)};
	const struct spi_buf_set rxs = {.buffers = rxb, .count = ARRAY_SIZE(rxb)};

	return spi_transceive_dt(&config.spi, &txs, &rxs);
}

static void hw_reset(void)
{
	gpio_pin_set_dt(&config.resetn, 1);
	k_msleep(CONFIG_WIFI_EMW3080_RESET_PULSE_MS);
	gpio_pin_set_dt(&config.resetn, 0);
	k_msleep(CONFIG_WIFI_EMW3080_BOOT_DELAY_MS);
}

static int spi_exchange(const struct emw3080_spi_tx *tx, struct emw3080_spi_rx *rx)
{
	struct emw3080_spi_hdr ctrl_hdr = hdr_make_controller(tx->len);
	struct emw3080_spi_hdr periph_hdr = {0};
	uint8_t *rx_data = rx->data;
	uint16_t periph_len = 0;
	int ret;

	rx->len = 0;

	k_sem_reset(&flow_sem);
	gpio_pin_set_dt(&config.nss, 1);

	ret = wait_flow_rise();
	if (ret != 0) {
		LOG_ERR("FLOW timeout before header");
		goto out;
	}

	ret = xfer((const uint8_t *)&ctrl_hdr, sizeof(ctrl_hdr), (uint8_t *)&periph_hdr,
		   sizeof(periph_hdr));
	if (ret != 0) {
		LOG_ERR("header transfer failed (%d)", ret);
		ret = -EIO;
		goto out;
	}

	ret = hdr_parse_peripheral(&periph_hdr, &periph_len);
	if (ret != 0 || (tx->len == 0U && periph_len == 0U)) {
		goto out;
	}

	/* Clock an oversized frame out anyway so the module drops it. */
	if (periph_len > rx->size) {
		LOG_ERR("rx frame %u B > buffer %u B, dropped", periph_len, rx->size);
		rx_data = NULL;
	}

	ret = wait_flow_rise();
	if (ret != 0) {
		LOG_ERR("FLOW timeout before payload");
		goto out;
	}

	ret = xfer(tx->data, tx->len, rx_data, periph_len);
	if (ret != 0) {
		LOG_ERR("payload transfer failed (%d)", ret);
		ret = -EIO;
	} else if (rx_data != NULL) {
		rx->len = periph_len;
	}

out:
	gpio_pin_set_dt(&config.nss, 0);
	return ret;
}

int emw3080_spi_send(const uint8_t *data, uint16_t len, k_timeout_t timeout)
{
	int ret;

	if (data == NULL || len == 0U) {
		return -EINVAL;
	}
	if (len > EMW3080_SPI_MAX_FRAME_LEN) {
		return -EMSGSIZE;
	}

	k_mutex_lock(&send_lock, K_FOREVER);

	k_mutex_lock(&xfer_lock, K_FOREVER);
	k_sem_reset(&tx_done);
	tx_frame.data = data;
	tx_frame.len = len;
	tx_pending = true;
	k_mutex_unlock(&xfer_lock);

	k_sem_give(&wake_sem);

	if (k_sem_take(&tx_done, timeout) == 0) {
		ret = tx_result;
	} else {
		/* The thread cannot be using the frame while xfer_lock is held. */
		k_mutex_lock(&xfer_lock, K_FOREVER);
		if (tx_pending) {
			tx_pending = false;
			ret = -ETIMEDOUT;
		} else {
			ret = tx_result;
		}
		k_mutex_unlock(&xfer_lock);
	}

	k_mutex_unlock(&send_lock);
	return ret;
}

void emw3080_spi_reset(void)
{
	k_mutex_lock(&xfer_lock, K_FOREVER);
	hw_reset();
	k_mutex_unlock(&xfer_lock);
	k_sem_give(&wake_sem);
}

static void transport_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		k_sem_take(&wake_sem, K_FOREVER);

		while (true) {
			struct emw3080_spi_tx tx = {0};
			bool has_tx;
			int ret;

			k_mutex_lock(&xfer_lock, K_FOREVER);

			has_tx = tx_pending;
			if (!has_tx && !notify_pending()) {
				k_mutex_unlock(&xfer_lock);
				break;
			}
			if (has_tx) {
				tx = tx_frame;
			}

			ret = spi_exchange(&tx, &spi_rx);

			if (has_tx) {
				tx_pending = false;
				tx_result = ret;
				k_sem_give(&tx_done);
			}

			k_mutex_unlock(&xfer_lock);

			if (spi_rx.len > 0U) {
				rx_cb(spi_rx.data, spi_rx.len, rx_cb_user);
			}

			if (ret != 0) {
				/* Do not spin if NOTIFY stays high on a failing link. */
				k_msleep(EMW3080_SPI_ERR_BACKOFF_MS);
			}
		}
	}
}

static int pins_setup(void)
{
	const struct gpio_dt_spec *pins[] = {
		&config.resetn,
		&config.nss,
		&config.notify,
		&config.flow,
	};
	int ret;

	ARRAY_FOR_EACH(pins, i) {
		if (!gpio_is_ready_dt(pins[i])) {
			return -ENODEV;
		}
	}
	if (!spi_is_ready_dt(&config.spi)) {
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config.resetn, GPIO_OUTPUT_INACTIVE);
	if (ret != 0) {
		return ret;
	}
	ret = gpio_pin_configure_dt(&config.nss, GPIO_OUTPUT_INACTIVE);
	if (ret != 0) {
		return ret;
	}
	ret = gpio_pin_configure_dt(&config.notify, GPIO_INPUT);
	if (ret != 0) {
		return ret;
	}
	ret = gpio_pin_configure_dt(&config.flow, GPIO_INPUT);
	if (ret != 0) {
		return ret;
	}

	gpio_init_callback(&flow_cb, flow_isr, BIT(config.flow.pin));
	gpio_init_callback(&notify_cb, notify_isr, BIT(config.notify.pin));

	ret = gpio_add_callback_dt(&config.flow, &flow_cb);
	if (ret != 0) {
		return ret;
	}
	ret = gpio_add_callback_dt(&config.notify, &notify_cb);
	if (ret != 0) {
		return ret;
	}
	ret = gpio_pin_interrupt_configure_dt(&config.flow, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		return ret;
	}
	return gpio_pin_interrupt_configure_dt(&config.notify, GPIO_INT_EDGE_TO_ACTIVE);
}

int emw3080_spi_init(emw3080_spi_rx_cb_t cb, void *user)
{
	k_tid_t tid;
	int ret;

	if (cb == NULL) {
		return -EINVAL;
	}

	ret = pins_setup();
	if (ret != 0) {
		LOG_ERR("pin setup failed (%d)", ret);
		return ret;
	}

	hw_reset();

	rx_cb = cb;
	rx_cb_user = user;

	tid = k_thread_create(&transport_thread_data, transport_stack,
			      K_THREAD_STACK_SIZEOF(transport_stack), transport_thread, NULL, NULL,
			      NULL, K_PRIO_PREEMPT(CONFIG_WIFI_EMW3080_SPI_THREAD_PRIORITY), 0,
			      K_NO_WAIT);
	k_thread_name_set(tid, "emw3080_spi");
	k_sem_give(&wake_sem);

	return 0;
}
