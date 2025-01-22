/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_lis2dh

#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define START_TRIG_INT1			0
#define START_TRIG_INT2			1
#define TRIGGED_INT1			4
#define TRIGGED_INT2			5

LOG_MODULE_DECLARE(lis2dh, CONFIG_SENSOR_LOG_LEVEL);
#include "lis2dh.h"

static const gpio_flags_t gpio_int_cfg[5] = {
			GPIO_INT_EDGE_BOTH,
			GPIO_INT_EDGE_RISING,
			GPIO_INT_EDGE_FALLING,
			GPIO_INT_LEVEL_HIGH,
			GPIO_INT_LEVEL_LOW,
			};

static inline void setup_int1(const struct device *dev,
			      bool enable)
{
	const struct lis2dh_config *cfg = dev->config;

	gpio_pin_interrupt_configure_dt(&cfg->gpio_drdy,
					enable
					? gpio_int_cfg[cfg->int1_mode]
					: GPIO_INT_DISABLE);
}

static int lis2dh_trigger_drdy_set(const struct device *dev,
				   enum sensor_channel chan,
				   sensor_trigger_handler_t handler,
				   const struct sensor_trigger *trig)
{
	const struct lis2dh_config *cfg = dev->config;
	struct lis2dh_data *lis2dh = dev->data;
	int status;

	if (cfg->gpio_drdy.port == NULL) {
		LOG_ERR("trigger_set DRDY int not supported");
		return -ENOTSUP;
	}

	setup_int1(dev, false);

	/* cancel potentially pending trigger */
	atomic_clear_bit(&lis2dh->trig_flags, TRIGGED_INT1);

	status = lis2dh->hw_tf->update_reg(dev, LIS2DH_REG_CTRL3,
					   LIS2DH_EN_DRDY1_INT1, 0);

	lis2dh->handler_drdy = handler;
	lis2dh->trig_drdy = trig;
	if ((handler == NULL) || (status < 0)) {
		return status;
	}

	lis2dh->chan_drdy = chan;

	/* serialize start of int1 in thread to synchronize output sampling
	 * and first interrupt. this avoids concurrent bus context access.
	 */
	atomic_set_bit(&lis2dh->trig_flags, START_TRIG_INT1);
#if defined(CONFIG_LIS2DH_TRIGGER_OWN_THREAD)
	k_sem_give(&lis2dh->gpio_sem);
#elif defined(CONFIG_LIS2DH_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&lis2dh->work);
#endif

	return 0;
}

static int lis2dh_start_trigger_int1(const struct device *dev)
{
	int status;
	uint8_t raw[LIS2DH_BUF_SZ];
	uint8_t ctrl1 = 0U;
	struct lis2dh_data *lis2dh = dev->data;

	/* power down temporarily to align interrupt & data output sampling */
	status = lis2dh->hw_tf->read_reg(dev, LIS2DH_REG_CTRL1, &ctrl1);
	if (unlikely(status < 0)) {
		return status;
	}
	status = lis2dh->hw_tf->write_reg(dev, LIS2DH_REG_CTRL1,
					  ctrl1 & ~LIS2DH_ODR_MASK);

	if (unlikely(status < 0)) {
		return status;
	}

	LOG_DBG("ctrl1=0x%x @tick=%u", ctrl1, k_cycle_get_32());

	/* empty output data */
	status = lis2dh->hw_tf->read_data(dev, LIS2DH_REG_STATUS,
					  raw, sizeof(raw));
	if (unlikely(status < 0)) {
		return status;
	}

	setup_int1(dev, true);

	/* re-enable output sampling */
	status = lis2dh->hw_tf->write_reg(dev, LIS2DH_REG_CTRL1, ctrl1);
	if (unlikely(status < 0)) {
		return status;
	}

	return lis2dh->hw_tf->update_reg(dev, LIS2DH_REG_CTRL3,
					 LIS2DH_EN_DRDY1_INT1,
					 LIS2DH_EN_DRDY1_INT1);
}

#define LIS2DH_ANYM_CFG (LIS2DH_INT_CFG_ZHIE_ZUPE | LIS2DH_INT_CFG_YHIE_YUPE |\
			 LIS2DH_INT_CFG_XHIE_XUPE)

static inline void setup_int2(const struct device *dev,
			      bool enable)
{
	const struct lis2dh_config *cfg = dev->config;

	gpio_pin_interrupt_configure_dt(&cfg->gpio_int,
					enable
					? gpio_int_cfg[cfg->int2_mode]
					: GPIO_INT_DISABLE);
}

/* common handler for any motion and tap triggers */
static int lis2dh_trigger_anym_tap_set(const struct device *dev,
				       sensor_trigger_handler_t handler,
				       const struct sensor_trigger *trig)
{
	const struct lis2dh_config *cfg = dev->config;
	struct lis2dh_data *lis2dh = dev->data;
	int status;
	uint8_t reg_val;

	if (cfg->gpio_int.port == NULL) {
		LOG_ERR("trigger_set AnyMotion int not supported");
		return -ENOTSUP;
	}

	setup_int2(dev, false);

	/* cancel potentially pending trigger */
	atomic_clear_bit(&lis2dh->trig_flags, TRIGGED_INT2);

	if (cfg->hw.anym_on_int1) {
		status = lis2dh->hw_tf->update_reg(dev, LIS2DH_REG_CTRL3,
						   LIS2DH_EN_DRDY1_INT1, 0);
	}

	/* disable any movement interrupt events */
	status = lis2dh->hw_tf->write_reg(dev,
					  cfg->hw.anym_on_int1 ?
						LIS2DH_REG_INT1_CFG :
						LIS2DH_REG_INT2_CFG,
					  0);
	/* disable any click interrupt events */
	status = lis2dh->hw_tf->write_reg(dev,
					  LIS2DH_REG_CFG_CLICK,
					  0);

	/* make sure any pending interrupt is cleared */
	status = lis2dh->hw_tf->read_reg(dev,
					 cfg->hw.anym_on_int1 ?
						LIS2DH_REG_INT1_SRC :
						LIS2DH_REG_INT2_SRC,
					 &reg_val);
	status = lis2dh->hw_tf->read_reg(dev,
					 LIS2DH_REG_CLICK_SRC,
					 &reg_val);

	if (trig->type == SENSOR_TRIG_DELTA) {
		lis2dh->handler_anymotion = handler;
		lis2dh->trig_anymotion = trig;
	} else if (trig->type == SENSOR_TRIG_TAP) {
		lis2dh->handler_tap = handler;
		lis2dh->trig_tap = trig;
	}

	if ((handler == NULL) || (status < 0)) {
		return status;
	}

	/* serialize start of int2 in thread to synchronize output sampling
	 * and first interrupt. this avoids concurrent bus context access.
	 */
	atomic_set_bit(&lis2dh->trig_flags, START_TRIG_INT2);
#if defined(CONFIG_LIS2DH_TRIGGER_OWN_THREAD)
	k_sem_give(&lis2dh->gpio_sem);
#elif defined(CONFIG_LIS2DH_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&lis2dh->work);
#endif
	return 0;
}

static int lis2dh_trigger_anym_set(const struct device *dev,
				   sensor_trigger_handler_t handler,
				   const struct sensor_trigger *trig)
{
	return lis2dh_trigger_anym_tap_set(dev, handler, trig);
}

static int lis2dh_trigger_tap_set(const struct device *dev,
				  sensor_trigger_handler_t handler,
				  const struct sensor_trigger *trig)
{
	return lis2dh_trigger_anym_tap_set(dev, handler, trig);
}

static int lis2dh_start_trigger_int2(const struct device *dev)
{
	struct lis2dh_data *lis2dh = dev->data;
	const struct lis2dh_config *cfg = dev->config;
	int status = 0;
	uint8_t reg = 0, mask = 0, val = 0;

	setup_int2(dev, true);

	bool has_anyt = (lis2dh->handler_tap != NULL);
	bool has_anym = (lis2dh->handler_anymotion != NULL);

	/* configure any motion interrupt */
	reg  = cfg->hw.anym_on_int1 ? LIS2DH_REG_INT1_CFG : LIS2DH_REG_INT2_CFG;
	val  = (cfg->hw.anym_mode << LIS2DH_INT_CFG_MODE_SHIFT) | LIS2DH_ANYM_CFG;
	status = lis2dh->hw_tf->write_reg(dev, reg, val);
	if (status < 0) {
		LOG_ERR("Failed to configure any motion interrupt");
		return status;
	}

	/* enable any motion detection on int line */
	reg  = cfg->hw.anym_on_int1 ? LIS2DH_REG_CTRL3 : LIS2DH_REG_CTRL6;
	mask = cfg->hw.anym_on_int1 ? LIS2DH_EN_IA_INT1 : LIS2DH_EN_IA_INT2;
	val  = has_anym ? mask : 0;
	status = lis2dh->hw_tf->update_reg(dev, reg, mask, val);
	if (status < 0) {
		LOG_ERR("Failed to enable any motion detection on int line");
		return status;
	}

	/* configure tap interrupt on all axes */
	reg  = LIS2DH_REG_CFG_CLICK;
	mask = LIS2DH_EN_CLICK_XS | LIS2DH_EN_CLICK_YS | LIS2DH_EN_CLICK_ZS;
	val  = has_anyt ? mask : 0;
	status = lis2dh->hw_tf->update_reg(dev, reg, mask, val);
	if (status < 0) {
		LOG_ERR("Failed to configure tap interrupt");
		return status;
	}

	/* set click detection on int line */
	reg  = cfg->hw.anym_on_int1 ? LIS2DH_REG_CTRL3 : LIS2DH_REG_CTRL6;
	mask = cfg->hw.anym_on_int1 ? LIS2DH_EN_CLICK_INT1 : LIS2DH_EN_CLICK_INT2;
	val  = has_anyt ? mask : 0;
	status = lis2dh->hw_tf->update_reg(dev, reg, mask, val);
	if (status < 0) {
		LOG_ERR("Failed to enable click detection on int line");
		return status;
	}
	return 0;
}

int lis2dh_trigger_set(const struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	if (trig->type == SENSOR_TRIG_DATA_READY &&
	    trig->chan == SENSOR_CHAN_ACCEL_XYZ) {
		return lis2dh_trigger_drdy_set(dev, trig->chan, handler, trig);
	} else if (trig->type == SENSOR_TRIG_DELTA) {
		return lis2dh_trigger_anym_set(dev, handler, trig);
	} else if (trig->type == SENSOR_TRIG_TAP) {
		return lis2dh_trigger_tap_set(dev, handler, trig);
	}

	return -ENOTSUP;
}

int lis2dh_acc_slope_config(const struct device *dev,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	struct lis2dh_data *lis2dh = dev->data;
	const struct lis2dh_config *cfg = dev->config;
	int status;

	if (attr == SENSOR_ATTR_SLOPE_TH) {
		uint8_t range_g, reg_val;
		uint32_t slope_th_ums2;

		status = lis2dh->hw_tf->read_reg(dev, LIS2DH_REG_CTRL4,
						 &reg_val);
		if (status < 0) {
			return status;
		}

		/* fs reg value is in the range 0 (2g) - 3 (16g) */
		range_g = 2 * (1 << ((LIS2DH_FS_MASK & reg_val)
				      >> LIS2DH_FS_SHIFT));

		slope_th_ums2 = val->val1 * 1000000 + val->val2;

		/* make sure the provided threshold does not exceed range */
		if ((slope_th_ums2 - 1) > (range_g * SENSOR_G)) {
			return -EINVAL;
		}

		/* 7 bit full range value */
		reg_val = 128 / range_g * (slope_th_ums2 - 1) / SENSOR_G;

		LOG_INF("int2_ths=0x%x range_g=%d ums2=%u", reg_val,
			    range_g, slope_th_ums2 - 1);

		/* Configure threshold for the any motion recognition */
		status = lis2dh->hw_tf->write_reg(dev,
						  cfg->hw.anym_on_int1 ?
							LIS2DH_REG_INT1_THS :
							LIS2DH_REG_INT2_THS,
						  reg_val);

		/* Configure threshold for the Click recognition */
		status = lis2dh->hw_tf->write_reg(dev,
						  LIS2DH_REG_CFG_CLICK_THS,
						  LIS2DH_CLICK_LIR | reg_val);
	} else { /* SENSOR_ATTR_SLOPE_DUR */
		/*
		 * slope duration is measured in number of samples:
		 * N/ODR where N is the register value
		 */
		if (val->val1 < 0 || val->val1 > 127) {
			return -ENOTSUP;
		}

		LOG_INF("int2_dur=0x%x", val->val1);

		/* Configure time limit for the any motion recognition */
		status = lis2dh->hw_tf->write_reg(dev,
						  cfg->hw.anym_on_int1 ?
							LIS2DH_REG_INT1_DUR :
							LIS2DH_REG_INT2_DUR,
						  val->val1);

		/* Configure time limit for the Click recognition */
		status = lis2dh->hw_tf->write_reg(dev,
						  LIS2DH_REG_TIME_LIMIT,
						  val->val1);
	}

	return status;
}

static void lis2dh_gpio_int1_callback(const struct device *dev,
				      struct gpio_callback *cb, uint32_t pins)
{
	struct lis2dh_data *lis2dh =
		CONTAINER_OF(cb, struct lis2dh_data, gpio_int1_cb);

	ARG_UNUSED(pins);

	atomic_set_bit(&lis2dh->trig_flags, TRIGGED_INT1);

	/* int is level triggered so disable until processed */
	setup_int1(lis2dh->dev, false);

#if defined(CONFIG_LIS2DH_TRIGGER_OWN_THREAD)
	k_sem_give(&lis2dh->gpio_sem);
#elif defined(CONFIG_LIS2DH_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&lis2dh->work);
#endif
}

static void lis2dh_gpio_int2_callback(const struct device *dev,
				      struct gpio_callback *cb, uint32_t pins)
{
	struct lis2dh_data *lis2dh =
		CONTAINER_OF(cb, struct lis2dh_data, gpio_int2_cb);

	ARG_UNUSED(pins);

	atomic_set_bit(&lis2dh->trig_flags, TRIGGED_INT2);

	/* int is level triggered so disable until processed */
	setup_int2(lis2dh->dev, false);

#if defined(CONFIG_LIS2DH_TRIGGER_OWN_THREAD)
	k_sem_give(&lis2dh->gpio_sem);
#elif defined(CONFIG_LIS2DH_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&lis2dh->work);
#endif
}

static void lis2dh_thread_cb(const struct device *dev)
{
	struct lis2dh_data *lis2dh = dev->data;
	const struct lis2dh_config *cfg = dev->config;
	int status;

	if (cfg->gpio_drdy.port &&
			unlikely(atomic_test_and_clear_bit(&lis2dh->trig_flags,
			START_TRIG_INT1))) {
		status = lis2dh_start_trigger_int1(dev);

		if (unlikely(status < 0)) {
			LOG_ERR("lis2dh_start_trigger_int1: %d", status);
		}
		return;
	}

	if (cfg->gpio_int.port &&
			unlikely(atomic_test_and_clear_bit(&lis2dh->trig_flags,
			START_TRIG_INT2))) {
		status = lis2dh_start_trigger_int2(dev);

		if (unlikely(status < 0)) {
			LOG_ERR("lis2dh_start_trigger_int2: %d", status);
		}
		return;
	}

	if (cfg->gpio_drdy.port &&
			atomic_test_and_clear_bit(&lis2dh->trig_flags,
			TRIGGED_INT1)) {
		if (likely(lis2dh->handler_drdy != NULL)) {
			lis2dh->handler_drdy(dev, lis2dh->trig_drdy);
		}

		/* Reactivate level triggered interrupt if handler did not
		 * disable itself
		 */
		if (likely(lis2dh->handler_drdy != NULL)) {
			setup_int1(dev, true);
		}

		return;
	}

	if (cfg->gpio_int.port &&
			atomic_test_and_clear_bit(&lis2dh->trig_flags,
			TRIGGED_INT2)) {
		uint8_t reg_val = 0;

		/* if necessary also clears an interrupt to de-assert int line */
		status = lis2dh->hw_tf->read_reg(dev,
						 cfg->hw.anym_on_int1 ?
							LIS2DH_REG_INT1_SRC :
							LIS2DH_REG_INT2_SRC,
						 &reg_val);
		if (status < 0) {
			LOG_ERR("clearing interrupt 2 failed: %d", status);
			return;
		}

		if (likely(lis2dh->handler_anymotion != NULL) &&
				(reg_val >> LIS2DH_INT_CFG_MODE_SHIFT)) {
			lis2dh->handler_anymotion(dev, lis2dh->trig_anymotion);

			LOG_DBG("@tick=%u int2_src=0x%x", k_cycle_get_32(), reg_val);
		}

		/* read click interrupt */
		status = lis2dh->hw_tf->read_reg(dev, LIS2DH_REG_CLICK_SRC,
						 &reg_val);
		if (status < 0) {
			LOG_ERR("clearing interrupt 2 failed: %d", status);
			return;
		}

		if (likely(lis2dh->handler_tap != NULL) &&
				(reg_val & LIS2DH_CLICK_SRC_SCLICK)) {
			lis2dh->handler_tap(dev, lis2dh->trig_tap);

			LOG_DBG("@tick=%u click_src=0x%x", k_cycle_get_32(), reg_val);
		}

		/* Reactivate level triggered interrupt if handler did not
		 * disable itself
		 */
		if (lis2dh->handler_anymotion || lis2dh->handler_tap) {
			setup_int2(dev, true);
		}

		return;
	}
}

#ifdef CONFIG_LIS2DH_TRIGGER_OWN_THREAD
static void lis2dh_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct lis2dh_data *lis2dh = p1;

	while (1) {
		k_sem_take(&lis2dh->gpio_sem, K_FOREVER);
		lis2dh_thread_cb(lis2dh->dev);
	}
}
#endif

#ifdef CONFIG_LIS2DH_TRIGGER_GLOBAL_THREAD
static void lis2dh_work_cb(struct k_work *work)
{
	struct lis2dh_data *lis2dh =
		CONTAINER_OF(work, struct lis2dh_data, work);

	lis2dh_thread_cb(lis2dh->dev);
}
#endif

int lis2dh_init_interrupt(const struct device *dev)
{
	struct lis2dh_data *lis2dh = dev->data;
	const struct lis2dh_config *cfg = dev->config;
	int status;
	uint8_t raw[2];

	lis2dh->dev = dev;

#if defined(CONFIG_LIS2DH_TRIGGER_OWN_THREAD)
	k_sem_init(&lis2dh->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&lis2dh->thread, lis2dh->thread_stack, CONFIG_LIS2DH_THREAD_STACK_SIZE,
			lis2dh_thread, lis2dh, NULL, NULL,
			K_PRIO_COOP(CONFIG_LIS2DH_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif defined(CONFIG_LIS2DH_TRIGGER_GLOBAL_THREAD)
	lis2dh->work.handler = lis2dh_work_cb;
#endif

	/*
	 * Setup INT1 (for DRDY) if defined in DT
	 */

	/* setup data ready gpio interrupt */
	if (!gpio_is_ready_dt(&cfg->gpio_drdy)) {
		/* API may return false even when ptr is NULL */
		if (cfg->gpio_drdy.port != NULL) {
			LOG_ERR("device %s is not ready", cfg->gpio_drdy.port->name);
			return -ENODEV;
		}

		LOG_DBG("gpio_drdy not defined in DT");
		status = 0;
		goto check_gpio_int;
	}

	/* data ready int1 gpio configuration */
	status = gpio_pin_configure_dt(&cfg->gpio_drdy, GPIO_INPUT);
	if (status < 0) {
		LOG_ERR("Could not configure %s.%02u",
			cfg->gpio_drdy.port->name, cfg->gpio_drdy.pin);
		return status;
	}

	gpio_init_callback(&lis2dh->gpio_int1_cb,
			   lis2dh_gpio_int1_callback,
			   BIT(cfg->gpio_drdy.pin));

	status = gpio_add_callback(cfg->gpio_drdy.port, &lis2dh->gpio_int1_cb);
	if (status < 0) {
		LOG_ERR("Could not add gpio int1 callback");
		return status;
	}

	LOG_INF("%s: int1 on %s.%02u", dev->name,
				       cfg->gpio_drdy.port->name,
				       cfg->gpio_drdy.pin);

check_gpio_int:
	/*
	 * Setup Interrupt (for Any Motion) if defined in DT
	 */

	/* setup any motion gpio interrupt */
	if (!gpio_is_ready_dt(&cfg->gpio_int)) {
		/* API may return false even when ptr is NULL */
		if (cfg->gpio_int.port != NULL) {
			LOG_ERR("device %s is not ready", cfg->gpio_int.port->name);
			return -ENODEV;
		}

		LOG_DBG("gpio_int not defined in DT");
		status = 0;
		goto end;
	}

	/* any motion int2 gpio configuration */
	status = gpio_pin_configure_dt(&cfg->gpio_int, GPIO_INPUT);
	if (status < 0) {
		LOG_ERR("Could not configure %s.%02u",
			cfg->gpio_int.port->name, cfg->gpio_int.pin);
		return status;
	}

	gpio_init_callback(&lis2dh->gpio_int2_cb,
			   lis2dh_gpio_int2_callback,
			   BIT(cfg->gpio_int.pin));

	/* callback is going to be enabled by trigger setting function */
	status = gpio_add_callback(cfg->gpio_int.port, &lis2dh->gpio_int2_cb);
	if (status < 0) {
		LOG_ERR("Could not add gpio int2 callback (%d)", status);
		return status;
	}

	LOG_INF("%s: int2 on %s.%02u", dev->name,
	   cfg->gpio_int.port->name,
	   cfg->gpio_int.pin);

	/* disable interrupt in case of warm (re)boot */
	status = lis2dh->hw_tf->write_reg(dev,
					  cfg->hw.anym_on_int1 ?
						LIS2DH_REG_INT1_CFG :
						LIS2DH_REG_INT2_CFG,
					  0);
	if (status < 0) {
		LOG_ERR("Interrupt disable reg write failed (%d)", status);
		return status;
	}
	status = lis2dh->hw_tf->write_reg(dev,
					  LIS2DH_REG_CFG_CLICK,
					  0);
	if (status < 0) {
		LOG_ERR("Interrupt disable reg write failed (%d)", status);
		return status;
	}

	(void)memset(raw, 0, sizeof(raw));
	status = lis2dh->hw_tf->write_data(dev,
					   cfg->hw.anym_on_int1 ?
						LIS2DH_REG_INT1_THS :
						LIS2DH_REG_INT2_THS,
					   raw, sizeof(raw));
	if (status < 0) {
		LOG_ERR("Burst write to THS failed (%d)", status);
		return status;
	}

	if (cfg->hw.anym_latch) {
		/* latch line interrupt */
		status = lis2dh->hw_tf->write_reg(dev,
						  LIS2DH_REG_CTRL5,
						  cfg->hw.anym_on_int1 ?
							LIS2DH_EN_LIR_INT1 :
							LIS2DH_EN_LIR_INT2);
	}

	if (status < 0) {
		LOG_ERR("enable reg write failed (%d)", status);
		return status;
	}

end:
	return status;
}
