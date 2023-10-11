/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT altr_nios2_i2c

#include <errno.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/sys/util.h>
#include <altera_common.h>
#include "altera_avalon_i2c.h"

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_nios2);

#define NIOS2_I2C_TIMEOUT_USEC		1000

struct i2c_nios2_data {
	ALT_AVALON_I2C_DEV_t i2c_dev;
	IRQ_DATA_t irq_data;
	struct k_sem sem_lock;
};

static int
i2c_nios2_configure(const struct device *dev, uint32_t dev_config)
{
	struct i2c_nios2_data *data = (struct i2c_nios2_data *)dev->data;

	int32_t rc = 0;

	k_sem_take(&data->sem_lock, K_FOREVER);
	if (!(I2C_MODE_CONTROLLER & dev_config)) {
		LOG_ERR("i2c config mode error\n");
		rc = -EINVAL;
		goto i2c_cfg_err;
	}

	if (I2C_ADDR_10_BITS & dev_config) {
		LOG_ERR("i2c config addressing error\n");
		rc = -EINVAL;
		goto i2c_cfg_err;
	}

	if (I2C_SPEED_GET(dev_config) != I2C_SPEED_STANDARD) {
		LOG_ERR("i2c config speed error\n");
		rc = -EINVAL;
		goto i2c_cfg_err;
	}

	alt_avalon_i2c_init(&data->i2c_dev);

i2c_cfg_err:
	k_sem_give(&data->sem_lock);
	return rc;
}

static int i2c_nios2_transfer(const struct device *dev, struct i2c_msg *msgs,
			      uint8_t num_msgs, uint16_t addr)
{
	struct i2c_nios2_data *data = (struct i2c_nios2_data *)dev->data;
	ALT_AVALON_I2C_STATUS_CODE status;
	uint32_t restart, stop;
	int32_t i, timeout, rc = 0;

	k_sem_take(&data->sem_lock, K_FOREVER);
	/* register the optional interrupt callback */
	alt_avalon_i2c_register_optional_irq_handler(
			&data->i2c_dev, &data->irq_data);

	/* Iterate over all the messages */
	for (i = 0; i < num_msgs; i++) {

		/* convert restart flag */
		if (msgs->flags & I2C_MSG_RESTART) {
			restart = ALT_AVALON_I2C_RESTART;
		} else {
			restart = ALT_AVALON_I2C_NO_RESTART;
		}

		/* convert stop flag */
		if (msgs->flags & I2C_MSG_STOP) {
			stop = ALT_AVALON_I2C_STOP;
		} else {
			stop = ALT_AVALON_I2C_NO_STOP;
		}

		/* Set the slave device address */
		alt_avalon_i2c_master_target_set(&data->i2c_dev, addr);

		/* Start the transfer */
		if (msgs->flags & I2C_MSG_READ) {
			status = alt_avalon_i2c_master_receive_using_interrupts(
							&data->i2c_dev,
							msgs->buf, msgs->len,
							restart, stop);
		} else {
			status = alt_avalon_i2c_master_transmit_using_interrupts
							(&data->i2c_dev,
							msgs->buf, msgs->len,
							restart, stop);
		}

		/* Return an error if the transfer didn't
		 * start successfully e.g., if the bus was busy
		 */
		if (status != ALT_AVALON_I2C_SUCCESS) {
			LOG_ERR("i2c transfer error %lu\n", status);
			rc = -EIO;
			goto i2c_transfer_err;
		}

		timeout = NIOS2_I2C_TIMEOUT_USEC;
		while (timeout) {
			k_busy_wait(1);
			status = alt_avalon_i2c_interrupt_transaction_status(
							&data->i2c_dev);
			if (status == ALT_AVALON_I2C_SUCCESS) {
				break;
			}
			timeout--;
		}

		if (timeout <= 0) {
			LOG_ERR("i2c busy or timeout error %lu\n", status);
			rc = -EIO;
			goto i2c_transfer_err;
		}

		/* move to the next message */
		msgs++;
	}

i2c_transfer_err:
	alt_avalon_i2c_disable(&data->i2c_dev);
	k_sem_give(&data->sem_lock);
	return rc;
}

static void i2c_nios2_isr(const struct device *dev)
{
	struct i2c_nios2_data *data = (struct i2c_nios2_data *)dev->data;

	/* Call Altera HAL driver ISR */
	alt_handle_irq(&data->i2c_dev, DT_INST_IRQN(0));
}

static int i2c_nios2_init(const struct device *dev);

static struct i2c_driver_api i2c_nios2_driver_api = {
	.configure = i2c_nios2_configure,
	.transfer = i2c_nios2_transfer,
};

static struct i2c_nios2_data i2c_nios2_dev_data = {
	.i2c_dev = {
		.i2c_base = (alt_u32 *)DT_INST_REG_ADDR(0),
		.irq_controller_ID = I2C_0_IRQ_INTERRUPT_CONTROLLER_ID,
		.irq_ID = DT_INST_IRQN(0),
		.ip_freq_in_hz = DT_INST_PROP(0, clock_frequency),
	},
};

static int i2c_nios2_init(const struct device *dev)
{
	struct i2c_nios2_data *data = (struct i2c_nios2_data *)dev->data;
	int rc;

	/* initialize semaphore */
	k_sem_init(&data->sem_lock, 1, 1);

	rc = i2c_nios2_configure(dev,
			I2C_MODE_CONTROLLER |
			I2C_SPEED_SET(I2C_SPEED_STANDARD));
	if (rc) {
		LOG_ERR("i2c configure failed %d\n", rc);
		return rc;
	}

	/* clear ISR register content */
	alt_avalon_i2c_int_clear(&data->i2c_dev,
			ALT_AVALON_I2C_ISR_ALL_CLEARABLE_INTS_MSK);
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
			i2c_nios2_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));
	return 0;
}

I2C_DEVICE_DT_INST_DEFINE(0, i2c_nios2_init, NULL,
		    &i2c_nios2_dev_data, NULL,
		    POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,
		    &i2c_nios2_driver_api);
