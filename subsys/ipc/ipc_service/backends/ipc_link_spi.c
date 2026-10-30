/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * IPC link transport over an SPI bus, between a controller and a peripheral MCU. Every
 * transaction is one frame each way, of the frame size. The peripheral drives two lines: RDY
 * while it has a transfer armed, IRQ while it has something to send. The controller clocks a
 * transaction while RDY is active and either side has something to send.
 */

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/ipc/ipc_link.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "ipc_link_priv.h"

LOG_MODULE_REGISTER(ipc_link_spi, CONFIG_IPC_SERVICE_BACKEND_LINK_LOG_LEVEL);

/*
 * How long the controller waits for RDY before it reads the lines again, and how long the
 * peripheral pauses after a transfer that fails right away
 */
#define IPC_LINK_SPI_RETRY K_MSEC(100)

#define IPC_LINK_SPI_OPERATION (SPI_WORD_SET(8) | SPI_TRANSFER_MSB)

struct ipc_link_spi_config {
	struct ipc_link_config link;
	struct spi_dt_spec bus;
	/* Driven by the peripheral while it has a transfer armed */
	struct gpio_dt_spec rdy;
	/* Driven by the peripheral while it has something to send */
	struct gpio_dt_spec irq;
	k_thread_stack_t *stack;
	size_t stack_size;
	bool controller;
};

struct ipc_link_spi_data {
	const struct device *dev;
	struct k_thread thread;
	/* Opened, or something to send on the controller */
	struct k_sem kick;
	/* RDY became active */
	struct k_sem rdy;
	/* The peripheral's transfer completed */
	struct k_sem done;
	struct gpio_callback rdy_cb;
	struct gpio_callback irq_cb;
	int result;
};

#ifdef CONFIG_SPI_ASYNC
static void ipc_link_spi_peripheral_done(const struct device *bus, int result, void *userdata)
{
	const struct device *dev = userdata;
	const struct ipc_link_spi_config *cfg = dev->config;
	struct ipc_link_spi_data *data = dev->data;

	ARG_UNUSED(bus);

	(void)gpio_pin_set_dt(&cfg->rdy, 0);

	data->result = result;
	k_sem_give(&data->done);
}

/* RDY may only rise once the transfer is armed, or the controller clocks an unarmed peripheral */
static int ipc_link_spi_peripheral_transceive(const struct device *dev,
					      const struct spi_buf_set *tx,
					      const struct spi_buf_set *rx)
{
	const struct ipc_link_spi_config *cfg = dev->config;
	struct ipc_link_spi_data *data = dev->data;
	int ret;

	ret = spi_transceive_cb(cfg->bus.bus, &cfg->bus.config, tx, rx,
				ipc_link_spi_peripheral_done, (void *)dev);
	if (ret < 0) {
		return ret;
	}

	(void)gpio_pin_set_dt(&cfg->rdy, 1);
	(void)k_sem_take(&data->done, K_FOREVER);

	return data->result;
}
#else
static int ipc_link_spi_peripheral_transceive(const struct device *dev,
					      const struct spi_buf_set *tx,
					      const struct spi_buf_set *rx)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(tx);
	ARG_UNUSED(rx);

	return -ENOTSUP;
}
#endif /* CONFIG_SPI_ASYNC */

/* The peripheral keeps IRQ active while it has something to send */
static void ipc_link_spi_set_irq(const struct device *dev)
{
	const struct ipc_link_spi_config *cfg = dev->config;
	bool pending = ipc_link_pending(dev);

	(void)gpio_pin_set_dt(&cfg->irq, pending ? 1 : 0);

	/* A message queued in between raised IRQ on its own, don't leave it lowered */
	if (!pending && ipc_link_pending(dev)) {
		(void)gpio_pin_set_dt(&cfg->irq, 1);
	}
}

/* One transaction */
static void ipc_link_spi_transfer(const struct device *dev)
{
	const struct ipc_link_spi_config *cfg = dev->config;
	struct ipc_link_spi_data *data = dev->data;
	/* One buffer each way, the frame size: what a peripheral's DMA can run */
	struct spi_buf tx_buf = {.len = cfg->link.mtu};
	struct spi_buf rx_buf = {.len = cfg->link.mtu};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
	const struct spi_buf_set rx = {.buffers = &rx_buf, .count = 1};
	size_t len;
	int ret;

	/* Before the frame is built: IRQ tells the controller that this frame carries something,
	 * and building it takes that from the link
	 */
	if (!cfg->controller) {
		ipc_link_spi_set_irq(dev);
	}

	tx_buf.buf = (void *)ipc_link_tx_prepare(dev, &len);
	rx_buf.buf = ipc_link_rx_buf(dev);

	if (cfg->controller) {
		k_sem_reset(&data->rdy);
		ret = spi_transceive_dt(&cfg->bus, &tx, &rx);
	} else {
		ret = ipc_link_spi_peripheral_transceive(dev, &tx, &rx);
	}

	if (ret < 0) {
		LOG_DBG("Transfer failed (%d)", ret);

		/* Don't spin on a bus that fails right away: the controller recovers lost
		 * transactions. Not after a bad header, the controller clocked that one.
		 */
		if (!cfg->controller) {
			k_sleep(IPC_LINK_SPI_RETRY);
		}
	}

	/* A peripheral driver reports the received count: a transaction cut short delivers
	 * only part of the frame, none at all after a chip select pulse without clocks, and
	 * the rest of the buffer is stale
	 */
	(void)ipc_link_rx(dev,
			  cfg->controller ? cfg->link.mtu : MIN((size_t)MAX(ret, 0), cfg->link.mtu),
			  ret >= 0);
}

static bool ipc_link_spi_wait_rdy(const struct device *dev)
{
	const struct ipc_link_spi_config *cfg = dev->config;
	struct ipc_link_spi_data *data = dev->data;

	(void)k_sem_take(&data->rdy, IPC_LINK_SPI_RETRY);

	/* The edge wakes the controller, but RDY may have fallen since: a rebooting peripheral.
	 * Without the edge, RDY may have been active already.
	 */
	return gpio_pin_get_dt(&cfg->rdy) > 0;
}

static void ipc_link_spi_controller_loop(const struct device *dev)
{
	const struct ipc_link_spi_config *cfg = dev->config;
	struct ipc_link_spi_data *data = dev->data;

	while (true) {
		if (!ipc_link_pending(dev) && gpio_pin_get_dt(&cfg->irq) <= 0) {
			/* An IRQ edge after the level was read gives the semaphore: none is lost */
			(void)k_sem_take(&data->kick, K_FOREVER);
			continue;
		}

		if (ipc_link_spi_wait_rdy(dev)) {
			ipc_link_spi_transfer(dev);
		}
	}
}

static void ipc_link_spi_peripheral_loop(const struct device *dev)
{
	while (true) {
		ipc_link_spi_transfer(dev);
	}
}

static void ipc_link_spi_thread(void *p1, void *p2, void *p3)
{
	const struct device *dev = p1;
	const struct ipc_link_spi_config *cfg = dev->config;
	struct ipc_link_spi_data *data = dev->data;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* Until the instance is opened: on the controller, an IRQ edge gives the semaphore too */
	do {
		(void)k_sem_take(&data->kick, K_FOREVER);
	} while (cfg->controller && !cfg->link.link->opened);

	if (cfg->controller) {
		ipc_link_spi_controller_loop(dev);
	} else {
		ipc_link_spi_peripheral_loop(dev);
	}
}

static int ipc_link_spi_open(const struct device *dev)
{
	const struct ipc_link_spi_config *cfg = dev->config;
	struct ipc_link_spi_data *data = dev->data;

	if (!cfg->controller && !IS_ENABLED(CONFIG_SPI_ASYNC)) {
		return -ENOTSUP;
	}

	k_sem_give(&data->kick);

	return 0;
}

static void ipc_link_spi_kick(const struct device *dev)
{
	const struct ipc_link_spi_config *cfg = dev->config;
	struct ipc_link_spi_data *data = dev->data;

	if (cfg->controller) {
		k_sem_give(&data->kick);
	} else {
		(void)gpio_pin_set_dt(&cfg->irq, 1);
	}
}

static const struct ipc_link_transport_api ipc_link_spi_api = {
	.open = ipc_link_spi_open,
	.kick = ipc_link_spi_kick,
};

static void ipc_link_spi_rdy_handler(const struct device *port, struct gpio_callback *cb,
				     gpio_port_pins_t pins)
{
	struct ipc_link_spi_data *data = CONTAINER_OF(cb, struct ipc_link_spi_data, rdy_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	k_sem_give(&data->rdy);
}

static void ipc_link_spi_irq_handler(const struct device *port, struct gpio_callback *cb,
				     gpio_port_pins_t pins)
{
	struct ipc_link_spi_data *data = CONTAINER_OF(cb, struct ipc_link_spi_data, irq_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	k_sem_give(&data->kick);
}

static int ipc_link_spi_input_init(const struct gpio_dt_spec *spec, struct gpio_callback *cb,
				   gpio_callback_handler_t handler)
{
	int ret;

	ret = gpio_pin_configure_dt(spec, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	gpio_init_callback(cb, handler, BIT(spec->pin));

	ret = gpio_add_callback_dt(spec, cb);
	if (ret < 0) {
		return ret;
	}

	return gpio_pin_interrupt_configure_dt(spec, GPIO_INT_EDGE_TO_ACTIVE);
}

static int ipc_link_spi_init(const struct device *dev)
{
	const struct ipc_link_spi_config *cfg = dev->config;
	struct ipc_link_spi_data *data = dev->data;
	k_tid_t tid;
	int ret;

	if (!spi_is_ready_dt(&cfg->bus) || !gpio_is_ready_dt(&cfg->rdy) ||
	    !gpio_is_ready_dt(&cfg->irq)) {
		LOG_ERR("SPI bus or GPIOs not ready");
		return -ENODEV;
	}

	data->dev = dev;
	k_sem_init(&data->kick, 0, 1);
	k_sem_init(&data->rdy, 0, 1);
	k_sem_init(&data->done, 0, 1);

	if (cfg->controller) {
		ret = ipc_link_spi_input_init(&cfg->rdy, &data->rdy_cb, ipc_link_spi_rdy_handler);
		if (ret == 0) {
			ret = ipc_link_spi_input_init(&cfg->irq, &data->irq_cb,
						      ipc_link_spi_irq_handler);
		}
	} else {
		ret = gpio_pin_configure_dt(&cfg->rdy, GPIO_OUTPUT_INACTIVE);
		if (ret == 0) {
			ret = gpio_pin_configure_dt(&cfg->irq, GPIO_OUTPUT_INACTIVE);
		}
	}

	if (ret < 0) {
		LOG_ERR("Failed to configure the GPIOs (%d)", ret);
		return ret;
	}

	ret = ipc_link_init(dev);
	if (ret < 0) {
		return ret;
	}

	tid = k_thread_create(&data->thread, cfg->stack, cfg->stack_size, ipc_link_spi_thread,
			      (void *)dev, NULL, NULL, CONFIG_IPC_SERVICE_BACKEND_LINK_SPI_PRIORITY,
			      0, K_NO_WAIT);
	k_thread_name_set(tid, dev->name);

	return 0;
}

#define IPC_LINK_SPI_DEFINE(inst, role, is_controller, mode)                                       \
	IPC_LINK_DEFINE(ipc_link_spi_##role##_##inst##_link, DT_INST_PROP(inst, mtu));             \
                                                                                                   \
	static K_KERNEL_STACK_DEFINE(ipc_link_spi_##role##_##inst##_stack,                         \
				     CONFIG_IPC_SERVICE_BACKEND_LINK_SPI_STACK_SIZE);              \
                                                                                                   \
	static struct ipc_link_spi_data ipc_link_spi_##role##_##inst##_data;                       \
                                                                                                   \
	static const struct ipc_link_spi_config ipc_link_spi_##role##_##inst##_config = {          \
		.link =                                                                            \
			{                                                                          \
				.api = &ipc_link_spi_api,                                          \
				IPC_LINK_CONFIG(ipc_link_spi_##role##_##inst##_link),              \
				.mtu = DT_INST_PROP(inst, mtu),                                    \
				.retransmit_timeout_ms =                                           \
					DT_INST_PROP(inst, retransmit_timeout_ms),                 \
			},                                                                         \
		.bus = SPI_DT_SPEC_INST_GET(inst, mode | IPC_LINK_SPI_OPERATION),                  \
		.rdy = GPIO_DT_SPEC_INST_GET(inst, rdy_gpios),                                     \
		.irq = GPIO_DT_SPEC_INST_GET(inst, irq_gpios),                                     \
		.stack = ipc_link_spi_##role##_##inst##_stack,                                     \
		.stack_size = K_KERNEL_STACK_SIZEOF(ipc_link_spi_##role##_##inst##_stack),         \
		.controller = is_controller,                                                       \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, ipc_link_spi_init, NULL, &ipc_link_spi_##role##_##inst##_data, \
			      &ipc_link_spi_##role##_##inst##_config, POST_KERNEL,                 \
			      CONFIG_IPC_SERVICE_BACKEND_LINK_INIT_PRIORITY,                       \
			      &ipc_link_backend_ops);

#define DT_DRV_COMPAT zephyr_ipc_spi
DT_INST_FOREACH_STATUS_OKAY_VARGS(IPC_LINK_SPI_DEFINE, controller, true, SPI_OP_MODE_CONTROLLER)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT zephyr_ipc_spi_peripheral
DT_INST_FOREACH_STATUS_OKAY_VARGS(IPC_LINK_SPI_DEFINE, peripheral, false, SPI_OP_MODE_PERIPHERAL)
#undef DT_DRV_COMPAT
