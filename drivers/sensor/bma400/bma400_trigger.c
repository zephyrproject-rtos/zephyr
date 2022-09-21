/*
 * Copyright (c) 2022 Elektronikutvecklingsbyrån EUB AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bosch_bma400

#include <device.h>
#include <drivers/i2c.h>
#include <sys/util.h>
#include <kernel.h>
#include <drivers/sensor.h>
#include "bma400.h"

#include <logging/log.h>
LOG_MODULE_DECLARE(bma400, CONFIG_SENSOR_LOG_LEVEL);

static const struct gpio_dt_spec int1 = GPIO_DT_SPEC_INST_GET(0, int1_gpios);
static const struct gpio_dt_spec int2 = GPIO_DT_SPEC_INST_GET(0, int2_gpios);

static inline void setup_int1(bool enable)
{
	gpio_pin_interrupt_configure_dt(&int1,
			(enable ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE));
}

int bma400_attr_set(const struct device *dev,
		enum sensor_channel chan,
		enum sensor_attribute attr,
		const struct sensor_value *val)
{
	struct bma400_data *drv_data = dev->data;
	uint64_t slope_th;

	if (chan != SENSOR_CHAN_ACCEL_XYZ) {
		return -ENOTSUP;
	}

	if (attr == SENSOR_ATTR_SLOPE_TH) {
	} else if (attr == SENSOR_ATTR_LOWER_THRESH) {
		if (i2c_reg_write_byte(drv_data->i2c, drv_data->addr,
					BMA400_REG_GEN1INT_CONFIG0,
					BMA400_GEN1_ACT_X_EN |
					BMA400_GEN1_ACT_Y_EN |
					BMA400_GEN1_ACT_Z_EN |
					BMA400_GEN1_DATA_SRC_ACC_FILT1 |
					BMA400_GEN1_ACT_REFU_ONETIME) < 0) {
			LOG_ERR("Couldn't enable generic interrupt for XYZ");
			return -EIO;
		}

		if (i2c_reg_write_byte(drv_data->i2c, drv_data->addr,
					BMA400_REG_GEN1INT_CONFIG1,
					(1 << BMA400_GEN_INT_CRITERION_POS)) < 0) {
			LOG_ERR("Couldn't set threshold for interrupt");
			return -EIO;
		}

		uint8_t threshold_val = 1; // TODO: convert from input
		if (i2c_reg_write_byte(drv_data->i2c, drv_data->addr,
					BMA400_REG_GEN1INT_CONFIG2,
					threshold_val) < 0) {
			LOG_ERR("Couldn't set threshold value");
			return -EIO;
		}

		/*
		uint16_t duration = 0; // TODO: Use from other attribute
		uint8_t duration_msb = 0;
		uint8_t duration_lsb = 0;
		if (i2c_reg_write_byte(drv_data->i2c, drv_data->addr,
					BMA400_REG_GEN1INT_CONFIG3,
					duration_msb) < 0) {
			LOG_ERR("Couldn't set threshold duration (msb)");
			return -EIO;
		}
		if (i2c_reg_write_byte(drv_data->i2c, drv_data->addr,
					BMA400_REG_GEN1INT_CONFIG31,
					duration_lsb) < 0) {
			LOG_ERR("Couldn't set threshold duration (lsb)");
			return -EIO;
		}
		*/

	} else {
		return -ENOTSUP;
	}


	return 0;
}

static void bma400_gpio_callback(const struct device *dev,
		struct gpio_callback *cb, uint32_t pins)
{
	struct bma400_data *drv_data =
		CONTAINER_OF(cb, struct bma400_data, gpio_cb);

	ARG_UNUSED(pins);

	setup_int1(false);

#if defined(CONFIG_BMA400_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_BMA400_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

static void bma400_thread_cb(const struct device *dev)
{
	struct bma400_data *drv_data = dev->data;
	uint8_t status = 0U;
	int err = 0;

	/* check for data ready, and use its trigger if available */
	err = i2c_reg_read_byte(drv_data->i2c, drv_data->addr,
			BMA400_REG_INT_STAT0, &status);
	if (status & BMA400_EN_DRDY_MSK) {
		if (drv_data->data_ready_handler != NULL && err == 0) {
			drv_data->data_ready_handler(dev,
					&drv_data->data_ready_trigger);
		}
	}

	if (status & BMA400_EN_GEN1_MSK) {
		if (drv_data->any_motion_handler != NULL && err == 0) {
			drv_data->any_motion_handler(dev,
					&drv_data->any_motion_trigger);
		}
	}

	setup_int1(true);
}

#ifdef CONFIG_BMA400_TRIGGER_OWN_THREAD
static void bma400_thread(struct bma400_data *drv_data)
{
	while (1) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		bma400_thread_cb(drv_data->dev);
	}
}
#endif

#ifdef CONFIG_BMA400_TRIGGER_GLOBAL_THREAD
static void bma400_work_cb(struct k_work *work)
{
	struct bma400_data *drv_data =
		CONTAINER_OF(work, struct bma400_data, work);
	// const struct device *dev = CONTAINER_OF(drv_data, struct device, data);

	bma400_thread_cb(drv_data->dev);
}
#endif

int bma400_trigger_set(const struct device *dev,
		const struct sensor_trigger *trig,
		sensor_trigger_handler_t handler)
{
	struct bma400_data *drv_data = dev->data;

	if (trig->type == SENSOR_TRIG_DATA_READY) {
		/* Disable interrupt while updating */
		setup_int1(false);
		if (i2c_reg_write_byte(drv_data->i2c, drv_data->addr,
					BMA400_REG_INT_CONF_0,
					0 << BMA400_EN_DRDY_POS) < 0) { // FIXME, this is just 0
			LOG_ERR("Could not disable data ready interrupt");
			return -EIO;
		}

		drv_data->data_ready_handler = handler;
		if (handler == NULL) {
			setup_int1(true);
			return 0;
		}

		drv_data->data_ready_trigger = *trig;

		/* Enable data ready interrupt */
		if (i2c_reg_write_byte(drv_data->i2c, drv_data->addr,
					BMA400_REG_INT_CONF_0,
					1 << BMA400_EN_DRDY_POS) < 0) {
			return -EIO;
		}

		/* Map data ready interrupt to int1 pin */
		if (i2c_reg_update_byte(drv_data->i2c, drv_data->addr,
					BMA400_REG_INT1_MAP,
					0x80,
					0x80) < 0) {
			return -EIO;
		}

		setup_int1(true);

	} else if (trig->type == SENSOR_TRIG_THRESHOLD) {
		setup_int1(false);
		drv_data->any_motion_handler = handler;
		if (handler == NULL) {
			setup_int1(true);
			return 0;
		}

		drv_data->any_motion_trigger = *trig;

		if (i2c_reg_update_byte(drv_data->i2c, drv_data->addr,
					BMA400_REG_INT1_MAP,
					BMA400_EN_GEN1_MSK,
					(1 << BMA400_EN_GEN1_POS)) < 0) {
			LOG_ERR("Couldn't enable GEN1 interrupt");
			return -EIO;
		}

		setup_int1(true);
	} else if (trig->type == SENSOR_TRIG_DELTA) {
		return -ENOTSUP; // TODO
	} else {
		return -ENOTSUP;
	}

	return 0;
}

int bma400_init_interrupt(const struct device *dev)
{
	struct bma400_data *drv_data = dev->data;

	/* Enable activity change interrupt */
	/*
	   if (i2c_reg_write_byte(drv_data->i2c, drv_data->addr,
	   BMA400_REG_INT_CONFIG1,
	   BMA400_EN_ACTCH_MSK) < 0) {
	   return -EIO;
	   }
	   */

	/* This register controls polarity and driver mode (pushpull/open
	 * drain) which we prob won't need immediately

	 But it would be nice to check GPIO_ACTIVE_* and configure this register
	 accordingly
	 */
	/*
	if (i2c_reg_write_byte(drv_data->i2c, drv_data->addr,
				BMA400_REG_INT_12_IO_CTRL, some_stuff
			      ) < 0) {
		return -EIO;
	}
	*/

	/* Hysteresis = 48mg, update reference data after each INT, set data
	 * source to acc_filt2, enable all axes
	 */
	/*
	   if (i2c_reg_write_byte(drv_data->i2c, drv_data->addr,
	   BMA400_REG_GEN1INT_CONFIG0,
	   BMA400_HYST_48_MG |
	   BMA400_GEN1_ACT_REFU_EVERYTIME |
	   BMA400_GEN1_DATA_SRC_ACC_FILT2 |
	   BMA400_GEN1_ACT_X_EN |
	   BMA400_GEN1_ACT_Y_EN |
	   BMA400_GEN1_ACT_Z_EN) < 0) {
	   return -EIO;
	   }
	   */

	/* Configure int on ACTIVITY, all axes ORed */
	/*
	   if (i2c_reg_write_byte(drv_data->i2c, drv_data->addr,
	   BMA400_REG_GEN1INT_CONFIG1,
	   0x02) < 0) {
	   return -EIO;
	   }
	   */

	/* Activity threshold, 8mg/LSB, so 16 -> 128 mg threshold */
	/*
	   if (i2c_reg_write_byte(drv_data->i2c, drv_data->addr,
	   BMA400_REG_GEN1INT_CONFIG2,
	   1) < 0) {
	   return -EIO;
	   }
	   */

	/* Configure GPIO pin for input */
	if (gpio_pin_configure_dt(&int1, GPIO_INPUT) < 0) {
		return -EIO;
	}

	/* Set interrupt callback for GPIO pin */
	gpio_init_callback(&drv_data->gpio_cb, bma400_gpio_callback, BIT(int1.pin));
	if (gpio_add_callback(int1.port, &drv_data->gpio_cb) < 0) {
		return -EIO;
	}

#if defined(CONFIG_BMA400_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_BMA400_THREAD_STACK_SIZE,
			(k_thread_entry_t)bma400_thread, drv_data,
			NULL, NULL, K_PRIO_COOP(CONFIG_BMA400_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_BMA400_TRIGGER_GLOBAL_THREAD)
	k_work_init(&drv_data->work, bma400_work_cb);
#endif
	return 0;
}
