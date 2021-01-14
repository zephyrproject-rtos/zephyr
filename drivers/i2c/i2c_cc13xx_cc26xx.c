/*
 * Copyright (c) 2019 Brett Witherspoon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc13xx_cc26xx_i2c

#include <kernel.h>
#include <drivers/i2c.h>
#include <power/power.h>

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(i2c_cc13xx_cc26xx);

#include <driverlib/i2c.h>
#include <driverlib/ioc.h>
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
#ifdef CONFIG_PM_DEVICE
	uint32_t pm_state;
#endif
};

struct i2c_cc13xx_cc26xx_config {
	uint32_t base;
	uint32_t scl_pin;
	uint32_t sda_pin;
};

static inline struct i2c_cc13xx_cc26xx_data *get_dev_data(const struct device *dev)
{
	return dev->data;
}

static inline const struct i2c_cc13xx_cc26xx_config *
get_dev_config(const struct device *dev)
{
	return dev->config;
}

static int i2c_cc13xx_cc26xx_transmit(const struct device *dev,
				      const uint8_t *buf,
				      uint32_t len, uint16_t addr)
{
	const uint32_t base = get_dev_config(dev)->base;
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

static int i2c_cc13xx_cc26xx_receive(const struct device *dev, uint8_t *buf,
				     uint32_t len,
				     uint16_t addr)
{
	const uint32_t base = get_dev_config(dev)->base;
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

static int i2c_cc13xx_cc26xx_transfer(const struct device *dev,
				      struct i2c_msg *msgs,
				      uint8_t num_msgs, uint16_t addr)
{
	int ret = 0;

	if (num_msgs == 0) {
		return 0;
	}

	k_sem_take(&get_dev_data(dev)->lock, K_FOREVER);

#if defined(CONFIG_PM) && \
	defined(CONFIG_PM_SLEEP_STATES)
	pm_ctrl_disable_state(POWER_STATE_SLEEP_2);
#endif

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

#if defined(CONFIG_PM) && \
	defined(CONFIG_PM_SLEEP_STATES)
	pm_ctrl_enable_state(POWER_STATE_SLEEP_2);
#endif

	k_sem_give(&get_dev_data(dev)->lock);

	return ret;
}

#define CPU_FREQ DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency)
static int i2c_cc13xx_cc26xx_configure(const struct device *dev,
				       uint32_t dev_config)
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
	I2CMasterInitExpClk(get_dev_config(dev)->base, CPU_FREQ, fast);

#ifdef CONFIG_PM
	get_dev_data(dev)->dev_config = dev_config;
#endif

	return 0;
}

static void i2c_cc13xx_cc26xx_isr(const void *arg)
{
	const uint32_t base = get_dev_config(arg)->base;
	struct i2c_cc13xx_cc26xx_data *data = get_dev_data(arg);

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
	int ret = Power_NOTIFYDONE;
	int16_t res_id;

	/* Reconfigure the hardware if returning from sleep */
	if (eventType == PowerCC26XX_AWAKE_STANDBY) {
		res_id = PowerCC26XX_PERIPH_I2C0;

		if (Power_getDependencyCount(res_id) != 0) {
			/* Reconfigure and enable I2C only if powered */
			if (i2c_cc13xx_cc26xx_configure(dev,
				get_dev_data(dev)->dev_config) != 0) {
				ret = Power_NOTIFYERROR;
			}

			I2CMasterIntEnable(get_dev_config(dev)->base);
		}
	}

	return (ret);
}
#endif

#ifdef CONFIG_PM_DEVICE
static int i2c_cc13xx_cc26xx_set_power_state(const struct device *dev,
					     uint32_t new_state)
{
	int ret = 0;

	if ((new_state == DEVICE_PM_ACTIVE_STATE) &&
		(new_state != get_dev_data(dev)->pm_state)) {
		Power_setDependency(PowerCC26XX_PERIPH_I2C0);
		IOCPinTypeI2c(get_dev_config(dev)->base,
			get_dev_config(dev)->sda_pin,
			get_dev_config(dev)->scl_pin);
		ret = i2c_cc13xx_cc26xx_configure(dev,
			get_dev_data(dev)->dev_config);
		if (ret == 0) {
			I2CMasterIntEnable(get_dev_config(dev)->base);
			get_dev_data(dev)->pm_state = new_state;
		}
	} else {
		__ASSERT_NO_MSG(new_state == DEVICE_PM_LOW_POWER_STATE ||
			new_state == DEVICE_PM_SUSPEND_STATE ||
			new_state == DEVICE_PM_OFF_STATE);

		if (get_dev_data(dev)->pm_state == DEVICE_PM_ACTIVE_STATE) {
			I2CMasterIntDisable(get_dev_config(dev)->base);
			I2CMasterDisable(get_dev_config(dev)->base);
			/* Reset pin type to default GPIO configuration */
			IOCPortConfigureSet(get_dev_config(dev)->scl_pin,
				IOC_PORT_GPIO, IOC_STD_OUTPUT);
			IOCPortConfigureSet(get_dev_config(dev)->sda_pin,
				IOC_PORT_GPIO, IOC_STD_OUTPUT);
			Power_releaseDependency(PowerCC26XX_PERIPH_I2C0);
			get_dev_data(dev)->pm_state = new_state;
		}
	}

	return ret;
}

static int i2c_cc13xx_cc26xx_pm_control(const struct device *dev,
					uint32_t ctrl_command,
					void *context, device_pm_cb cb,
					void *arg)
{
	int ret = 0;

	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		uint32_t new_state = *((const uint32_t *)context);

		if (new_state != get_dev_data(dev)->pm_state) {
			ret = i2c_cc13xx_cc26xx_set_power_state(dev,
				new_state);
		}
	} else {
		__ASSERT_NO_MSG(ctrl_command == DEVICE_PM_GET_POWER_STATE);
		*((uint32_t *)context) = get_dev_data(dev)->pm_state;
	}

	if (cb) {
		cb(dev, ret, context, arg);
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

static int i2c_cc13xx_cc26xx_init(const struct device *dev)
{
	uint32_t cfg;
	int err;

#ifdef CONFIG_PM_DEVICE
	get_dev_data(dev)->pm_state = DEVICE_PM_ACTIVE_STATE;
#endif

#ifdef CONFIG_PM
	/* Set Power dependencies & constraints */
	Power_setDependency(PowerCC26XX_PERIPH_I2C0);

	/* Register notification function */
	Power_registerNotify(&get_dev_data(dev)->postNotify,
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

	/* Configure IOC module to route SDA and SCL signals */
	IOCPinTypeI2c(get_dev_config(dev)->base, get_dev_config(dev)->sda_pin,
		      get_dev_config(dev)->scl_pin);

	cfg = i2c_map_dt_bitrate(DT_INST_PROP(0, clock_frequency));
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
	.base = DT_INST_REG_ADDR(0),
	.sda_pin = DT_INST_PROP(0, sda_pin),
	.scl_pin = DT_INST_PROP(0, scl_pin)
};

static struct i2c_cc13xx_cc26xx_data i2c_cc13xx_cc26xx_data = {
	.lock = Z_SEM_INITIALIZER(i2c_cc13xx_cc26xx_data.lock, 1, 1),
	.complete = Z_SEM_INITIALIZER(i2c_cc13xx_cc26xx_data.complete, 0, 1),
	.error = I2C_MASTER_ERR_NONE
};

DEVICE_DT_INST_DEFINE(0,
		i2c_cc13xx_cc26xx_init,
		i2c_cc13xx_cc26xx_pm_control,
		&i2c_cc13xx_cc26xx_data, &i2c_cc13xx_cc26xx_config,
		POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,
		&i2c_cc13xx_cc26xx_driver_api);
