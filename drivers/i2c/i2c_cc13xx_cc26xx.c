/*
 * Copyright (c) 2019 Brett Witherspoon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <drivers/i2c.h>

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(i2c_cc13xx_cc26xx);

#include <driverlib/i2c.h>
#include <driverlib/ioc.h>
#include <driverlib/prcm.h>

#include "i2c-priv.h"

DEVICE_DECLARE(i2c_cc13xx_cc26xx);

struct i2c_cc13xx_cc26xx_data {
	struct k_sem lock;
	struct k_sem complete;
	volatile u32_t error;
};

struct i2c_cc13xx_cc26xx_config {
	u32_t base;
	u32_t scl_pin;
	u32_t sda_pin;
};

static inline struct i2c_cc13xx_cc26xx_data *get_dev_data(struct device *dev)
{
	return dev->driver_data;
}

static inline const struct i2c_cc13xx_cc26xx_config *
get_dev_config(struct device *dev)
{
	return dev->config->config_info;
}

static int i2c_cc13xx_cc26xx_transmit(struct device *dev, const u8_t *buf,
				      u32_t len, u16_t addr)
{
	const u32_t base = get_dev_config(dev)->base;
	struct i2c_cc13xx_cc26xx_data *data = get_dev_data(dev);

	/* Sending address without data is not supported */
	if (len == 0) {
		return -EIO;
	}

	I2CMasterSlaveAddrSet(base, addr, false);

	/* The following assumes a single master. Use I2CMasterBusBusy() if
	 * wanting to implement multiple master support.
	 */

	/* Single transmission */
	if (len == 1) {
		I2CMasterDataPut(base, *buf);

		I2CMasterControl(base, I2C_MASTER_CMD_SINGLE_SEND);

		k_sem_take(&data->complete, K_FOREVER);

		return data->error == I2C_MASTER_ERR_NONE ? 0 : -EIO;
	}

	/* Burst transmission */
	I2CMasterDataPut(base, buf[0]);

	I2CMasterControl(base, I2C_MASTER_CMD_BURST_SEND_START);

	k_sem_take(&data->complete, K_FOREVER);

	if (data->error != I2C_MASTER_ERR_NONE) {
		goto send_error_stop;
	}

	for (int i = 1; i < len - 1; i++) {
		I2CMasterDataPut(base, buf[i]);

		I2CMasterControl(base, I2C_MASTER_CMD_BURST_SEND_CONT);

		k_sem_take(&data->complete, K_FOREVER);

		if (data->error != I2C_MASTER_ERR_NONE) {
			goto send_error_stop;
		}
	}

	I2CMasterDataPut(base, buf[len - 1]);

	I2CMasterControl(base, I2C_MASTER_CMD_BURST_SEND_FINISH);

	k_sem_take(&data->complete, K_FOREVER);

	if (data->error != I2C_MASTER_ERR_NONE) {
		return -EIO;
	}

	return 0;

send_error_stop:
	I2CMasterControl(base, I2C_MASTER_CMD_BURST_SEND_ERROR_STOP);
	return -EIO;
}

static int i2c_cc13xx_cc26xx_receive(struct device *dev, u8_t *buf, u32_t len,
				     u16_t addr)
{
	const u32_t base = get_dev_config(dev)->base;
	struct i2c_cc13xx_cc26xx_data *data = get_dev_data(dev);

	/* Sending address without data is not supported */
	if (len == 0) {
		return -EIO;
	}

	I2CMasterSlaveAddrSet(base, addr, true);

	/* The following assumes a single master. Use I2CMasterBusBusy() if
	 * wanting to implement multiple master support.
	 */

	/* Single receive */
	if (len == 1) {
		I2CMasterControl(base, I2C_MASTER_CMD_SINGLE_RECEIVE);

		k_sem_take(&data->complete, K_FOREVER);

		if (data->error != I2C_MASTER_ERR_NONE) {
			return -EIO;
		}

		*buf = I2CMasterDataGet(base);

		return 0;
	}

	/* Burst receive */
	I2CMasterControl(base, I2C_MASTER_CMD_BURST_RECEIVE_START);

	k_sem_take(&data->complete, K_FOREVER);

	if (data->error != I2C_MASTER_ERR_NONE) {
		goto recv_error_stop;
	}

	buf[0] = I2CMasterDataGet(base);

	for (int i = 1; i < len - 1; i++) {
		I2CMasterControl(base, I2C_MASTER_CMD_BURST_RECEIVE_CONT);

		k_sem_take(&data->complete, K_FOREVER);

		if (data->error != I2C_MASTER_ERR_NONE) {
			goto recv_error_stop;
		}

		buf[i] = I2CMasterDataGet(base);
	}

	I2CMasterControl(base, I2C_MASTER_CMD_BURST_RECEIVE_FINISH);

	k_sem_take(&data->complete, K_FOREVER);

	if (data->error != I2C_MASTER_ERR_NONE) {
		return -EIO;
	}

	buf[len - 1] = I2CMasterDataGet(base);

	return 0;

recv_error_stop:
	I2CMasterControl(base, I2C_MASTER_CMD_BURST_RECEIVE_ERROR_STOP);
	return -EIO;
}

static int i2c_cc13xx_cc26xx_transfer(struct device *dev, struct i2c_msg *msgs,
				      u8_t num_msgs, u16_t addr)
{
	int ret = 0;

	if (num_msgs == 0) {
		return 0;
	}

	k_sem_take(&get_dev_data(dev)->lock, K_FOREVER);

	for (int i = 0; i < num_msgs; i++) {
		/* Not supported by hardware */
		if (msgs[i].flags & I2C_MSG_ADDR_10_BITS) {
			ret = -EIO;
			break;
		}

		if ((msgs[i].flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
			ret = i2c_cc13xx_cc26xx_transmit(dev, msgs[i].buf,
							 msgs[i].len, addr);
		} else {
			ret = i2c_cc13xx_cc26xx_receive(dev, msgs[i].buf,
							msgs[i].len, addr);
		}

		if (ret) {
			break;
		}
	}

	k_sem_give(&get_dev_data(dev)->lock);

	return ret;
}

static int i2c_cc13xx_cc26xx_configure(struct device *dev, u32_t dev_config)
{
	bool fast;

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		fast = false;
		break;
	case I2C_SPEED_FAST:
		fast = true;
		break;
	default:
		LOG_ERR("Unsupported speed");
		return -EIO;
	}

	/* Support for slave mode has not been implemented */
	if (!(dev_config & I2C_MODE_MASTER)) {
		LOG_ERR("Slave mode is not supported");
		return -EIO;
	}

	/* This is deprecated and could be ignored in the future */
	if (dev_config & I2C_ADDR_10_BITS) {
		LOG_ERR("10-bit addressing mode is not supported");
		return -EIO;
	}

	/* Enables and configures I2C master */
	I2CMasterInitExpClk(get_dev_config(dev)->base,
			    sys_clock_hw_cycles_per_sec(), fast);

	return 0;
}

static void i2c_cc13xx_cc26xx_isr(void *arg)
{
	const u32_t base = get_dev_config(arg)->base;
	struct i2c_cc13xx_cc26xx_data *data = get_dev_data(arg);

	if (I2CMasterIntStatus(base, true)) {
		I2CMasterIntClear(base);

		data->error = I2CMasterErr(base);

		k_sem_give(&data->complete);
	}
}

static int i2c_cc13xx_cc26xx_init(struct device *dev)
{
	u32_t cfg;
	int err;

	/* Enable I2C power domain */
	PRCMPowerDomainOn(PRCM_DOMAIN_SERIAL);

	/* Enable I2C peripheral clock */
	PRCMPeripheralRunEnable(PRCM_PERIPH_I2C0);
	/* Enable in sleep mode until proper power management is added */
	PRCMPeripheralSleepEnable(PRCM_PERIPH_I2C0);
	PRCMPeripheralDeepSleepEnable(PRCM_PERIPH_I2C0);

	/* Load PRCM settings */
	PRCMLoadSet();
	while (!PRCMLoadGet()) {
		continue;
	}

	/* I2C should not be accessed until power domain is on. */
	while (PRCMPowerDomainStatus(PRCM_DOMAIN_SERIAL) !=
	       PRCM_DOMAIN_POWER_ON) {
		continue;
	}

	IRQ_CONNECT(DT_INST_0_TI_CC13XX_CC26XX_I2C_IRQ_0,
		    DT_INST_0_TI_CC13XX_CC26XX_I2C_IRQ_0_PRIORITY,
		    i2c_cc13xx_cc26xx_isr, DEVICE_GET(i2c_cc13xx_cc26xx), 0);
	irq_enable(DT_INST_0_TI_CC13XX_CC26XX_I2C_IRQ_0);

	/* Configure IOC module to route SDA and SCL signals */
	IOCPinTypeI2c(get_dev_config(dev)->base, get_dev_config(dev)->sda_pin,
		      get_dev_config(dev)->scl_pin);

	cfg = i2c_map_dt_bitrate(DT_INST_0_TI_CC13XX_CC26XX_I2C_CLOCK_FREQUENCY);
	err = i2c_cc13xx_cc26xx_configure(dev, cfg | I2C_MODE_MASTER);
	if (err) {
		LOG_ERR("Failed to configure");
		return err;
	}

	I2CMasterIntEnable(get_dev_config(dev)->base);

	return 0;
}

static const struct i2c_driver_api i2c_cc13xx_cc26xx_driver_api = {
	.configure = i2c_cc13xx_cc26xx_configure,
	.transfer = i2c_cc13xx_cc26xx_transfer
};

static const struct i2c_cc13xx_cc26xx_config i2c_cc13xx_cc26xx_config = {
	.base = DT_INST_0_TI_CC13XX_CC26XX_I2C_BASE_ADDRESS,
	.sda_pin = DT_INST_0_TI_CC13XX_CC26XX_I2C_SDA_PIN,
	.scl_pin = DT_INST_0_TI_CC13XX_CC26XX_I2C_SCL_PIN
};

static struct i2c_cc13xx_cc26xx_data i2c_cc13xx_cc26xx_data = {
	.lock = Z_SEM_INITIALIZER(i2c_cc13xx_cc26xx_data.lock, 1, 1),
	.complete = Z_SEM_INITIALIZER(i2c_cc13xx_cc26xx_data.complete, 0, 1),
	.error = I2C_MASTER_ERR_NONE
};

DEVICE_AND_API_INIT(i2c_cc13xx_cc26xx, DT_INST_0_TI_CC13XX_CC26XX_I2C_LABEL,
		    i2c_cc13xx_cc26xx_init, &i2c_cc13xx_cc26xx_data,
		    &i2c_cc13xx_cc26xx_config, POST_KERNEL,
		    CONFIG_I2C_INIT_PRIORITY, &i2c_cc13xx_cc26xx_driver_api);
