/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <device.h>
#include <i2c.h>
#include <board.h>

#include "qm_ss_i2c.h"
#include "qm_ss_isr.h"
#include "ss_clk.h"

/* Convenient macros to get the controller instance and the driver data. */
#define GET_CONTROLLER_INSTANCE(dev) \
	(((const struct i2c_qmsi_ss_config_info *) \
	  dev->config->config_info)->instance)
#define GET_DRIVER_DATA(dev) \
	((struct i2c_qmsi_ss_driver_data *)dev->driver_data)

struct i2c_qmsi_ss_config_info {
	qm_ss_i2c_t instance; /* Controller instance. */
	union dev_config default_cfg;
	void (*irq_cfg)(void);
};

struct i2c_qmsi_ss_driver_data {
	device_sync_call_t sync;
	int transfer_status;
	struct k_sem sem;
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	uint32_t device_power_state;
#ifdef CONFIG_SYS_POWER_DEEP_SLEEP
	qm_ss_i2c_context_t i2c_ctx;
#endif
#endif
};

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static void ss_i2c_qmsi_set_power_state(struct device *dev,
					uint32_t power_state)
{
	struct i2c_qmsi_ss_driver_data *drv_data = GET_DRIVER_DATA(dev);

	drv_data->device_power_state = power_state;
}

static uint32_t ss_i2c_qmsi_get_power_state(struct device *dev)
{
	struct i2c_qmsi_ss_driver_data *drv_data = GET_DRIVER_DATA(dev);

	return drv_data->device_power_state;
}

#ifdef CONFIG_SYS_POWER_DEEP_SLEEP
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
#endif /* CONFIG_SYS_POWER_DEEP_SLEEP */

/*
* Implements the driver control management functionality
* the *context may include IN data or/and OUT data
*/
static int ss_i2c_device_ctrl(struct device *dev, uint32_t ctrl_command,
			      void *context)
{
	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
#ifdef CONFIG_SYS_POWER_DEEP_SLEEP
		if (*((uint32_t *)context) == DEVICE_PM_SUSPEND_STATE) {
			return ss_i2c_suspend_device(dev);
		} else if (*((uint32_t *)context) == DEVICE_PM_ACTIVE_STATE) {
			return ss_i2c_resume_device_from_suspend(dev);
		}
#endif /* CONFIG_SYS_POWER_DEEP_SLEEP */
	} else if (ctrl_command == DEVICE_PM_GET_POWER_STATE) {
		*((uint32_t *)context) = ss_i2c_qmsi_get_power_state(dev);
	}

	return 0;
}
#else
#define ss_i2c_qmsi_set_power_state(...)
#endif /* CONFIG_DEVICE_POWER_MANAGEMENT */

static int i2c_qmsi_ss_init(struct device *dev);

static void i2c_qmsi_ss_isr(void *arg)
{
	struct device *dev = arg;
	qm_ss_i2c_t instance = GET_CONTROLLER_INSTANCE(dev);

	if (instance == QM_SS_I2C_0) {
		qm_ss_i2c_0_isr(NULL);
	} else {
		qm_ss_i2c_1_isr(NULL);
	}
}

#ifdef CONFIG_I2C_0

static struct i2c_qmsi_ss_driver_data driver_data_0;

static void i2c_qmsi_ss_config_irq_0(void);

static const struct i2c_qmsi_ss_config_info config_info_0 = {
	.instance = QM_SS_I2C_0,
	.default_cfg.raw = CONFIG_I2C_0_DEFAULT_CFG,
	.irq_cfg = i2c_qmsi_ss_config_irq_0,
};

DEVICE_DEFINE(i2c_ss_0, CONFIG_I2C_0_NAME, i2c_qmsi_ss_init,
	      ss_i2c_device_ctrl, &driver_data_0, &config_info_0, POST_KERNEL,
	      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

static void i2c_qmsi_ss_config_irq_0(void)
{
	uint32_t mask = 0;

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
	IRQ_CONNECT(I2C_SS_0_ERR_VECTOR, 1, i2c_qmsi_ss_isr,
		    DEVICE_GET(i2c_ss_0), 0);
	IRQ_CONNECT(I2C_SS_0_RX_VECTOR, 1, i2c_qmsi_ss_isr,
		    DEVICE_GET(i2c_ss_0), 0);
	IRQ_CONNECT(I2C_SS_0_TX_VECTOR, 1, i2c_qmsi_ss_isr,
		    DEVICE_GET(i2c_ss_0), 0);
	IRQ_CONNECT(I2C_SS_0_STOP_VECTOR, 1, i2c_qmsi_ss_isr,
		    DEVICE_GET(i2c_ss_0), 0);

	irq_enable(I2C_SS_0_ERR_VECTOR);
	irq_enable(I2C_SS_0_RX_VECTOR);
	irq_enable(I2C_SS_0_TX_VECTOR);
	irq_enable(I2C_SS_0_STOP_VECTOR);
}
#endif /* CONFIG_I2C_0 */

#ifdef CONFIG_I2C_1

static struct i2c_qmsi_ss_driver_data driver_data_1;

static void i2c_qmsi_ss_config_irq_1(void);

static const struct i2c_qmsi_ss_config_info config_info_1 = {
	.instance = QM_SS_I2C_1,
	.default_cfg.raw = CONFIG_I2C_1_DEFAULT_CFG,
	.irq_cfg = i2c_qmsi_ss_config_irq_1,
};

DEVICE_DEFINE(i2c_ss_1, CONFIG_I2C_1_NAME, i2c_qmsi_ss_init,
	      ss_i2c_device_ctrl, &driver_data_1, &config_info_1, POST_KERNEL,
	      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

static void i2c_qmsi_ss_config_irq_1(void)
{
	uint32_t mask = 0;

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
	IRQ_CONNECT(I2C_SS_1_ERR_VECTOR, 1, i2c_qmsi_ss_isr,
		    DEVICE_GET(i2c_ss_1), 0);
	IRQ_CONNECT(I2C_SS_1_RX_VECTOR, 1, i2c_qmsi_ss_isr,
		    DEVICE_GET(i2c_ss_1), 0);
	IRQ_CONNECT(I2C_SS_1_TX_VECTOR, 1, i2c_qmsi_ss_isr,
		    DEVICE_GET(i2c_ss_1), 0);
	IRQ_CONNECT(I2C_SS_1_STOP_VECTOR, 1, i2c_qmsi_ss_isr,
		    DEVICE_GET(i2c_ss_1), 0);

	irq_enable(I2C_SS_1_ERR_VECTOR);
	irq_enable(I2C_SS_1_RX_VECTOR);
	irq_enable(I2C_SS_1_TX_VECTOR);
	irq_enable(I2C_SS_1_STOP_VECTOR);
}
#endif /* CONFIG_I2C_1 */

static int i2c_qmsi_ss_configure(struct device *dev, uint32_t config)
{
	qm_ss_i2c_t instance = GET_CONTROLLER_INSTANCE(dev);
	struct i2c_qmsi_ss_driver_data *driver_data = GET_DRIVER_DATA(dev);
	union dev_config cfg;
	qm_ss_i2c_config_t qm_cfg;
	uint32_t i2c_base = QM_SS_I2C_0_BASE;

	cfg.raw = config;

	/* This driver only supports master mode. */
	if (!cfg.bits.is_master_device) {
		return -EINVAL;
	}

	qm_cfg.address_mode = (cfg.bits.use_10_bit_addr) ? QM_SS_I2C_10_BIT :
							   QM_SS_I2C_7_BIT;

	switch (cfg.bits.speed) {
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

	__builtin_arc_sr(((CONFIG_I2C_SDA_SETUP << 16) +
			   CONFIG_I2C_SS_SDA_HOLD),
			 (i2c_base + QM_SS_I2C_SDA_CONFIG));

	return 0;
}

static void transfer_complete(void *data, int rc, qm_ss_i2c_status_t status,
			uint32_t len)
{
	struct device *dev = data;
	struct i2c_qmsi_ss_driver_data *driver_data;

	driver_data = GET_DRIVER_DATA(dev);
	driver_data->transfer_status = rc;
	device_sync_call_complete(&driver_data->sync);
}

static int i2c_qmsi_ss_transfer(struct device *dev, struct i2c_msg *msgs,
			     uint8_t num_msgs, uint16_t addr)
{
	struct i2c_qmsi_ss_driver_data *driver_data = GET_DRIVER_DATA(dev);
	qm_ss_i2c_t instance = GET_CONTROLLER_INSTANCE(dev);
	int rc;

	if  (msgs == NULL || num_msgs == 0) {
		return -ENOTSUP;
	}

	for (int i = 0; i < num_msgs; i++) {
		uint8_t *buf = msgs[i].buf;
		uint32_t len = msgs[i].len;
		uint8_t op =  msgs[i].flags & I2C_MSG_RW_MASK;
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
			return -EIO;
		}

		/* Block current thread until the I2C transfer completes. */
		device_sync_call_wait(&driver_data->sync);

		if (driver_data->transfer_status != 0) {
			return -EIO;
		}
	}

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
	int err;

	config->irq_cfg();
	ss_clk_i2c_enable(instance);

	k_sem_init(&driver_data->sem, 0, UINT_MAX);
	k_sem_give(&driver_data->sem);

	err = i2c_qmsi_ss_configure(dev, config->default_cfg.raw);

	if (err < 0) {
		return err;
	}

	device_sync_call_init(&driver_data->sync);
	dev->driver_api = &api;

	ss_i2c_qmsi_set_power_state(dev, DEVICE_PM_ACTIVE_STATE);

	return 0;
}
