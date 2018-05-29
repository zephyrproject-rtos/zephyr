/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <device.h>
#include <i2c.h>
#include <ioapic.h>
#include <power.h>

#include "qm_i2c.h"
#include "qm_isr.h"
#include "clk.h"
#include "soc.h"

#include "i2c-priv.h"

/* Convenient macros to get the controller instance and the driver data. */
#define GET_CONTROLLER_INSTANCE(dev) \
	(((const struct i2c_qmsi_config_info *) \
	  dev->config->config_info)->instance)
#define GET_DRIVER_DATA(dev) \
	((struct i2c_qmsi_driver_data *)dev->driver_data)

struct i2c_qmsi_config_info {
	qm_i2c_t instance; /* Controller instance. */
	u32_t bitrate;
	clk_periph_t clock_gate;
};

static int i2c_qmsi_init(struct device *dev);

struct i2c_qmsi_driver_data {
	struct k_sem device_sync_sem;
	int transfer_status;
	struct k_sem sem;
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	u32_t device_power_state;
	qm_i2c_context_t i2c_ctx;
#endif
};

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static void i2c_qmsi_set_power_state(struct device *dev, u32_t power_state)
{
	struct i2c_qmsi_driver_data *drv_data = dev->driver_data;

	drv_data->device_power_state = power_state;
}

static u32_t i2c_qmsi_get_power_state(struct device *dev)
{
	struct i2c_qmsi_driver_data *drv_data = dev->driver_data;

	return drv_data->device_power_state;
}

static int i2c_suspend_device(struct device *dev)
{
	if (device_busy_check(dev)) {
		return -EBUSY;
	}

	struct i2c_qmsi_driver_data *drv_data = GET_DRIVER_DATA(dev);

	qm_i2c_save_context(GET_CONTROLLER_INSTANCE(dev), &drv_data->i2c_ctx);

	i2c_qmsi_set_power_state(dev, DEVICE_PM_SUSPEND_STATE);

	return 0;
}

static int i2c_resume_device_from_suspend(struct device *dev)
{
	struct i2c_qmsi_driver_data *drv_data = GET_DRIVER_DATA(dev);

	qm_i2c_restore_context(GET_CONTROLLER_INSTANCE(dev),
			       &drv_data->i2c_ctx);

	i2c_qmsi_set_power_state(dev, DEVICE_PM_ACTIVE_STATE);

	return 0;
}

/*
* Implements the driver control management functionality
* the *context may include IN data or/and OUT data
*/
static int i2c_device_ctrl(struct device *dev, u32_t ctrl_command,
			   void *context)
{
	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		if (*((u32_t *)context) == DEVICE_PM_SUSPEND_STATE) {
			return i2c_suspend_device(dev);
		} else if (*((u32_t *)context) == DEVICE_PM_ACTIVE_STATE) {
			return i2c_resume_device_from_suspend(dev);
		}
	} else if (ctrl_command == DEVICE_PM_GET_POWER_STATE) {
		*((u32_t *)context) = i2c_qmsi_get_power_state(dev);
		return 0;
	}

	return 0;
}
#else
#define i2c_qmsi_set_power_state(...)
#endif /* CONFIG_DEVICE_POWER_MANAGEMENT */

#ifdef CONFIG_I2C_0

static struct i2c_qmsi_driver_data driver_data_0;

static const struct i2c_qmsi_config_info config_info_0 = {
	.instance = QM_I2C_0,
	.bitrate = CONFIG_I2C_0_BITRATE,
	.clock_gate = CLK_PERIPH_I2C_M0_REGISTER | CLK_PERIPH_CLK,
};

DEVICE_DEFINE(i2c_0, CONFIG_I2C_0_NAME, &i2c_qmsi_init,
	      i2c_device_ctrl, &driver_data_0, &config_info_0, POST_KERNEL,
	      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

#endif /* CONFIG_I2C_0 */

#ifdef CONFIG_I2C_1

static struct i2c_qmsi_driver_data driver_data_1;

static const struct i2c_qmsi_config_info config_info_1 = {
	.instance = QM_I2C_1,
	.bitrate = CONFIG_I2C_1_BITRATE,
	.clock_gate = CLK_PERIPH_I2C_M1_REGISTER | CLK_PERIPH_CLK,
};

DEVICE_DEFINE(i2c_1, CONFIG_I2C_1_NAME, &i2c_qmsi_init,
	      i2c_device_ctrl, &driver_data_1, &config_info_1, POST_KERNEL,
	      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

#endif /* CONFIG_I2C_1 */

static int i2c_qmsi_configure(struct device *dev, u32_t config)
{
	qm_i2c_t instance = GET_CONTROLLER_INSTANCE(dev);
	struct i2c_qmsi_driver_data *driver_data = GET_DRIVER_DATA(dev);
	qm_i2c_reg_t *const controller = QM_I2C[instance];
	int rc;
	qm_i2c_config_t qm_cfg;

	/* This driver only supports master mode. */
	if (!(I2C_MODE_MASTER & config))
		return -EINVAL;

	qm_cfg.mode = QM_I2C_MASTER;
	if (I2C_ADDR_10_BITS & config) {
		qm_cfg.address_mode = QM_I2C_10_BIT;
	} else {
		qm_cfg.address_mode = QM_I2C_7_BIT;
	}

	switch (I2C_SPEED_GET(config)) {
	case I2C_SPEED_STANDARD:
		qm_cfg.speed = QM_I2C_SPEED_STD;
		break;
	case I2C_SPEED_FAST:
		qm_cfg.speed = QM_I2C_SPEED_FAST;
		break;
	case I2C_SPEED_FAST_PLUS:
		qm_cfg.speed = QM_I2C_SPEED_FAST_PLUS;
		break;
	default:
		return -EINVAL;
	}

	k_sem_take(&driver_data->sem, K_FOREVER);
	rc = qm_i2c_set_config(instance, &qm_cfg);
	k_sem_give(&driver_data->sem);

	controller->ic_sda_hold = (CONFIG_I2C_SDA_RX_HOLD << 16) +
				   CONFIG_I2C_SDA_TX_HOLD;

	controller->ic_sda_setup = CONFIG_I2C_SDA_SETUP;

	return rc;
}

static void transfer_complete(void *data, int rc, qm_i2c_status_t status,
			 u32_t len)
{
	struct device *dev = (struct device *) data;
	struct i2c_qmsi_driver_data *driver_data;

	driver_data = GET_DRIVER_DATA(dev);
	driver_data->transfer_status = rc;
	k_sem_give(&driver_data->device_sync_sem);
}

static int i2c_qmsi_transfer(struct device *dev, struct i2c_msg *msgs,
			     u8_t num_msgs, u16_t addr)
{
	struct i2c_qmsi_driver_data *driver_data = GET_DRIVER_DATA(dev);
	qm_i2c_t instance = GET_CONTROLLER_INSTANCE(dev);
	int rc;

	__ASSERT_NO_MSG(msgs);
	if (!num_msgs) {
		return 0;
	}

	device_busy_set(dev);

	for (int i = 0; i < num_msgs; i++) {
		u8_t op =  msgs[i].flags & I2C_MSG_RW_MASK;
		bool stop = (msgs[i].flags & I2C_MSG_STOP) == I2C_MSG_STOP;
		qm_i2c_transfer_t xfer = { 0 };
		if (op == I2C_MSG_WRITE) {
			xfer.tx = msgs[i].buf;
			xfer.tx_len = msgs[i].len;
		} else {
			xfer.rx = msgs[i].buf;
			xfer.rx_len = msgs[i].len;
		}

		xfer.callback = transfer_complete;
		xfer.callback_data = dev;
		xfer.stop = stop;

		k_sem_take(&driver_data->sem, K_FOREVER);
		rc = qm_i2c_master_irq_transfer(instance, &xfer, addr);
		k_sem_give(&driver_data->sem);

		if (rc != 0) {
			device_busy_clear(dev);
			return -EIO;
		}

		/* Block current thread until the I2C transfer completes. */
		k_sem_take(&driver_data->device_sync_sem, K_FOREVER);

		if (driver_data->transfer_status != 0) {
			device_busy_clear(dev);
			return -EIO;
		}
	}

	device_busy_clear(dev);

	return 0;
}

static const struct i2c_driver_api api = {
	.configure = i2c_qmsi_configure,
	.transfer = i2c_qmsi_transfer,
};

static int i2c_qmsi_init(struct device *dev)
{
	struct i2c_qmsi_driver_data *driver_data = GET_DRIVER_DATA(dev);
	const struct i2c_qmsi_config_info *config = dev->config->config_info;
	qm_i2c_t instance = GET_CONTROLLER_INSTANCE(dev);
	u32_t bitrate_cfg;
	int err;

	k_sem_init(&driver_data->device_sync_sem, 0, UINT_MAX);
	k_sem_init(&driver_data->sem, 1, UINT_MAX);

	switch (instance) {
	case QM_I2C_0:
		/* Register interrupt handler, unmask IRQ and route it
		 * to Lakemont core.
		 */
		IRQ_CONNECT(CONFIG_I2C_0_IRQ,
			    CONFIG_I2C_0_IRQ_PRI, qm_i2c_0_irq_isr, NULL,
			    CONFIG_I2C_0_IRQ_FLAGS);
		irq_enable(CONFIG_I2C_0_IRQ);
		QM_IR_UNMASK_INTERRUPTS(
				QM_INTERRUPT_ROUTER->i2c_master_0_int_mask);
		break;

#ifdef CONFIG_I2C_1
	case QM_I2C_1:
		IRQ_CONNECT(CONFIG_I2C_1_IRQ,
			    CONFIG_I2C_1_IRQ_PRI, qm_i2c_1_irq_isr, NULL,
			    CONFIG_I2C_1_IRQ_FLAGS);
		irq_enable(CONFIG_I2C_1_IRQ);
		QM_IR_UNMASK_INTERRUPTS(
				QM_INTERRUPT_ROUTER->i2c_master_1_int_mask);
		break;
#endif /* CONFIG_I2C_1 */

	default:
		return -EIO;
	}

	clk_periph_enable(config->clock_gate);

	bitrate_cfg = _i2c_map_dt_bitrate(config->bitrate);

	err = i2c_qmsi_configure(dev, I2C_MODE_MASTER | bitrate_cfg);
	if (err < 0) {
		return err;
	}

	dev->driver_api = &api;

	i2c_qmsi_set_power_state(dev, DEVICE_PM_ACTIVE_STATE);

	return 0;
}
