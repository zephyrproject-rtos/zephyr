/*
 * Copyright (c) 2019 Brett Witherspoon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc13xx_cc26xx_i2c

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_cc13xx_cc26xx);

#include <driverlib/i2c.h>
#include <driverlib/prcm.h>

#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26X2.h>

#include "i2c-priv.h"

struct i2c_cc13xx_cc26xx_data {
	struct k_sem lock;
	struct k_sem complete;
	volatile uint32_t error;
#ifdef CONFIG_PM
	Power_NotifyObj postNotify;
	uint32_t dev_config;
#endif
};

struct i2c_cc13xx_cc26xx_config {
	uint32_t base;
	const struct pinctrl_dev_config *pcfg;
};

static int i2c_cc13xx_cc26xx_transmit(const struct device *dev,
				      const uint8_t *buf,
				      uint32_t len, uint16_t addr)
{
	const struct i2c_cc13xx_cc26xx_config *config = dev->config;
	const uint32_t base = config->base;
	struct i2c_cc13xx_cc26xx_data *data = dev->data;

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

static int i2c_cc13xx_cc26xx_receive(const struct device *dev, uint8_t *buf,
				     uint32_t len,
				     uint16_t addr)
{
	const struct i2c_cc13xx_cc26xx_config *config = dev->config;
	const uint32_t base = config->base;
	struct i2c_cc13xx_cc26xx_data *data = dev->data;

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

static int i2c_cc13xx_cc26xx_transfer(const struct device *dev,
				      struct i2c_msg *msgs,
				      uint8_t num_msgs, uint16_t addr)
{
	struct i2c_cc13xx_cc26xx_data *data = dev->data;
	int ret = 0;

	if (num_msgs == 0) {
		return 0;
	}

	k_sem_take(&data->lock, K_FOREVER);

	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);

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

	pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);

	k_sem_give(&data->lock);

	return ret;
}

#define CPU_FREQ DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency)
static int i2c_cc13xx_cc26xx_configure(const struct device *dev,
				       uint32_t dev_config)
{
	const struct i2c_cc13xx_cc26xx_config *config = dev->config;
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
	if (!(dev_config & I2C_MODE_CONTROLLER)) {
		LOG_ERR("Slave mode is not supported");
		return -EIO;
	}

	/* This is deprecated and could be ignored in the future */
	if (dev_config & I2C_ADDR_10_BITS) {
		LOG_ERR("10-bit addressing mode is not supported");
		return -EIO;
	}

	/* Enables and configures I2C master */
	I2CMasterInitExpClk(config->base, CPU_FREQ, fast);

#ifdef CONFIG_PM
	struct i2c_cc13xx_cc26xx_data *data = dev->data;

	data->dev_config = dev_config;
#endif

	return 0;
}

static void i2c_cc13xx_cc26xx_isr(const struct device *dev)
{
	const struct i2c_cc13xx_cc26xx_config *config = dev->config;
	struct i2c_cc13xx_cc26xx_data *data = dev->data;
	const uint32_t base = config->base;

	if (I2CMasterIntStatus(base, true)) {
		I2CMasterIntClear(base);

		data->error = I2CMasterErr(base);

		k_sem_give(&data->complete);
	}
}

#ifdef CONFIG_PM
/*
 *  ======== postNotifyFxn ========
 *  Called by Power module when waking up the CPU from Standby. The i2c needs
 *  to be reconfigured afterwards, unless Zephyr's device PM turned it off, in
 *  which case it'd be responsible for turning it back on and reconfigure it.
 */
static int postNotifyFxn(unsigned int eventType, uintptr_t eventArg,
	uintptr_t clientArg)
{
	const struct device *dev = (const struct device *)clientArg;
	const struct i2c_cc13xx_cc26xx_config *config = dev->config;
	struct i2c_cc13xx_cc26xx_data *data = dev->data;
	int ret = Power_NOTIFYDONE;
	int16_t res_id;

	/* Reconfigure the hardware if returning from sleep */
	if (eventType == PowerCC26XX_AWAKE_STANDBY) {
		res_id = PowerCC26XX_PERIPH_I2C0;

		if (Power_getDependencyCount(res_id) != 0) {
			/* Reconfigure and enable I2C only if powered */
			if (i2c_cc13xx_cc26xx_configure(dev,
				data->dev_config) != 0) {
				ret = Power_NOTIFYERROR;
			}

			I2CMasterIntEnable(config->base);
		}
	}

	return (ret);
}
#endif

#ifdef CONFIG_PM_DEVICE
static int i2c_cc13xx_cc26xx_pm_action(const struct device *dev,
				       enum pm_device_action action)
{
	const struct i2c_cc13xx_cc26xx_config *config = dev->config;
	struct i2c_cc13xx_cc26xx_data *data = dev->data;
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		Power_setDependency(PowerCC26XX_PERIPH_I2C0);
		ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			return ret;
		}
		ret = i2c_cc13xx_cc26xx_configure(dev, data->dev_config);
		if (ret == 0) {
			I2CMasterIntEnable(config->base);
		}
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		I2CMasterIntDisable(config->base);
		I2CMasterDisable(config->base);
		/* Reset pin type to default GPIO configuration */
		ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_SLEEP);
		if (ret < 0) {
			return ret;
		}
		Power_releaseDependency(PowerCC26XX_PERIPH_I2C0);
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

static int i2c_cc13xx_cc26xx_init(const struct device *dev)
{
	const struct i2c_cc13xx_cc26xx_config *config = dev->config;
	uint32_t cfg;
	int err;

#ifdef CONFIG_PM
	struct i2c_cc13xx_cc26xx_data *data = dev->data;

	/* Set Power dependencies & constraints */
	Power_setDependency(PowerCC26XX_PERIPH_I2C0);

	/* Register notification function */
	Power_registerNotify(&data->postNotify,
		PowerCC26XX_AWAKE_STANDBY,
		postNotifyFxn, (uintptr_t)dev);
#else
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
#endif

	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    i2c_cc13xx_cc26xx_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

	err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		LOG_ERR("Failed to configure pinctrl state");
		return err;
	}

	cfg = i2c_map_dt_bitrate(DT_INST_PROP(0, clock_frequency));
	err = i2c_cc13xx_cc26xx_configure(dev, cfg | I2C_MODE_CONTROLLER);
	if (err) {
		LOG_ERR("Failed to configure");
		return err;
	}

	I2CMasterIntEnable(config->base);

	return 0;
}

static const struct i2c_driver_api i2c_cc13xx_cc26xx_driver_api = {
	.configure = i2c_cc13xx_cc26xx_configure,
	.transfer = i2c_cc13xx_cc26xx_transfer
};

PINCTRL_DT_INST_DEFINE(0);

static const struct i2c_cc13xx_cc26xx_config i2c_cc13xx_cc26xx_config = {
	.base = DT_INST_REG_ADDR(0),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
};

static struct i2c_cc13xx_cc26xx_data i2c_cc13xx_cc26xx_data = {
	.lock = Z_SEM_INITIALIZER(i2c_cc13xx_cc26xx_data.lock, 1, 1),
	.complete = Z_SEM_INITIALIZER(i2c_cc13xx_cc26xx_data.complete, 0, 1),
	.error = I2C_MASTER_ERR_NONE
};

PM_DEVICE_DT_INST_DEFINE(0, i2c_cc13xx_cc26xx_pm_action);

I2C_DEVICE_DT_INST_DEFINE(0,
		i2c_cc13xx_cc26xx_init,
		PM_DEVICE_DT_INST_GET(0),
		&i2c_cc13xx_cc26xx_data, &i2c_cc13xx_cc26xx_config,
		POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,
		&i2c_cc13xx_cc26xx_driver_api);
