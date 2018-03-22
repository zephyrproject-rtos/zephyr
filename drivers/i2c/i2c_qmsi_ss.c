/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <device.h>
#include <i2c.h>
#include <board.h>

#include "qm_ss_i2c.h"
#include "qm_ss_isr.h"
#include "ss_clk.h"

#include "i2c-priv.h"

/* Convenient macros to get the controller instance and the driver data. */
#define GET_CONTROLLER_INSTANCE(dev) \
	(((const struct i2c_qmsi_ss_config_info *) \
	  dev->config->config_info)->instance)
#define GET_DRIVER_DATA(dev) \
	((struct i2c_qmsi_ss_driver_data *)dev->driver_data)

struct i2c_qmsi_ss_config_info {
	qm_ss_i2c_t instance; /* Controller instance. */
	u32_t bitrate;
	void (*irq_cfg)(void);
};

struct i2c_qmsi_ss_driver_data {
	struct k_sem device_sync_sem;
	int transfer_status;
	struct k_sem sem;
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	u32_t device_power_state;
	qm_ss_i2c_context_t i2c_ctx;
#endif
};

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static void ss_i2c_qmsi_set_power_state(struct device *dev,
					u32_t power_state)
{
	struct i2c_qmsi_ss_driver_data *drv_data = GET_DRIVER_DATA(dev);

	drv_data->device_power_state = power_state;
}

static u32_t ss_i2c_qmsi_get_power_state(struct device *dev)
{
	struct i2c_qmsi_ss_driver_data *drv_data = GET_DRIVER_DATA(dev);

	return drv_data->device_power_state;
}

static int ss_i2c_suspend_device(struct device *dev)
{
	if (device_busy_check(dev)) {
		return -EBUSY;
	}

	struct i2c_qmsi_ss_driver_data *drv_data = GET_DRIVER_DATA(dev);

	qm_ss_i2c_save_context(GET_CONTROLLER_INSTANCE(dev),
			       &drv_data->i2c_ctx);

	ss_i2c_qmsi_set_power_state(dev, DEVICE_PM_SUSPEND_STATE);

	return 0;
}

static int ss_i2c_resume_device_from_suspend(struct device *dev)
{
	struct i2c_qmsi_ss_driver_data *drv_data = GET_DRIVER_DATA(dev);

	qm_ss_i2c_restore_context(GET_CONTROLLER_INSTANCE(dev),
				  &drv_data->i2c_ctx);

	ss_i2c_qmsi_set_power_state(dev, DEVICE_PM_ACTIVE_STATE);

	return 0;
}

/*
* Implements the driver control management functionality
* the *context may include IN data or/and OUT data
*/
static int ss_i2c_device_ctrl(struct device *dev, u32_t ctrl_command,
			      void *context)
{
	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		if (*((u32_t *)context) == DEVICE_PM_SUSPEND_STATE) {
			return ss_i2c_suspend_device(dev);
		} else if (*((u32_t *)context) == DEVICE_PM_ACTIVE_STATE) {
			return ss_i2c_resume_device_from_suspend(dev);
		}
	} else if (ctrl_command == DEVICE_PM_GET_POWER_STATE) {
		*((u32_t *)context) = ss_i2c_qmsi_get_power_state(dev);
	}

	return 0;
}
#else
#define ss_i2c_qmsi_set_power_state(...)
#endif /* CONFIG_DEVICE_POWER_MANAGEMENT */

static int i2c_qmsi_ss_init(struct device *dev);

#ifdef CONFIG_I2C_SS_0

static struct i2c_qmsi_ss_driver_data driver_data_0;

static void i2c_qmsi_ss_config_irq_0(void);

static const struct i2c_qmsi_ss_config_info config_info_0 = {
	.instance = QM_SS_I2C_0,
	.bitrate = CONFIG_I2C_SS_0_BITRATE,
	.irq_cfg = i2c_qmsi_ss_config_irq_0,
};

DEVICE_DEFINE(i2c_ss_0, CONFIG_I2C_SS_0_NAME, i2c_qmsi_ss_init,
	      ss_i2c_device_ctrl, &driver_data_0, &config_info_0, POST_KERNEL,
	      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

static void i2c_qmsi_ss_config_irq_0(void)
{
	u32_t mask = 0;

	/* Need to unmask the interrupts in System Control Subsystem (SCSS)
	 * so the interrupt controller can route these interrupts to
	 * the sensor subsystem.
	 */
	mask = sys_read32(SCSS_REGISTER_BASE + I2C_SS_0_ERR_MASK);
	mask &= INT_ENABLE_ARC;
	sys_write32(mask, SCSS_REGISTER_BASE + I2C_SS_0_ERR_MASK);

	mask = sys_read32(SCSS_REGISTER_BASE + I2C_SS_0_TX_MASK);
	mask &= INT_ENABLE_ARC;
	sys_write32(mask, SCSS_REGISTER_BASE + I2C_SS_0_TX_MASK);

	mask = sys_read32(SCSS_REGISTER_BASE + I2C_SS_0_RX_MASK);
	mask &= INT_ENABLE_ARC;
	sys_write32(mask, SCSS_REGISTER_BASE + I2C_SS_0_RX_MASK);

	mask = sys_read32(SCSS_REGISTER_BASE + I2C_SS_0_STOP_MASK);
	mask &= INT_ENABLE_ARC;
	sys_write32(mask, SCSS_REGISTER_BASE + I2C_SS_0_STOP_MASK);

	/* Connect the IRQs to ISR */
	IRQ_CONNECT(CONFIG_I2C_SS_0_ERR_IRQ, CONFIG_I2C_SS_0_ERR_IRQ_PRI,
		    qm_ss_i2c_0_error_isr, DEVICE_GET(i2c_ss_0), 0);
	IRQ_CONNECT(CONFIG_I2C_SS_0_RX_IRQ, CONFIG_I2C_SS_0_RX_IRQ_PRI,
		    qm_ss_i2c_0_rx_avail_isr, DEVICE_GET(i2c_ss_0), 0);
	IRQ_CONNECT(CONFIG_I2C_SS_0_TX_IRQ, CONFIG_I2C_SS_0_TX_IRQ_PRI,
		    qm_ss_i2c_0_tx_req_isr, DEVICE_GET(i2c_ss_0), 0);
	IRQ_CONNECT(CONFIG_I2C_SS_0_STOP_IRQ, CONFIG_I2C_SS_0_STOP_IRQ_PRI,
		    qm_ss_i2c_0_stop_det_isr, DEVICE_GET(i2c_ss_0), 0);

	irq_enable(CONFIG_I2C_SS_0_ERR_IRQ);
	irq_enable(CONFIG_I2C_SS_0_RX_IRQ);
	irq_enable(CONFIG_I2C_SS_0_TX_IRQ);
	irq_enable(CONFIG_I2C_SS_0_STOP_IRQ);
}
#endif /* CONFIG_I2C_SS_0 */

#ifdef CONFIG_I2C_SS_1

static struct i2c_qmsi_ss_driver_data driver_data_1;

static void i2c_qmsi_ss_config_irq_1(void);

static const struct i2c_qmsi_ss_config_info config_info_1 = {
	.instance = QM_SS_I2C_1,
	.bitrate = CONFIG_I2C_SS_1_BITRATE,
	.irq_cfg = i2c_qmsi_ss_config_irq_1,
};

DEVICE_DEFINE(i2c_ss_1, CONFIG_I2C_SS_1_NAME, i2c_qmsi_ss_init,
	      ss_i2c_device_ctrl, &driver_data_1, &config_info_1, POST_KERNEL,
	      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

static void i2c_qmsi_ss_config_irq_1(void)
{
	u32_t mask = 0;

	/* Need to unmask the interrupts in System Control Subsystem (SCSS)
	 * so the interrupt controller can route these interrupts to
	 * the sensor subsystem.
	 */
	mask = sys_read32(SCSS_REGISTER_BASE + I2C_SS_1_ERR_MASK);
	mask &= INT_ENABLE_ARC;
	sys_write32(mask, SCSS_REGISTER_BASE + I2C_SS_1_ERR_MASK);

	mask = sys_read32(SCSS_REGISTER_BASE + I2C_SS_1_TX_MASK);
	mask &= INT_ENABLE_ARC;
	sys_write32(mask, SCSS_REGISTER_BASE + I2C_SS_1_TX_MASK);

	mask = sys_read32(SCSS_REGISTER_BASE + I2C_SS_1_RX_MASK);
	mask &= INT_ENABLE_ARC;
	sys_write32(mask, SCSS_REGISTER_BASE + I2C_SS_1_RX_MASK);

	mask = sys_read32(SCSS_REGISTER_BASE + I2C_SS_1_STOP_MASK);
	mask &= INT_ENABLE_ARC;
	sys_write32(mask, SCSS_REGISTER_BASE + I2C_SS_1_STOP_MASK);

	/* Connect the IRQs to ISR */
	IRQ_CONNECT(CONFIG_I2C_SS_1_ERR_IRQ, CONFIG_I2C_SS_1_ERR_IRQ_PRI,
		    qm_ss_i2c_1_error_isr, DEVICE_GET(i2c_ss_1), 0);
	IRQ_CONNECT(CONFIG_I2C_SS_1_RX_IRQ, CONFIG_I2C_SS_1_RX_IRQ_PRI,
		    qm_ss_i2c_1_rx_avail_isr, DEVICE_GET(i2c_ss_1), 0);
	IRQ_CONNECT(CONFIG_I2C_SS_1_TX_IRQ, CONFIG_I2C_SS_1_TX_IRQ_PRI,
		    qm_ss_i2c_1_tx_req_isr, DEVICE_GET(i2c_ss_1), 0);
	IRQ_CONNECT(CONFIG_I2C_SS_1_STOP_IRQ, CONFIG_I2C_SS_1_STOP_IRQ_PRI,
		    qm_ss_i2c_1_stop_det_isr, DEVICE_GET(i2c_ss_1), 0);

	irq_enable(CONFIG_I2C_SS_1_ERR_IRQ);
	irq_enable(CONFIG_I2C_SS_1_RX_IRQ);
	irq_enable(CONFIG_I2C_SS_1_TX_IRQ);
	irq_enable(CONFIG_I2C_SS_1_STOP_IRQ);
}
#endif /* CONFIG_I2C_SS_1 */

static int i2c_qmsi_ss_configure(struct device *dev, u32_t config)
{
	qm_ss_i2c_t instance = GET_CONTROLLER_INSTANCE(dev);
	struct i2c_qmsi_ss_driver_data *driver_data = GET_DRIVER_DATA(dev);
	qm_ss_i2c_config_t qm_cfg;
	u32_t i2c_base = QM_SS_I2C_0_BASE;

	/* This driver only supports master mode. */
	if (!(I2C_MODE_MASTER & config)) {
		return -EINVAL;
	}

	if (I2C_ADDR_10_BITS & config) {
		qm_cfg.address_mode = QM_SS_I2C_10_BIT;
	} else {
		qm_cfg.address_mode = QM_SS_I2C_7_BIT;
	}

	switch (I2C_SPEED_GET(config)) {
	case I2C_SPEED_STANDARD:
		qm_cfg.speed = QM_SS_I2C_SPEED_STD;
		break;
	case I2C_SPEED_FAST:
		qm_cfg.speed = QM_SS_I2C_SPEED_FAST;
		break;
	default:
		return -EINVAL;
	}

	k_sem_take(&driver_data->sem, K_FOREVER);
	if (qm_ss_i2c_set_config(instance, &qm_cfg) != 0) {
		k_sem_give(&driver_data->sem);
		return -EIO;
	}
	k_sem_give(&driver_data->sem);

	if (instance == QM_SS_I2C_1) {
		i2c_base = QM_SS_I2C_1_BASE;
	}

	__builtin_arc_sr(((CONFIG_I2C_SS_SDA_SETUP << 16) +
			   CONFIG_I2C_SS_SDA_HOLD),
			 (i2c_base + QM_SS_I2C_SDA_CONFIG));

	return 0;
}

static void transfer_complete(void *data, int rc, qm_ss_i2c_status_t status,
			uint32_t len)
{
	struct device *dev = data;
	struct i2c_qmsi_ss_driver_data *driver_data;

	ARG_UNUSED(status);
	ARG_UNUSED(len);

	driver_data = GET_DRIVER_DATA(dev);
	driver_data->transfer_status = rc;
	k_sem_give(&driver_data->device_sync_sem);
}

static int i2c_qmsi_ss_transfer(struct device *dev, struct i2c_msg *msgs,
			     u8_t num_msgs, u16_t addr)
{
	struct i2c_qmsi_ss_driver_data *driver_data = GET_DRIVER_DATA(dev);
	qm_ss_i2c_t instance = GET_CONTROLLER_INSTANCE(dev);
	int rc;

	__ASSERT_NO_MSG(msgs);
	if (!num_msgs) {
		return 0;
	}

	device_busy_set(dev);

	for (int i = 0; i < num_msgs; i++) {
		u8_t *buf = msgs[i].buf;
		u32_t len = msgs[i].len;
		u8_t op =  msgs[i].flags & I2C_MSG_RW_MASK;
		bool stop = (msgs[i].flags & I2C_MSG_STOP) == I2C_MSG_STOP;
		qm_ss_i2c_transfer_t xfer = { 0 };

		if (op == I2C_MSG_WRITE) {
			xfer.tx = buf;
			xfer.tx_len = len;
		} else {
			xfer.rx = buf;
			xfer.rx_len = len;
		}

		xfer.callback = transfer_complete;
		xfer.callback_data = dev;
		xfer.stop = stop;

		k_sem_take(&driver_data->sem, K_FOREVER);
		rc = qm_ss_i2c_master_irq_transfer(instance, &xfer, addr);
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
	.configure = i2c_qmsi_ss_configure,
	.transfer = i2c_qmsi_ss_transfer,
};

static int i2c_qmsi_ss_init(struct device *dev)
{
	struct i2c_qmsi_ss_driver_data *driver_data = GET_DRIVER_DATA(dev);
	const struct i2c_qmsi_ss_config_info *config = dev->config->config_info;
	qm_ss_i2c_t instance = GET_CONTROLLER_INSTANCE(dev);
	u32_t bitrate_cfg;
	int err;

	config->irq_cfg();
	ss_clk_i2c_enable(instance);

	k_sem_init(&driver_data->sem, 1, UINT_MAX);

	bitrate_cfg = _i2c_map_dt_bitrate(config->bitrate);

	err = i2c_qmsi_ss_configure(dev, I2C_MODE_MASTER | bitrate_cfg);

	if (err < 0) {
		return err;
	}

	k_sem_init(&driver_data->device_sync_sem, 0, UINT_MAX);
	dev->driver_api = &api;

	ss_i2c_qmsi_set_power_state(dev, DEVICE_PM_ACTIVE_STATE);

	return 0;
}
