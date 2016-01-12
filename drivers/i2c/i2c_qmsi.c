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

#include <device.h>
#include <i2c.h>
#include <init.h>

#include "qm_i2c.h"
#include "qm_scss.h"

static int i2c_qmsi_configure(struct device *dev, uint32_t config)
{
	union dev_config cfg;
	qm_i2c_config_t qm_cfg;

	cfg.raw = config;

	/* This driver only supports master mode. */
	if (!cfg.bits.is_master_device)
		return DEV_INVALID_CONF;

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
	case QM_I2C_SPEED_FAST_PLUS:
		qm_cfg.speed = QM_I2C_SPEED_FAST_PLUS;
		break;
	default:
		return DEV_INVALID_CONF;
	}

	if (qm_i2c_set_config(QM_I2C_0, &qm_cfg) != QM_RC_OK) {
		clk_periph_disable(CLK_PERIPH_I2C_M0_REGISTER);
		return DEV_FAIL;
	}

	return DEV_OK;
}

/* FIXME: This function is implemented using QMSI polling APIs since
 * the interrupt API (qm_i2c_master_irq_transfer) seems not to be
 * working properly. Once this issue is solved, we should change this
 * function to use the interrupt API and sleep the current thread while
 * the I2C transfer is carried out.
 */
static int i2c_qmsi_transfer(struct device *dev, struct i2c_msg *msgs,
			     uint8_t num_msgs, uint16_t addr)
{
	qm_rc_t rc;

	if (qm_i2c_get_status(QM_I2C_0) != QM_I2C_IDLE)
		return DEV_USED;

	if  (msgs == NULL || num_msgs == 0)
		return DEV_INVALID_OP;

	for (int i = 0; i < num_msgs; i++) {
		uint8_t *buf = msgs[i].buf;
		uint32_t len = msgs[i].len;
		uint8_t op =  msgs[i].flags & I2C_MSG_RW_MASK;
		bool stop = (msgs[i].flags & I2C_MSG_STOP) == I2C_MSG_STOP;

		rc = (op == I2C_MSG_WRITE) ?
			qm_i2c_master_write(QM_I2C_0, addr, buf, len, stop) :
			qm_i2c_master_read(QM_I2C_0, addr, buf, len, stop);

		if (rc != QM_RC_OK)
			return DEV_FAIL;
	}

	return DEV_OK;
}

static int i2c_qmsi_suspend(struct device *dev)
{
	return DEV_NO_SUPPORT;
}

static int i2c_qmsi_resume(struct device *dev)
{
	return DEV_NO_SUPPORT;
}

static struct i2c_driver_api api = {
	.configure = i2c_qmsi_configure,
	.transfer = i2c_qmsi_transfer,
	.suspend = i2c_qmsi_suspend,
	.resume = i2c_qmsi_resume,
};

static int i2c_qmsi_init(struct device *dev)
{
	clk_periph_enable(CLK_PERIPH_I2C_M0_REGISTER);

	dev->driver_api = &api;
	return 0;
}

DECLARE_DEVICE_INIT_CONFIG(i2c_qmsi_0, CONFIG_I2C_QMSI_0_NAME, i2c_qmsi_init,
			   NULL);

SYS_DEFINE_DEVICE(i2c_qmsi_0, 0, SECONDARY, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
