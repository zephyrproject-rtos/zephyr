/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ambiq_i2c

#include <errno.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

#include <am_mcu_apollo.h>

#include <zephyr/logging/log.h>
#include <zephyr/drivers/pinctrl.h>

LOG_MODULE_REGISTER(ambiq_i2c, CONFIG_I2C_LOG_LEVEL);

typedef int (*ambiq_i2c_pwr_func_t)(void);

#define PWRCTRL_MAX_WAIT_US       5
#define I2C_TRANSFER_TIMEOUT_MSEC 500 /* Transfer timeout period */

#include "i2c-priv.h"

struct i2c_ambiq_config {
	uint32_t base;
	int size;
	uint32_t bitrate;
	const struct pinctrl_dev_config *pcfg;
	ambiq_i2c_pwr_func_t pwr_func;
	void (*irq_config_func)(void);
};

typedef void (*i2c_ambiq_callback_t)(const struct device *dev, int result, void *data);

struct i2c_ambiq_data {
	am_hal_iom_config_t iom_cfg;
	void *iom_handler;
	struct k_sem bus_sem;
	struct k_sem transfer_sem;
	i2c_ambiq_callback_t callback;
	void *callback_data;
	int inst_idx;
	uint32_t transfer_status;
};

#ifdef CONFIG_I2C_AMBIQ_DMA
static __aligned(32) struct {
	__aligned(32) uint32_t buf[CONFIG_I2C_DMA_TCB_BUFFER_SIZE];
} i2c_dma_tcb_buf[DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)] __attribute__((__section__(".nocache")));

static void i2c_ambiq_callback(void *callback_ctxt, uint32_t status)
{
	const struct device *dev = callback_ctxt;
	struct i2c_ambiq_data *data = dev->data;

	if (data->callback) {
		data->callback(dev, status, data->callback_data);
	}
	data->transfer_status = status;
}
#endif

static void i2c_ambiq_isr(const struct device *dev)
{
	uint32_t ui32Status;
	struct i2c_ambiq_data *data = dev->data;

	am_hal_iom_interrupt_status_get(data->iom_handler, false, &ui32Status);
	am_hal_iom_interrupt_clear(data->iom_handler, ui32Status);
	am_hal_iom_interrupt_service(data->iom_handler, ui32Status);
	k_sem_give(&data->transfer_sem);
}

static int i2c_ambiq_read(const struct device *dev, struct i2c_msg *msg, uint16_t addr)
{
	struct i2c_ambiq_data *data = dev->data;

	int ret = 0;

	am_hal_iom_transfer_t trans = {0};

	trans.ui8Priority = 1;
	trans.eDirection = AM_HAL_IOM_RX;
	trans.uPeerInfo.ui32I2CDevAddr = addr;
	trans.ui32NumBytes = msg->len;
	trans.pui32RxBuffer = (uint32_t *)msg->buf;

#ifdef CONFIG_I2C_AMBIQ_DMA
	data->transfer_status = -EFAULT;
	ret = am_hal_iom_nonblocking_transfer(data->iom_handler, &trans, i2c_ambiq_callback,
					      (void *)dev);
	if (k_sem_take(&data->transfer_sem, K_MSEC(I2C_TRANSFER_TIMEOUT_MSEC))) {
		LOG_ERR("Timeout waiting for transfer complete");
		/* cancel timed out transaction */
		am_hal_iom_disable(data->iom_handler);
		/* clean up for next xfer */
		k_sem_reset(&data->transfer_sem);
		am_hal_iom_enable(data->iom_handler);
		return -ETIMEDOUT;
	}
	ret = data->transfer_status;
#else
	ret = am_hal_iom_blocking_transfer(data->iom_handler, &trans);
#endif
	return (ret != AM_HAL_STATUS_SUCCESS) ? -EIO : 0;
}

static int i2c_ambiq_write(const struct device *dev, struct i2c_msg *msg, uint16_t addr)
{
	struct i2c_ambiq_data *data = dev->data;

	int ret = 0;

	am_hal_iom_transfer_t trans = {0};

	trans.ui8Priority = 1;
	trans.eDirection = AM_HAL_IOM_TX;
	trans.uPeerInfo.ui32I2CDevAddr = addr;
	trans.ui32NumBytes = msg->len;
	trans.pui32TxBuffer = (uint32_t *)msg->buf;

#ifdef CONFIG_I2C_AMBIQ_DMA
	data->transfer_status = -EFAULT;
	ret = am_hal_iom_nonblocking_transfer(data->iom_handler, &trans, i2c_ambiq_callback,
					      (void *)dev);

	if (k_sem_take(&data->transfer_sem, K_MSEC(I2C_TRANSFER_TIMEOUT_MSEC))) {
		LOG_ERR("Timeout waiting for transfer complete");
		/* cancel timed out transaction */
		am_hal_iom_disable(data->iom_handler);
		/* clean up for next xfer */
		k_sem_reset(&data->transfer_sem);
		am_hal_iom_enable(data->iom_handler);
		return -ETIMEDOUT;
	}
	ret = data->transfer_status;
#else
	ret = am_hal_iom_blocking_transfer(data->iom_handler, &trans);
#endif

	return (ret != AM_HAL_STATUS_SUCCESS) ? -EIO : 0;
}

static int i2c_ambiq_configure(const struct device *dev, uint32_t dev_config)
{
	struct i2c_ambiq_data *data = dev->data;

	if (!(I2C_MODE_CONTROLLER & dev_config)) {
		return -EINVAL;
	}

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		data->iom_cfg.ui32ClockFreq = AM_HAL_IOM_100KHZ;
		break;
	case I2C_SPEED_FAST:
		data->iom_cfg.ui32ClockFreq = AM_HAL_IOM_400KHZ;
		break;
	case I2C_SPEED_FAST_PLUS:
		data->iom_cfg.ui32ClockFreq = AM_HAL_IOM_1MHZ;
		break;
	default:
		return -EINVAL;
	}

#ifdef CONFIG_I2C_AMBIQ_DMA
	data->iom_cfg.pNBTxnBuf = i2c_dma_tcb_buf[data->inst_idx].buf;
	data->iom_cfg.ui32NBTxnBufLength = CONFIG_I2C_DMA_TCB_BUFFER_SIZE;
#endif

	am_hal_iom_configure(data->iom_handler, &data->iom_cfg);

	return 0;
}

static int i2c_ambiq_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			      uint16_t addr)
{
	struct i2c_ambiq_data *data = dev->data;
	int ret = 0;

	if (!num_msgs) {
		return 0;
	}

	/* Send out messages */
	k_sem_take(&data->bus_sem, K_FOREVER);

	for (int i = 0; i < num_msgs; i++) {
		if (msgs[i].flags & I2C_MSG_READ) {
			ret = i2c_ambiq_read(dev, &(msgs[i]), addr);
		} else {
			ret = i2c_ambiq_write(dev, &(msgs[i]), addr);
		}

		if (ret != 0) {
			k_sem_give(&data->bus_sem);
			return ret;
		}
	}

	k_sem_give(&data->bus_sem);

	return 0;
}

static int i2c_ambiq_init(const struct device *dev)
{
	struct i2c_ambiq_data *data = dev->data;
	const struct i2c_ambiq_config *config = dev->config;
	uint32_t bitrate_cfg = i2c_map_dt_bitrate(config->bitrate);
	int ret = 0;

	data->iom_cfg.eInterfaceMode = AM_HAL_IOM_I2C_MODE;

	if (AM_HAL_STATUS_SUCCESS !=
	    am_hal_iom_initialize((config->base - REG_IOM_BASEADDR) / config->size,
				  &data->iom_handler)) {
		LOG_ERR("Fail to initialize I2C\n");
		return -ENXIO;
	}

	ret = config->pwr_func();

	ret |= i2c_ambiq_configure(dev, I2C_MODE_CONTROLLER | bitrate_cfg);
	if (ret < 0) {
		LOG_ERR("Fail to config I2C\n");
		goto end;
	}

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Fail to config I2C pins\n");
		goto end;
	}

#ifdef CONFIG_I2C_AMBIQ_DMA
	am_hal_iom_interrupt_clear(data->iom_handler, AM_HAL_IOM_INT_DCMP | AM_HAL_IOM_INT_CMDCMP);
	am_hal_iom_interrupt_enable(data->iom_handler, AM_HAL_IOM_INT_DCMP | AM_HAL_IOM_INT_CMDCMP);
	config->irq_config_func();
#endif

	if (AM_HAL_STATUS_SUCCESS != am_hal_iom_enable(data->iom_handler)) {
		LOG_ERR("Fail to enable I2C\n");
		ret = -EIO;
	}
end:
	if (ret < 0) {
		am_hal_iom_uninitialize(data->iom_handler);
	}
	return ret;
}

static const struct i2c_driver_api i2c_ambiq_driver_api = {
	.configure = i2c_ambiq_configure,
	.transfer = i2c_ambiq_transfer,
};

#define AMBIQ_I2C_DEFINE(n)                                                                        \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static int pwr_on_ambiq_i2c_##n(void)                                                      \
	{                                                                                          \
		uint32_t addr = DT_REG_ADDR(DT_INST_PHANDLE(n, ambiq_pwrcfg)) +                    \
				DT_INST_PHA(n, ambiq_pwrcfg, offset);                              \
		sys_write32((sys_read32(addr) | DT_INST_PHA(n, ambiq_pwrcfg, mask)), addr);        \
		k_busy_wait(PWRCTRL_MAX_WAIT_US);                                                  \
		return 0;                                                                          \
	}                                                                                          \
	static void i2c_irq_config_func_##n(void)                                                  \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), i2c_ambiq_isr,              \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	};                                                                                         \
	static struct i2c_ambiq_data i2c_ambiq_data##n = {                                         \
		.bus_sem = Z_SEM_INITIALIZER(i2c_ambiq_data##n.bus_sem, 1, 1),                     \
		.transfer_sem = Z_SEM_INITIALIZER(i2c_ambiq_data##n.transfer_sem, 0, 1),           \
		.inst_idx = n,                                                                     \
	};                                                                                         \
	static const struct i2c_ambiq_config i2c_ambiq_config##n = {                               \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.size = DT_INST_REG_SIZE(n),                                                       \
		.bitrate = DT_INST_PROP(n, clock_frequency),                                       \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.irq_config_func = i2c_irq_config_func_##n,                                        \
		.pwr_func = pwr_on_ambiq_i2c_##n};                                                 \
	I2C_DEVICE_DT_INST_DEFINE(n, i2c_ambiq_init, NULL, &i2c_ambiq_data##n,                     \
				  &i2c_ambiq_config##n, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,     \
				  &i2c_ambiq_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AMBIQ_I2C_DEFINE)
