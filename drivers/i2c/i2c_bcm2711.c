/*
 * Copyright (c) 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT brcm_bcm2711_i2c

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#ifdef CONFIG_I2C_BCM2711_BUS_RECOVERY
#include <zephyr/drivers/gpio.h>
#include "i2c_bitbang.h"
#endif /* CONFIG_I2C_BCM2711_BUS_RECOVERY */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bcm2711_i2c, CONFIG_I2C_LOG_LEVEL);

#include "i2c-priv.h"

/* Register definitions */
#define BCM2711_I2C_CR   0x00
#define BCM2711_I2C_SR   0x04
#define BCM2711_I2C_DLEN 0x08
#define BCM2711_I2C_ADDR 0x0C
#define BCM2711_I2C_FIFO 0x10
#define BCM2711_I2C_DIV  0x14
#define BCM2711_I2C_DEL  0x18
#define BCM2711_I2C_CLKT 0x1C

/* Control register bits */
#define BCM2711_I2C_CR_READ  BIT(0)
#define BCM2711_I2C_CR_CLEAR BIT(4)
#define BCM2711_I2C_CR_ST    BIT(7)
#define BCM2711_I2C_CR_INTD  BIT(8)
#define BCM2711_I2C_CR_INTT  BIT(9)
#define BCM2711_I2C_CR_INTR  BIT(10)
#define BCM2711_I2C_CR_I2CEN BIT(15)

/* Status register bits */
#define BCM2711_I2C_SR_TA   BIT(0)
#define BCM2711_I2C_SR_DONE BIT(1)
#define BCM2711_I2C_SR_TXW  BIT(2)
#define BCM2711_I2C_SR_RXR  BIT(3)
#define BCM2711_I2C_SR_TXD  BIT(4)
#define BCM2711_I2C_SR_RXD  BIT(5)
#define BCM2711_I2C_SR_TXE  BIT(6)
#define BCM2711_I2C_SR_RXF  BIT(7)
#define BCM2711_I2C_SR_ERR  BIT(8)
#define BCM2711_I2C_SR_CLKT BIT(9)

#define BCM2711_I2C_EDGE_DELAY(divider) ((((divider) / 4) << 16) | ((divider) / 4))
#define BCM2711_I2C_MSG_LEN             (1 << (sizeof(uint8_t) * 8))
#define BCM2711_I2C_TIMEOUT             100

struct bcm2711_i2c_config {
	DEVICE_MMIO_ROM;
	uint32_t bitrate;
	uint32_t pclk;
	void (*irq_config_func)(const struct device *dev);
	const struct pinctrl_dev_config *pincfg;
#ifdef CONFIG_I2C_BCM2711_BUS_RECOVERY
	struct gpio_dt_spec scl;
	struct gpio_dt_spec sda;
#endif /* CONFIG_I2C_BCM2711_BUS_RECOVERY */
};

struct bcm2711_i2c_data {
	DEVICE_MMIO_RAM;
	struct k_sem bus_sem;
	struct k_sem dev_sync;
	struct i2c_msg *msgs;
	uint8_t m_buf[BCM2711_I2C_MSG_LEN];
	uint8_t num_msgs;
	uint16_t i2c_addr;
	int msg_err;
};

static inline uint32_t bcm2711_i2c_read_reg(const struct device *dev, uint32_t reg)
{
	return sys_read32(DEVICE_MMIO_GET(dev) + reg);
}

static inline void bcm2711_i2c_write_reg(const struct device *dev, uint32_t reg, uint32_t value)
{
	sys_write32(value, DEVICE_MMIO_GET(dev) + reg);
}

static void bcm2711_i2c_reset(const struct device *dev)
{
	/* Clear FIFO and reset state machine */
	bcm2711_i2c_write_reg(dev, BCM2711_I2C_CR, BCM2711_I2C_CR_CLEAR);

	/* Clear status flags */
	bcm2711_i2c_write_reg(dev, BCM2711_I2C_SR,
			      BCM2711_I2C_SR_CLKT | BCM2711_I2C_SR_ERR | BCM2711_I2C_SR_DONE);
}

static void bcm2711_i2c_send(const struct device *dev)
{
	struct bcm2711_i2c_data *data = dev->data;
	struct i2c_msg *msg = data->msgs;
	uint32_t status;

	for (uint32_t i = 0; i < msg->len; i++) {
		status = bcm2711_i2c_read_reg(dev, BCM2711_I2C_SR);
		if ((status & BCM2711_I2C_SR_TXD) == 0) {
			LOG_WRN("FIFO is full");
			break;
		}
		bcm2711_i2c_write_reg(dev, BCM2711_I2C_FIFO, msg->buf[i]);
	}
}

static void bcm2711_i2c_recv(const struct device *dev)
{
	struct bcm2711_i2c_data *data = dev->data;
	struct i2c_msg *msg = data->msgs;
	uint32_t status;

	for (uint32_t i = 0; i < msg->len; i++) {
		status = bcm2711_i2c_read_reg(dev, BCM2711_I2C_SR);
		if ((status & BCM2711_I2C_SR_RXD) == 0) {
			LOG_WRN("FIFO is empty");
			break;
		}
		msg->buf[i] = bcm2711_i2c_read_reg(dev, BCM2711_I2C_FIFO);
	}
}

static void bcm2711_i2c_transaction(const struct device *dev, struct i2c_msg *msg,
				    uint16_t i2c_addr)
{
	struct bcm2711_i2c_data *data = dev->data;
	uint32_t ctrl;

	data->num_msgs--;

	/* Configure control register */
	ctrl = BCM2711_I2C_CR_ST | BCM2711_I2C_CR_I2CEN;

	if (i2c_is_read_op(msg)) {
		ctrl |= BCM2711_I2C_CR_READ | BCM2711_I2C_CR_INTR;
	} else {
		ctrl |= BCM2711_I2C_CR_INTT;
	}

	if (data->num_msgs == 0) {
		ctrl |= BCM2711_I2C_CR_INTD;
	}

	/* Set slave address and message length */
	bcm2711_i2c_write_reg(dev, BCM2711_I2C_ADDR, i2c_addr);
	bcm2711_i2c_write_reg(dev, BCM2711_I2C_DLEN, msg->len);

	/* Start transfer */
	bcm2711_i2c_write_reg(dev, BCM2711_I2C_CR, ctrl);
}

static int bcm2711_i2c_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
				uint16_t i2c_addr)
{
	struct bcm2711_i2c_data *data = dev->data;
	int ret;

	if (msgs == NULL) {
		LOG_ERR("No messages to transfer");
		return -EINVAL;
	}

	k_sem_take(&data->bus_sem, K_FOREVER);

	/* Coalesce two consecutive write messages (e.g. i2c_burst_write)
	 * into one. BCM2711 I2C cannot do write + repeated-START + write.
	 */
	if (num_msgs == 2) {
		if (!i2c_is_read_op(&msgs[0]) && !i2c_is_read_op(&msgs[1])) {
			if (msgs[0].len + msgs[1].len <= BCM2711_I2C_MSG_LEN) {
				memcpy(data->m_buf, msgs[0].buf, msgs[0].len);
				memcpy(data->m_buf + msgs[0].len, msgs[1].buf, msgs[1].len);

				msgs[0].buf = data->m_buf;
				msgs[0].len = msgs[0].len + msgs[1].len;
				msgs[0].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

				num_msgs--;
			} else {
				LOG_ERR("Cannot transfer packet of length > %d",
					BCM2711_I2C_MSG_LEN);
				return -ENOTSUP;
			}
		}
	}

	data->msgs = msgs;
	data->num_msgs = num_msgs;
	data->i2c_addr = i2c_addr;
	data->msg_err = 0;

	/* Start transfer */
	bcm2711_i2c_transaction(dev, data->msgs, data->i2c_addr);

	/* Wait for transfer completion with timeout */
	ret = k_sem_take(&data->dev_sync, K_MSEC(BCM2711_I2C_TIMEOUT));
	if (ret < 0) {
		LOG_ERR("I2C transfer timed out");
		bcm2711_i2c_reset(dev);
		k_sem_give(&data->bus_sem);
		k_sem_give(&data->dev_sync);
		return ret;
	}

	k_sem_give(&data->bus_sem);

	/* Check for transfer errors */
	return data->msg_err;
}

static void bcm2711_i2c_isr(const struct device *dev)
{
	struct bcm2711_i2c_data *data = dev->data;
	uint32_t status;

	status = bcm2711_i2c_read_reg(dev, BCM2711_I2C_SR);

	if ((status & (BCM2711_I2C_SR_CLKT | BCM2711_I2C_SR_ERR)) > 0) {
		data->msg_err = -EIO;
		bcm2711_i2c_reset(dev);
		k_sem_give(&data->dev_sync);
		return;
	}

	if ((status & BCM2711_I2C_SR_DONE) > 0) {
		if (i2c_is_read_op(data->msgs)) {
			bcm2711_i2c_recv(dev);
		}

		bcm2711_i2c_reset(dev);
		k_sem_give(&data->dev_sync);
		return;
	}

	if ((status & BCM2711_I2C_SR_TXW) > 0) {
		bcm2711_i2c_send(dev);

		if (data->num_msgs > 0) {
			data->msgs++;
			bcm2711_i2c_transaction(dev, data->msgs, data->i2c_addr);
		}
	}

	if ((status & BCM2711_I2C_SR_RXR) > 0) {
		bcm2711_i2c_recv(dev);
	}
}

static int bcm2711_i2c_configure(const struct device *dev, uint32_t dev_config)
{
	const struct bcm2711_i2c_config *config = dev->config;
	uint32_t divider, edge_delay;

	/* Configure clock divider based on requested speed */
	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		LOG_DBG("Standard mode selected");
		divider = config->pclk / I2C_BITRATE_STANDARD;
		break;
	case I2C_SPEED_FAST:
		LOG_DBG("Fast mode selected");
		divider = config->pclk / I2C_BITRATE_FAST;
		break;
	default:
		LOG_ERR("Only Standard or Fast modes are supported");
		return -EINVAL;
	}

	/* Set clock divider */
	bcm2711_i2c_write_reg(dev, BCM2711_I2C_DIV, divider);

	/* Set edge delay */
	edge_delay = BCM2711_I2C_EDGE_DELAY(divider);
	bcm2711_i2c_write_reg(dev, BCM2711_I2C_DEL, edge_delay);

	return 0;
}

#if CONFIG_I2C_BCM2711_BUS_RECOVERY
static void bcm2711_i2c_bitbang_set_scl(void *io_context, int state)
{
	const struct bcm2711_i2c_config *config = io_context;

	gpio_pin_set_dt(&config->scl, state);
}

static void bcm2711_i2c_bitbang_set_sda(void *io_context, int state)
{
	const struct bcm2711_i2c_config *config = io_context;

	gpio_pin_set_dt(&config->sda, state);
}

static int bcm2711_i2c_bitbang_get_sda(void *io_context)
{
	const struct bcm2711_i2c_config *config = io_context;

	return gpio_pin_get_dt(&config->sda) == 0 ? 0 : 1;
}

static int bcm2711_i2c_recover_bus(const struct device *dev)
{
	const struct bcm2711_i2c_config *config = dev->config;
	struct bcm2711_i2c_data *data = dev->data;
	struct i2c_bitbang bitbang_ctx;
	struct i2c_bitbang_io bitbang_io = {
		.set_scl = bcm2711_i2c_bitbang_set_scl,
		.set_sda = bcm2711_i2c_bitbang_set_sda,
		.get_sda = bcm2711_i2c_bitbang_get_sda,
	};
	uint32_t bitrate;
	int ret;

	LOG_ERR("attempting to recover bus");

	if (!gpio_is_ready_dt(&config->scl)) {
		LOG_ERR("SCL GPIO device not ready");
		return -EIO;
	}

	if (!gpio_is_ready_dt(&config->sda)) {
		LOG_ERR("SDA GPIO device not ready");
		return -EIO;
	}

	k_sem_take(&data->bus_sem, K_FOREVER);

	ret = gpio_pin_configure_dt(&config->scl, GPIO_OUTPUT_HIGH);
	if (ret < 0) {
		LOG_ERR("Failed to configure SCL GPIO (err %d)", ret);
		goto restore;
	}

	ret = gpio_pin_configure_dt(&config->sda, GPIO_OUTPUT_HIGH);
	if (ret < 0) {
		LOG_ERR("Failed to configure SDA GPIO (err %d)", ret);
		goto restore;
	}

	i2c_bitbang_init(&bitbang_ctx, &bitbang_io, (void *)config);

	bitrate = i2c_map_dt_bitrate(config->bitrate) | I2C_MODE_CONTROLLER;
	ret = i2c_bitbang_configure(&bitbang_ctx, bitrate);
	if (ret < 0) {
		LOG_ERR("Failed to configure I2C bitbang (err %d)", ret);
		goto restore;
	}

	ret = i2c_bitbang_recover_bus(&bitbang_ctx);
	if (ret < 0) {
		LOG_ERR("Failed to recover bus (err %d)", ret);
	}

restore:
	(void)pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);

	k_sem_give(&data->bus_sem);

	return ret;
}
#endif /* CONFIG_I2C_BCM2711_BUS_RECOVERY */

static int bcm2711_i2c_init(const struct device *dev)
{
	struct bcm2711_i2c_data *data = dev->data;
	const struct bcm2711_i2c_config *config = dev->config;
	uint32_t bitrate;
	int ret;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	k_sem_init(&data->bus_sem, 1, 1);
	k_sem_init(&data->dev_sync, 0, 1);

	/* Clear control registers and disable CLKT */
	bcm2711_i2c_write_reg(dev, BCM2711_I2C_CLKT, 0);
	bcm2711_i2c_write_reg(dev, BCM2711_I2C_CR, 0);

	bitrate = i2c_map_dt_bitrate(config->bitrate);

	ret = bcm2711_i2c_configure(dev, bitrate);
	if (ret < 0) {
		LOG_ERR("Failed to configure I2C controller");
		return ret;
	}

	config->irq_config_func(dev);

	return 0;
}

static DEVICE_API(i2c, bcm2711_i2c_driver_api) = {
	.configure = bcm2711_i2c_configure,
	.transfer = bcm2711_i2c_transfer,
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif /* CONFIG_I2C_RTIO */
#if CONFIG_I2C_BCM2711_BUS_RECOVERY
	.recover_bus = bcm2711_i2c_recover_bus,
#endif /* CONFIG_I2C_BCM2711_BUS_RECOVERY */
};

#define BCM2711_I2C_PINCTRL_DEFINE(port) PINCTRL_DT_INST_DEFINE(port)

#define BCM2711_I2C_IRQ_CONF_FUNC(port)                                                            \
	static void bcm2711_i2c_irq_config_func_##port(const struct device *dev)                   \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		IRQ_CONNECT(DT_INST_IRQN(port), DT_INST_IRQ(port, priority), bcm2711_i2c_isr,      \
			    DEVICE_DT_INST_GET(port), 0);                                          \
		irq_enable(DT_INST_IRQN(port));                                                    \
	}

#define BCM2711_I2C_DEV_DATA(port) static struct bcm2711_i2c_data bcm2711_i2c_data_##port

#define BCM2711_I2C_DEV_CFG(port)                                                                  \
	static const struct bcm2711_i2c_config bcm2711_i2c_config_##port = {                       \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(port)),                                           \
		.bitrate = DT_INST_PROP(port, clock_frequency),                                    \
		.pclk = DT_INST_PROP_BY_PHANDLE(port, clocks, clock_frequency),                    \
		.irq_config_func = bcm2711_i2c_irq_config_func_##port,                             \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(port),                                    \
		IF_ENABLED(CONFIG_I2C_BCM2711_BUS_RECOVERY,                                        \
		(.scl = GPIO_DT_SPEC_INST_GET_OR(port, scl_gpios, {0}),                            \
		 .sda = GPIO_DT_SPEC_INST_GET_OR(port, sda_gpios, {0}),))                          \
	};

#define BCM2711_I2C_INIT(port)                                                                     \
	DEVICE_DT_INST_DEFINE(port, &bcm2711_i2c_init, NULL, &bcm2711_i2c_data_##port,             \
			      &bcm2711_i2c_config_##port, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,   \
			      &bcm2711_i2c_driver_api);

#define BCM2711_I2C_INSTANTIATE(inst)                                                              \
	BCM2711_I2C_PINCTRL_DEFINE(inst);                                                          \
	BCM2711_I2C_IRQ_CONF_FUNC(inst);                                                           \
	BCM2711_I2C_DEV_DATA(inst);                                                                \
	BCM2711_I2C_DEV_CFG(inst);                                                                 \
	BCM2711_I2C_INIT(inst);

DT_INST_FOREACH_STATUS_OKAY(BCM2711_I2C_INSTANTIATE)
