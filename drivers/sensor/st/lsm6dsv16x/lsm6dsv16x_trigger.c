/* ST Microelectronics LSM6DSV16X 6-axis IMU sensor driver
 *
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lsm6dsv16x.pdf
 */

#define DT_DRV_COMPAT st_lsm6dsv16x

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "lsm6dsv16x.h"
#include "lsm6dsv16x_rtio.h"

LOG_MODULE_DECLARE(LSM6DSV16X, CONFIG_SENSOR_LOG_LEVEL);

/**
 * lsm6dsv16x_enable_xl_int - XL enable selected int pin to generate interrupt
 */
static int lsm6dsv16x_enable_xl_int(const struct device *dev, int enable)
{
	const struct lsm6dsv16x_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	int ret;

	if (enable) {
		int16_t buf[3];

		/* dummy read: re-trigger interrupt */
		lsm6dsv16x_acceleration_raw_get(ctx, buf);
	}

	/* set interrupt */
	if ((cfg->drdy_pin == 1) || (ON_I3C_BUS(cfg) && (!I3C_INT_PIN(cfg)))) {
		lsm6dsv16x_pin_int_route_t val = {};

		ret = lsm6dsv16x_pin_int1_route_get(ctx, &val);
		if (ret < 0) {
			LOG_ERR("pint_int1_route_get error");
			return ret;
		}

		val.drdy_xl = 1;

		ret = lsm6dsv16x_pin_int1_route_set(ctx, &val);
	} else {
		lsm6dsv16x_pin_int_route_t val = {};

		ret = lsm6dsv16x_pin_int2_route_get(ctx, &val);
		if (ret < 0) {
			LOG_ERR("pint_int2_route_get error");
			return ret;
		}

		val.drdy_xl = 1;

		ret = lsm6dsv16x_pin_int2_route_set(ctx, &val);
	}

	return ret;
}

/**
 * lsm6dsv16x_enable_g_int - Gyro enable selected int pin to generate interrupt
 */
static int lsm6dsv16x_enable_g_int(const struct device *dev, int enable)
{
	const struct lsm6dsv16x_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	int ret;

	if (enable) {
		int16_t buf[3];

		/* dummy read: re-trigger interrupt */
		lsm6dsv16x_angular_rate_raw_get(ctx, buf);
	}

	/* set interrupt */
	if ((cfg->drdy_pin == 1) || (ON_I3C_BUS(cfg) && (!I3C_INT_PIN(cfg)))) {
		lsm6dsv16x_pin_int_route_t val = {};

		ret = lsm6dsv16x_pin_int1_route_get(ctx, &val);
		if (ret < 0) {
			LOG_ERR("pint_int1_route_get error");
			return ret;
		}

		val.drdy_g = 1;

		ret = lsm6dsv16x_pin_int1_route_set(ctx, &val);
	} else {
		lsm6dsv16x_pin_int_route_t val = {};

		ret = lsm6dsv16x_pin_int2_route_get(ctx, &val);
		if (ret < 0) {
			LOG_ERR("pint_int2_route_get error");
			return ret;
		}

		val.drdy_g = 1;

		ret = lsm6dsv16x_pin_int2_route_set(ctx, &val);
	}

	return ret;
}

/**
 * lsm6dsv16x_enable_wake_int - Enable selected int pin to generate wakeup interrupt
 */
static int lsm6dsv16x_enable_wake_int(const struct device *dev, int enable)
{
	const struct lsm6dsv16x_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	int ret;
	lsm6dsv16x_interrupt_mode_t int_mode;

	int_mode.enable = enable ? 1 : 0;
	int_mode.lir = !cfg->drdy_pulsed;
	ret = lsm6dsv16x_interrupt_enable_set(ctx, int_mode);
	if (ret < 0) {
		LOG_ERR("interrupt_enable_set error");
		return ret;
	}

	if ((cfg->drdy_pin == 1) || ON_I3C_BUS(cfg)) {
		lsm6dsv16x_pin_int_route_t val;

		ret = lsm6dsv16x_pin_int1_route_get(ctx, &val);
		if (ret < 0) {
			LOG_ERR("pint_int1_route_get error");
			return ret;
		}

		val.wakeup = enable ? 1 : 0;

		ret = lsm6dsv16x_pin_int1_route_set(ctx, &val);
	} else {
		lsm6dsv16x_pin_int_route_t val;

		ret = lsm6dsv16x_pin_int2_route_get(ctx, &val);
		if (ret < 0) {
			LOG_ERR("pint_int2_route_get error");
			return ret;
		}

		val.wakeup = enable ? 1 : 0;

		ret = lsm6dsv16x_pin_int2_route_set(ctx, &val);
	}

	return ret;
}

/**
 * lsm6dsv16x_trigger_set - link external trigger to event data ready
 */
int lsm6dsv16x_trigger_set(const struct device *dev,
			  const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler)
{
	const struct lsm6dsv16x_config *cfg = dev->config;
	struct lsm6dsv16x_data *lsm6dsv16x = dev->data;
	int ret = 0;

	if (!cfg->trig_enabled) {
		LOG_ERR("trigger_set op not supported");
		return -ENOTSUP;
	}

	if (trig == NULL) {
		LOG_ERR("no trigger");
		return -EINVAL;
	}

	if (!lsm6dsv16x_is_active(dev)) {
		return -EBUSY;
	}

	switch (trig->type) {
	case SENSOR_TRIG_DATA_READY:
		if (trig->chan == SENSOR_CHAN_ACCEL_XYZ) {
			lsm6dsv16x->handler_drdy_acc = handler;
			lsm6dsv16x->trig_drdy_acc = trig;
			if (handler) {
				lsm6dsv16x_enable_xl_int(dev, LSM6DSV16X_EN_BIT);
			} else {
				lsm6dsv16x_enable_xl_int(dev, LSM6DSV16X_DIS_BIT);
			}
		} else if (trig->chan == SENSOR_CHAN_GYRO_XYZ) {
			lsm6dsv16x->handler_drdy_gyr = handler;
			lsm6dsv16x->trig_drdy_gyr = trig;
			if (handler) {
				lsm6dsv16x_enable_g_int(dev, LSM6DSV16X_EN_BIT);
			} else {
				lsm6dsv16x_enable_g_int(dev, LSM6DSV16X_DIS_BIT);
			}
		}
		break;
	case SENSOR_TRIG_DELTA:
		if (trig->chan != SENSOR_CHAN_ACCEL_XYZ) {
			return -ENOTSUP;
		}
		lsm6dsv16x->handler_wakeup = handler;
		lsm6dsv16x->trig_wakeup = trig;
		if (handler) {
			lsm6dsv16x_enable_wake_int(dev, LSM6DSV16X_EN_BIT);
		} else {
			lsm6dsv16x_enable_wake_int(dev, LSM6DSV16X_DIS_BIT);
		}
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

#if defined(CONFIG_LSM6DSV16X_TRIGGER_OWN_THREAD) || \
	defined(CONFIG_LSM6DSV16X_TRIGGER_GLOBAL_THREAD)
/**
 * lsm6dsv16x_handle_interrupt - handle the drdy event
 * read data and call handler if registered any
 */
static void lsm6dsv16x_handle_interrupt(const struct device *dev)
{
	struct lsm6dsv16x_data *lsm6dsv16x = dev->data;
	const struct lsm6dsv16x_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lsm6dsv16x_data_ready_t status;
	lsm6dsv16x_all_int_src_t all_int_src;
	int ret;

	while (1) {
		/* When using I3C IBI interrupt the status register is already automatically
		 * read (clearing the interrupt condition), so we can skip the extra bus
		 * transaction for FIFO stream case.
		 */
		if (ON_I3C_BUS(cfg) && !I3C_INT_PIN(cfg) && IS_ENABLED(CONFIG_LSM6DSV16X_STREAM)) {
			break;
		}

		if (lsm6dsv16x_flag_data_ready_get(ctx, &status) < 0) {
			LOG_DBG("failed reading status reg");
			return;
		}

		ret = lsm6dsv16x_read_reg(ctx, LSM6DSV16X_ALL_INT_SRC, (uint8_t *)&all_int_src, 1);
		if (ret < 0) {
			LOG_DBG("failed reading all_int_src reg");
			return;
		}

		if (((status.drdy_xl == 0) && (status.drdy_gy == 0) && (all_int_src.wu_ia == 0)) ||
		    IS_ENABLED(CONFIG_LSM6DSV16X_STREAM)) {
			break;
		}

		if ((status.drdy_xl) && (lsm6dsv16x->handler_drdy_acc != NULL)) {
			lsm6dsv16x->handler_drdy_acc(dev, lsm6dsv16x->trig_drdy_acc);
		}

		if ((status.drdy_gy) && (lsm6dsv16x->handler_drdy_gyr != NULL)) {
			lsm6dsv16x->handler_drdy_gyr(dev, lsm6dsv16x->trig_drdy_gyr);
		}

		if ((all_int_src.wu_ia) && lsm6dsv16x->handler_wakeup != NULL) {
			lsm6dsv16x->handler_wakeup(dev, lsm6dsv16x->trig_wakeup);
		}
	}

	if (!ON_I3C_BUS(cfg) || (I3C_INT_PIN(cfg))) {
		ret = gpio_pin_interrupt_configure_dt(lsm6dsv16x->drdy_gpio,
						GPIO_INT_EDGE_TO_ACTIVE);
		if (ret < 0) {
			LOG_ERR("%s: Not able to configure pin_int", dev->name);
		}
	}
}
#endif /* CONFIG_LSM6DSV16X_TRIGGER_OWN_THREAD || CONFIG_LSM6DSV16X_TRIGGER_GLOBAL_THREAD */

static void lsm6dsv16x_intr_callback(struct lsm6dsv16x_data *lsm6dsv16x)
{
#if defined(CONFIG_LSM6DSV16X_TRIGGER_OWN_THREAD)
	k_sem_give(&lsm6dsv16x->intr_sem);
#elif defined(CONFIG_LSM6DSV16X_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&lsm6dsv16x->work);
#endif /* CONFIG_LSM6DSV16X_TRIGGER_OWN_THREAD */
	if (IS_ENABLED(CONFIG_LSM6DSV16X_STREAM)) {
		lsm6dsv16x_stream_irq_handler(lsm6dsv16x->dev);
	}
}

static void lsm6dsv16x_gpio_callback(const struct device *dev,
				    struct gpio_callback *cb, uint32_t pins)
{
	struct lsm6dsv16x_data *lsm6dsv16x =
		CONTAINER_OF(cb, struct lsm6dsv16x_data, gpio_cb);
	int ret;

	ARG_UNUSED(pins);

	ret = gpio_pin_interrupt_configure_dt(lsm6dsv16x->drdy_gpio, GPIO_INT_DISABLE);
	if (ret < 0) {
		LOG_ERR("%s: Not able to configure pin_int", dev->name);
	}

	lsm6dsv16x_intr_callback(lsm6dsv16x);
}

#ifdef CONFIG_LSM6DSV16X_TRIGGER_OWN_THREAD
static void lsm6dsv16x_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct lsm6dsv16x_data *lsm6dsv16x = p1;

	while (1) {
		k_sem_take(&lsm6dsv16x->intr_sem, K_FOREVER);
		lsm6dsv16x_handle_interrupt(lsm6dsv16x->dev);
	}
}
#endif /* CONFIG_LSM6DSV16X_TRIGGER_OWN_THREAD */

#ifdef CONFIG_LSM6DSV16X_TRIGGER_GLOBAL_THREAD
static void lsm6dsv16x_work_cb(struct k_work *work)
{
	struct lsm6dsv16x_data *lsm6dsv16x =
		CONTAINER_OF(work, struct lsm6dsv16x_data, work);

	lsm6dsv16x_handle_interrupt(lsm6dsv16x->dev);
}
#endif /* CONFIG_LSM6DSV16X_TRIGGER_GLOBAL_THREAD */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
static int lsm6dsv16x_ibi_cb(struct i3c_device_desc *target,
			  struct i3c_ibi_payload *payload)
{
	const struct device *dev = target->dev;
	struct lsm6dsv16x_data *lsm6dsv16x = dev->data;

	/*
	 * The IBI Payload consist of the following 10 bytes :
	 * 1st byte: MDB
	 * - MDB[0]: FIFO interrupts (FIFO_WTM_IA, FIFO_OVR_IA, FIFO_FULL_IA, CONTER_BDR_IA)
	 * - MDB[1]: Physical interrupts (XLDS, GDA, TDA, XLDA_OIS, GDA_OIS)
	 * - MDB[2]: Basic interrupts (SLEEP_CHANGE_IA, D6D_IA, DOUBLE_TAP, SINGLE_TAP, WU_IA,
	 *           FF_IA)
	 * - MDB[3]: SHUB DRDY (SENS_HUB_ENDOP)
	 * - MDB[4]: Advanced Function interrupt group
	 * - MDB[7:5]: 3'b000: Vendor Definied
	 *             3'b100: Timing Information
	 * 2nd byte: FIFO_STATUS1
	 * 3rd byte: FIFO_STATUS2
	 * 4th byte: ALL_INT_SRC
	 * 5th byte: STATUS_REG
	 * 6th byte: STATUS_REG_OIS
	 * 7th byte: STATUS_MASTER_MAIN
	 * 8th byte: EMB_FUNC_STATUS
	 * 9th byte: FSM_STATUS
	 * 10th byte: MLC_STATUS
	 */
	if (payload->payload_len != sizeof(lsm6dsv16x->ibi_payload)) {
		LOG_ERR("Invalid IBI payload length");
		return -EINVAL;
	}

	memcpy(&lsm6dsv16x->ibi_payload, payload->payload, payload->payload_len);

	lsm6dsv16x_intr_callback(lsm6dsv16x);

	return 0;
}
#endif

int lsm6dsv16x_init_interrupt(const struct device *dev)
{
	struct lsm6dsv16x_data *lsm6dsv16x = dev->data;
	const struct lsm6dsv16x_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	int ret;

	lsm6dsv16x->drdy_gpio = (cfg->drdy_pin == 1) ?
			(struct gpio_dt_spec *)&cfg->int1_gpio :
			(struct gpio_dt_spec *)&cfg->int2_gpio;

	/* setup data ready gpio interrupt (INT1 or INT2) */
	if ((!ON_I3C_BUS(cfg) || (I3C_INT_PIN(cfg))) && !gpio_is_ready_dt(lsm6dsv16x->drdy_gpio)) {
		LOG_ERR("Cannot get pointer to drdy_gpio device");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_LSM6DSV16X_STREAM)) {
		lsm6dsv16x_stream_irq_handler(lsm6dsv16x->dev);
	}

#if defined(CONFIG_LSM6DSV16X_TRIGGER_OWN_THREAD)
	k_sem_init(&lsm6dsv16x->intr_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&lsm6dsv16x->thread, lsm6dsv16x->thread_stack,
			CONFIG_LSM6DSV16X_THREAD_STACK_SIZE,
			lsm6dsv16x_thread, lsm6dsv16x,
			NULL, NULL, K_PRIO_COOP(CONFIG_LSM6DSV16X_THREAD_PRIORITY),
			0, K_NO_WAIT);
	k_thread_name_set(&lsm6dsv16x->thread, "lsm6dsv16x");
#elif defined(CONFIG_LSM6DSV16X_TRIGGER_GLOBAL_THREAD)
	lsm6dsv16x->work.handler = lsm6dsv16x_work_cb;
#endif /* CONFIG_LSM6DSV16X_TRIGGER_OWN_THREAD */

	if (!ON_I3C_BUS(cfg) || (I3C_INT_PIN(cfg))) {
		ret = gpio_pin_configure_dt(lsm6dsv16x->drdy_gpio, GPIO_INPUT);
		if (ret < 0) {
			LOG_DBG("Could not configure gpio");
			return ret;
		}

		gpio_init_callback(&lsm6dsv16x->gpio_cb,
				lsm6dsv16x_gpio_callback,
				BIT(lsm6dsv16x->drdy_gpio->pin));

		if (gpio_add_callback(lsm6dsv16x->drdy_gpio->port, &lsm6dsv16x->gpio_cb) < 0) {
			LOG_DBG("Could not set gpio callback");
			return -EIO;
		}

	}

	/* set data ready mode on int1/int2/tir */
	LOG_DBG("drdy_pulsed is %d", (int)cfg->drdy_pulsed);
	lsm6dsv16x_data_ready_mode_t mode = cfg->drdy_pulsed ? LSM6DSV16X_DRDY_PULSED :
								LSM6DSV16X_DRDY_LATCHED;


	ret = lsm6dsv16x_data_ready_mode_set(ctx, mode);
	if (ret < 0) {
		LOG_ERR("drdy_pulsed config error %d", (int)cfg->drdy_pulsed);
		return ret;
	}

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
	if (ON_I3C_BUS(cfg)) {
		if (I3C_INT_PIN(cfg)) {
			/* Enable INT Pins when using I3C */
			ret = lsm6dsv16x_i3c_int_en_set(ctx, I3C_INT_PIN(cfg));
			if (ret < 0) {
				LOG_ERR("failed to enable int pin for I3C %d", ret);
				return ret;
			}

			ret = gpio_pin_interrupt_configure_dt(lsm6dsv16x->drdy_gpio,
							      GPIO_INT_EDGE_TO_ACTIVE);
			if (ret < 0) {
				LOG_ERR("Could not configure gpio interrupt");
				return ret;
			}
		} else {
			/* I3C IBI does not utilize GPIO interrupt. */
			lsm6dsv16x->i3c_dev->ibi_cb = lsm6dsv16x_ibi_cb;

			/*
			 * Set IBI availability time, this is the time that the sensor
			 * will wait for inactivity before it is okay to generate an IBI TIR.
			 *
			 * NOTE: There is a bug in the API and the Documentation where
			 * the defines for the values are incorrect. The correct values are:
			 * 0 = 50us
			 * 1 = 2us
			 * 2 = 1ms
			 * 3 = 25ms
			 */
			ret = lsm6dsv16x_i3c_ibi_time_set(ctx, cfg->bus_act_sel);
			if (ret < 0) {
				LOG_ERR("failed to set ibi available time %d", ret);
				return -EIO;
			}

			if (i3c_ibi_enable(lsm6dsv16x->i3c_dev) != 0) {
				LOG_ERR("Could not enable I3C IBI");
				return -EIO;
			}
		}

		return 0;
	}
#endif

	return gpio_pin_interrupt_configure_dt(lsm6dsv16x->drdy_gpio,
					       GPIO_INT_EDGE_TO_ACTIVE);
}
