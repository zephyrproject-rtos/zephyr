/*
 * Copyright (c) 2022, Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT google_openthread_spinel

#define LOG_MODULE_NAME net_otPlat_spi_host

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_OPENTHREAD_L2_LOG_LEVEL);

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/pm/device.h>

#include <openthread/instance.h>
#include <openthread/ip6.h>
#include <openthread/message.h>
#include <openthread-system.h>

#include "platform-zephyr.h"

struct ot_spinel_config {
	struct spi_dt_spec bus;
	struct gpio_dt_spec rst_gpio;
	struct gpio_dt_spec irq_gpio;
	int rst_time;
	int startup_time;
};

struct ot_spinel_data {
	struct gpio_callback irq_gpio_callback;

	struct k_sem recv_sem;
	struct k_fifo tx_pkt_fifo;

	bool ready;
};

/* The only spinel device */
static const struct device *ot_spinel_dev = DEVICE_DT_INST_GET(0);

int notify_new_rx_frame(struct net_pkt *pkt)
{
	/* There isn't any actual radio, openthread stack already handled this */
	net_pkt_unref(pkt);

	/* Return ok, no need to print errors */
	return 0;
}

int notify_new_tx_frame(struct net_pkt *pkt)
{
	struct ot_spinel_data *data = ot_spinel_dev->data;

	if (!data->ready) {
		return -ENODEV;
	}

	k_fifo_put(&data->tx_pkt_fifo, pkt);
	otSysEventSignalPending();

	return 0;
}

static void openthread_handle_frame_to_send(otInstance *instance,
					    struct net_pkt *pkt)
{
	struct net_buf *buf;
	otMessage *message;
	otMessageSettings settings;

	NET_DBG("Sending Ip6 packet to OT stack");

	settings.mPriority = OT_MESSAGE_PRIORITY_NORMAL;
	settings.mLinkSecurityEnabled = true;
	message = otIp6NewMessage(instance, &settings);
	if (message == NULL) {
		goto exit;
	}

	for (buf = pkt->buffer; buf; buf = buf->frags) {
		if (otMessageAppend(message, buf->data, buf->len) != OT_ERROR_NONE) {
			NET_ERR("Error while appending to otMessage");
			otMessageFree(message);
			goto exit;
		}
	}

	if (otIp6Send(instance, message) != OT_ERROR_NONE) {
		NET_ERR("Error while calling otIp6Send");
		goto exit;
	}

exit:
	net_pkt_unref(pkt);
}

void platformSpiHostInit()
{
	const struct ot_spinel_config *config = ot_spinel_dev->config;
	struct ot_spinel_data *data = ot_spinel_dev->data;

	if (!device_is_ready(ot_spinel_dev)) {
		LOG_ERR("No Spinel device ready");
		return;
	}

	if (!data->ready) {
		data->ready = true;
		gpio_add_callback(config->irq_gpio.port, &data->irq_gpio_callback);
	}

	if (config->rst_gpio.port) {
		gpio_pin_set_dt(&config->rst_gpio, 1);
		k_sleep(K_MSEC(config->rst_time));
		gpio_pin_set_dt(&config->rst_gpio, 0);
		k_sleep(K_MSEC(config->startup_time));
	}
}

bool platformSpiHostCheckInterrupt()
{
	const struct ot_spinel_config *config = ot_spinel_dev->config;
	struct ot_spinel_data *data = ot_spinel_dev->data;

	if (!data->ready) {
		return false;
	}

	/* If we don't have an interrupt port, we always poll the RCP */
	if (!config->irq_gpio.port) {
		return true;
	}

	return k_sem_count_get(&data->recv_sem) > 0;
}

bool platformSpiHostWaitForFrame(uint64_t aTimeoutUs)
{
	const struct ot_spinel_config *config = ot_spinel_dev->config;
	struct ot_spinel_data *data = ot_spinel_dev->data;

	if (!data->ready) {
		return false;
	}

	if (!config->irq_gpio.port) {
		return true;
	}

	return k_sem_take(&data->recv_sem, K_USEC(aTimeoutUs)) == 0;
}

void platformSpiHostProcess(otInstance *aInstance)
{
	struct ot_spinel_data *data = ot_spinel_dev->data;
	struct net_pkt *tx_pkt;

	if (!data->ready) {
		return;
	}

	while ((tx_pkt = (struct net_pkt *) k_fifo_get(&data->tx_pkt_fifo, K_NO_WAIT)) != NULL) {
		openthread_handle_frame_to_send(aInstance, tx_pkt);
	}
}

int platformSpiHostTransfer(uint8_t *aSpiTxFrameBuffer,
			    uint8_t *aSpiRxFrameBuffer,
			    uint32_t aTransferLength)
{
	const struct ot_spinel_config *config = ot_spinel_dev->config;
	struct ot_spinel_data *data = ot_spinel_dev->data;

	if (!data->ready) {
		return -ENODEV;
	}

	struct spi_buf tx_buf = {
		.buf = aSpiTxFrameBuffer,
		.len = aTransferLength
	};
	struct spi_buf_set tx_bufs = {
		.buffers = &tx_buf,
		.count = 1,
	};

	struct spi_buf rx_buf = {
		.buf = aSpiRxFrameBuffer,
		.len = aTransferLength
	};
	struct spi_buf_set rx_bufs = {
		.buffers = &rx_buf,
		.count = 1,
	};

	return spi_transceive_dt(&config->bus, &tx_bufs, &rx_bufs);
}

static void ot_spinel_interrupt_handler(const struct device *port,
					struct gpio_callback *cb,
					gpio_port_pins_t pins)
{
	struct ot_spinel_data *data = CONTAINER_OF(cb, struct ot_spinel_data, irq_gpio_callback);

	k_sem_give(&data->recv_sem);
	otSysEventSignalPending();
}

#ifdef CONFIG_PM_DEVICE
static int ot_spinel_pm_action(const struct device *dev,
			       enum pm_device_action action)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(action);

	return -ENOTSUP;
}
#endif /* CONFIG_PM_DEVICE */

static int ot_spinel_init(const struct device *dev)
{
	const struct ot_spinel_config *config = dev->config;
	struct ot_spinel_data *data = dev->data;

	k_sem_init(&data->recv_sem, 0, 1);
	k_fifo_init(&data->tx_pkt_fifo);

	if (!spi_is_ready(&config->bus)) {
		LOG_ERR("SPI device not ready");
		return -ENODEV;
	}

	/* configure the optional interrupt input gpio */
	if (config->irq_gpio.port) {
		if (!device_is_ready(config->irq_gpio.port)) {
			LOG_ERR("Interrupt GPIO device not ready");
			return -ENODEV;
		}

		if (gpio_pin_configure_dt(&config->irq_gpio, GPIO_INPUT)) {
			LOG_ERR("Couldn't configure interrupt pin");
			return -EIO;
		}

		gpio_pin_interrupt_configure_dt(&config->irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);

		gpio_init_callback(&data->irq_gpio_callback, ot_spinel_interrupt_handler,
			BIT(config->irq_gpio.pin));
	}

	/* configure the optional reset output gpio */
	if (config->rst_gpio.port) {
		if (!device_is_ready(config->rst_gpio.port)) {
			LOG_ERR("Reset GPIO device not ready");
			return -ENODEV;
		}

		if (gpio_pin_configure_dt(&config->rst_gpio, GPIO_OUTPUT)) {
			LOG_ERR("Couldn't configure reset pin");
			return -EIO;
		}
	}

	return 0;
}

static const struct ot_spinel_config ot_spinel_config = {
	.bus = SPI_DT_SPEC_INST_GET(0, SPI_OP_MODE_MASTER | SPI_WORD_SET(8),
				    DT_INST_PROP_OR(0, cs_delay, 0)),
	.irq_gpio = GPIO_DT_SPEC_INST_GET_OR(0, irq_gpios, {}),
	.rst_gpio = GPIO_DT_SPEC_INST_GET_OR(0, reset_gpios, {}),
	.rst_time = DT_INST_PROP_OR(0, reset_time, 1),
	.startup_time = DT_INST_PROP_OR(0, startup_time, 0),
};

static struct ot_spinel_data ot_spinel_data;

PM_DEVICE_DT_INST_DEFINE(0, ot_spinel_pm_action);

DEVICE_DT_INST_DEFINE(0, &ot_spinel_init, PM_DEVICE_DT_INST_GET(0),
		      &ot_spinel_data, &ot_spinel_config, POST_KERNEL,
		      CONFIG_SPI_INIT_PRIORITY_DEVICE, NULL);
