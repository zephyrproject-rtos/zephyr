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
#include <ioapic.h>
#include <power.h>

#include "qm_i2c.h"
#include "qm_isr.h"
#include "clk.h"

/* Convenient macros to get the controller instance and the driver data. */
#define GET_CONTROLLER_INSTANCE(dev) \
	(((struct i2c_qmsi_config_info *)dev->config->config_info)->instance)
#define GET_DRIVER_DATA(dev) \
	((struct i2c_qmsi_driver_data *)dev->driver_data)

struct i2c_qmsi_config_info {
	qm_i2c_t instance; /* Controller instance. */
	union dev_config default_cfg;
	clk_periph_t clock_gate;
};

struct i2c_context_t {
	uint32_t ic_con; /**< Control Register. */
	uint32_t ic_fs_spklen; /**< SS and FS Spike Suppression Limit. */
	uint32_t ic_hs_spklen; /**< HS Spike Suppression Limit. */
	uint32_t ic_ss_scl_lcnt; /**< Standard Speed Clock SCL Low Count. */
	uint32_t ic_ss_scl_hcnt; /**< Standard Speed Clock SCL High Count. */
	uint32_t ic_fs_scl_lcnt; /**< Fast Speed Clock SCL Low Count. */
	uint32_t ic_fs_scl_hcnt; /**< Fast Speed Clock SCL High Count. */
	uint32_t ic_hs_scl_lcnt; /**< High Speed Clock SCL Low Count. */
	uint32_t ic_hs_scl_hcnt; /**< High Speed Clock SCL High Count. */
	uint32_t ic_intr_mask; /**< Interrupt Mask. */
	uint32_t ic_sda_hold; /**< SDA Hold. */
	uint32_t ic_sda_setup; /**< SDA Setup. */
	uint32_t ic_ack_general_call; /**< General Call Ack. */
	uint32_t ic_sar; /**< Slave Address. */
	uint32_t ic_dma_cr; /**< DMA Control Register. */
	uint32_t ic_dma_tdlr; /**< DMA Transmit Data Level Register. */
	uint32_t ic_dma_rdlr; /**< DMA Receive Data Level Register. */
	uint32_t int_i2c_mst_mask; /**< Interrupt Mask. */
};

static int i2c_qmsi_init(struct device *dev);

struct i2c_qmsi_driver_data {
	device_sync_call_t sync;
	int transfer_status;
	struct nano_sem sem;
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	struct i2c_context_t ctx_save;
#endif
};

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static int i2c_suspend_device(struct device *dev, int pm_policy)
{
	if (device_busy_check(dev)) {
		return -EBUSY;
	}

	if (pm_policy == SYS_PM_DEEP_SLEEP) {
		qm_i2c_t instance = GET_CONTROLLER_INSTANCE(dev);
		qm_i2c_reg_t *const regs = QM_I2C[instance];
		struct i2c_qmsi_driver_data *drv_data = dev->driver_data;
		struct i2c_context_t *const ctx_save = &drv_data->ctx_save;

		ctx_save->ic_con = regs->ic_con;
		ctx_save->ic_fs_spklen = regs->ic_fs_spklen;
		ctx_save->ic_hs_spklen = regs->ic_hs_spklen;
		ctx_save->ic_ss_scl_lcnt = regs->ic_ss_scl_lcnt;
		ctx_save->ic_ss_scl_hcnt = regs->ic_ss_scl_hcnt;
		ctx_save->ic_fs_scl_lcnt = regs->ic_fs_scl_lcnt;
		ctx_save->ic_fs_scl_hcnt = regs->ic_fs_scl_hcnt;
		ctx_save->ic_hs_scl_lcnt = regs->ic_hs_scl_lcnt;
		ctx_save->ic_hs_scl_hcnt = regs->ic_hs_scl_hcnt;
		ctx_save->ic_intr_mask = regs->ic_intr_mask;
		ctx_save->ic_sda_hold = regs->ic_sda_hold;
		ctx_save->ic_sda_setup = regs->ic_sda_setup;
		ctx_save->ic_ack_general_call = regs->ic_ack_general_call;
		ctx_save->ic_sar = regs->ic_sar;
		ctx_save->ic_dma_cr = regs->ic_dma_cr;
		ctx_save->ic_dma_tdlr = regs->ic_dma_tdlr;
		ctx_save->ic_dma_rdlr = regs->ic_dma_rdlr;

		if (instance == QM_I2C_0) {
			ctx_save->int_i2c_mst_mask =
				QM_SCSS_INT->int_i2c_mst_0_mask;
		} else {
			ctx_save->int_i2c_mst_mask =
				QM_SCSS_INT->int_i2c_mst_1_mask;
		}

	}

	return 0;
}

static int i2c_resume_device(struct device *dev, int pm_policy)
{
	if (pm_policy == SYS_PM_DEEP_SLEEP) {
		struct i2c_qmsi_config_info *config = dev->config->config_info;
		qm_i2c_reg_t *const regs = QM_I2C[config->instance];
		struct i2c_qmsi_driver_data *drv_data = dev->driver_data;
		struct i2c_context_t *const ctx_save = &drv_data->ctx_save;

		regs->ic_con = ctx_save->ic_con;
		regs->ic_fs_spklen = ctx_save->ic_fs_spklen;
		regs->ic_hs_spklen = ctx_save->ic_hs_spklen;
		regs->ic_ss_scl_lcnt = ctx_save->ic_ss_scl_lcnt;
		regs->ic_ss_scl_hcnt = ctx_save->ic_ss_scl_hcnt;
		regs->ic_fs_scl_lcnt = ctx_save->ic_fs_scl_lcnt;
		regs->ic_fs_scl_hcnt = ctx_save->ic_fs_scl_hcnt;
		regs->ic_hs_scl_lcnt = ctx_save->ic_hs_scl_lcnt;
		regs->ic_hs_scl_hcnt = ctx_save->ic_hs_scl_hcnt;
		regs->ic_intr_mask = ctx_save->ic_intr_mask;
		regs->ic_sda_hold = ctx_save->ic_sda_hold;
		regs->ic_sda_setup = ctx_save->ic_sda_setup;
		regs->ic_ack_general_call = ctx_save->ic_ack_general_call;
		regs->ic_sar = ctx_save->ic_sar;
		regs->ic_dma_cr = ctx_save->ic_dma_cr;
		regs->ic_dma_tdlr = ctx_save->ic_dma_tdlr;
		regs->ic_dma_rdlr = ctx_save->ic_dma_rdlr;

		if (config->instance == QM_I2C_0) {
			QM_SCSS_INT->int_i2c_mst_0_mask =
				ctx_save->int_i2c_mst_mask;
		} else {
			QM_SCSS_INT->int_i2c_mst_1_mask =
				ctx_save->int_i2c_mst_mask;
		}

	}

	return 0;
}
#endif /* CONFIG_DEVICE_POWER_MANAGEMENT */

DEFINE_DEVICE_PM_OPS(i2c, i2c_suspend_device, i2c_resume_device);

#ifdef CONFIG_I2C_0

static struct i2c_qmsi_driver_data driver_data_0;

static struct i2c_qmsi_config_info config_info_0 = {
	.instance = QM_I2C_0,
	.default_cfg.raw = CONFIG_I2C_0_DEFAULT_CFG,
	.clock_gate = CLK_PERIPH_I2C_M0_REGISTER | CLK_PERIPH_CLK,
};

DEVICE_INIT_PM(i2c_0, CONFIG_I2C_0_NAME, &i2c_qmsi_init,
	       DEVICE_PM_OPS_GET(i2c), &driver_data_0, &config_info_0,
	       SECONDARY, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

#endif /* CONFIG_I2C_0 */

#ifdef CONFIG_I2C_1

static struct i2c_qmsi_driver_data driver_data_1;

static struct i2c_qmsi_config_info config_info_1 = {
	.instance = QM_I2C_1,
	.default_cfg.raw = CONFIG_I2C_1_DEFAULT_CFG,
	.clock_gate = CLK_PERIPH_I2C_M1_REGISTER | CLK_PERIPH_CLK,
};

DEVICE_INIT_PM(i2c_1, CONFIG_I2C_1_NAME, &i2c_qmsi_init,
	       DEVICE_PM_OPS_GET(i2c), &driver_data_1, &config_info_1,
	       SECONDARY, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

#endif /* CONFIG_I2C_1 */

static int i2c_qmsi_configure(struct device *dev, uint32_t config)
{
	qm_i2c_t instance = GET_CONTROLLER_INSTANCE(dev);
	struct i2c_qmsi_driver_data *driver_data = GET_DRIVER_DATA(dev);
	qm_i2c_reg_t *const controller = QM_I2C[instance];
	union dev_config cfg;
	int rc;
	qm_i2c_config_t qm_cfg;

	cfg.raw = config;

	/* This driver only supports master mode. */
	if (!cfg.bits.is_master_device)
		return -EINVAL;

	qm_cfg.mode = QM_I2C_MASTER;
	qm_cfg.address_mode = (cfg.bits.use_10_bit_addr) ? QM_I2C_10_BIT :
							   QM_I2C_7_BIT;

	switch (cfg.bits.speed) {
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

	nano_sem_take(&driver_data->sem, TICKS_UNLIMITED);
	rc = qm_i2c_set_config(instance, &qm_cfg);
	nano_sem_give(&driver_data->sem);

	controller->ic_sda_hold = (CONFIG_I2C_SDA_RX_HOLD << 16) +
				   CONFIG_I2C_SDA_TX_HOLD;

	controller->ic_sda_setup = CONFIG_I2C_SDA_SETUP;

	return rc;
}

static void transfer_complete(void *data, int rc, qm_i2c_status_t status,
			 uint32_t len)
{
	struct device *dev = (struct device *) data;
	struct i2c_qmsi_driver_data *driver_data;

	driver_data = GET_DRIVER_DATA(dev);
	driver_data->transfer_status = rc;
	device_sync_call_complete(&driver_data->sync);
}

static int i2c_qmsi_transfer(struct device *dev, struct i2c_msg *msgs,
			     uint8_t num_msgs, uint16_t addr)
{
	struct i2c_qmsi_driver_data *driver_data = GET_DRIVER_DATA(dev);
	qm_i2c_t instance = GET_CONTROLLER_INSTANCE(dev);
	int rc;

	if  (msgs == NULL || num_msgs == 0) {
		return -ENOTSUP;
	}

	device_busy_set(dev);

	for (int i = 0; i < num_msgs; i++) {
		uint8_t op =  msgs[i].flags & I2C_MSG_RW_MASK;
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

		nano_sem_take(&driver_data->sem, TICKS_UNLIMITED);
		rc = qm_i2c_master_irq_transfer(instance, &xfer, addr);
		nano_sem_give(&driver_data->sem);

		if (rc != 0) {
			device_busy_clear(dev);
			return -EIO;
		}

		/* Block current thread until the I2C transfer completes. */
		device_sync_call_wait(&driver_data->sync);

		if (driver_data->transfer_status != 0) {
			device_busy_clear(dev);
			return -EIO;
		}
	}

	device_busy_clear(dev);

	return 0;
}

static struct i2c_driver_api api = {
	.configure = i2c_qmsi_configure,
	.transfer = i2c_qmsi_transfer,
};

static int i2c_qmsi_init(struct device *dev)
{
	struct i2c_qmsi_driver_data *driver_data = GET_DRIVER_DATA(dev);
	struct i2c_qmsi_config_info *config = dev->config->config_info;
	qm_i2c_t instance = GET_CONTROLLER_INSTANCE(dev);
	int err;

	device_sync_call_init(&driver_data->sync);
	nano_sem_init(&driver_data->sem);
	nano_sem_give(&driver_data->sem);

	switch (instance) {
	case QM_I2C_0:
		/* Register interrupt handler, unmask IRQ and route it
		 * to Lakemont core.
		 */
		IRQ_CONNECT(QM_IRQ_I2C_0,
			    CONFIG_I2C_0_IRQ_PRI, qm_i2c_0_isr, NULL,
			    (IOAPIC_LEVEL | IOAPIC_HIGH));
		irq_enable(QM_IRQ_I2C_0);
		QM_SCSS_INT->int_i2c_mst_0_mask &= ~BIT(0);
		break;

#ifdef CONFIG_I2C_1
	case QM_I2C_1:
		IRQ_CONNECT(QM_IRQ_I2C_1,
			    CONFIG_I2C_1_IRQ_PRI, qm_i2c_1_isr, NULL,
			    (IOAPIC_LEVEL | IOAPIC_HIGH));
		irq_enable(QM_IRQ_I2C_1);
		QM_SCSS_INT->int_i2c_mst_1_mask &= ~BIT(0);

		break;
#endif /* CONFIG_I2C_1 */

	clk_periph_enable(config->clock_gate);

	default:
		return -EIO;
	}

	err = i2c_qmsi_configure(dev, config->default_cfg.raw);
	if (err < 0) {
		return err;
	}

	dev->driver_api = &api;
	return 0;
}
