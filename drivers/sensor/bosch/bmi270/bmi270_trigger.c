/*
 * Copyright (c) 2023 Elektronikutvecklingsbyrån EUB AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bmi270);

#include "bmi270.h"

#if defined(CONFIG_BMI270_STREAM)
/* Dedicated work queue so FIFO handler runs on a thread with large stack (SPI + RTIO). */
static K_KERNEL_STACK_DEFINE(bmi270_fifo_work_stack, CONFIG_BMI270_FIFO_WORKQ_STACK_SIZE);
static struct k_work_q bmi270_fifo_work_q;
static bool bmi270_fifo_work_q_initialized;

static inline const struct gpio_dt_spec *bmi270_fifo_irq_pin(const struct bmi270_config *cfg)
{
#if defined(CONFIG_BMI270_FIFO_ON_INT2)
	return &cfg->int2;
#else
	return &cfg->int1;
#endif
}

static void bmi270_fifo_work_handler(struct k_work *work)
{
	struct bmi270_data *data = CONTAINER_OF(work, struct bmi270_data, fifo_work);

	bmi270_stream_handle_fifo(data->dev);
}

void bmi270_submit_fifo_work(const struct device *dev)
{
	struct bmi270_data *data = dev->data;

	k_work_submit_to_queue(&bmi270_fifo_work_q, &data->fifo_work);
}

struct k_work_q *bmi270_get_fifo_work_q(void)
{
	return &bmi270_fifo_work_q;
}

static bool bmi270_try_submit_fifo_irq(const struct device *dev, const struct gpio_dt_spec *irq_pin,
				       const char *label)
{
	struct bmi270_data *data = dev->data;

	if (data->streaming_sqe == NULL) {
		return false;
	}

	if (irq_pin->port) {
		gpio_pin_interrupt_configure_dt(irq_pin, GPIO_INT_DISABLE);
	}

	LOG_DBG("%s: submit FIFO work", label);
	k_work_submit_to_queue(&bmi270_fifo_work_q, &data->fifo_work);
	return true;
}
#endif

enum {
	INT_FLAGS_INT1,
	INT_FLAGS_INT2,
};

static void bmi270_raise_int_flag(const struct device *dev, int bit)
{
	struct bmi270_data *data = dev->data;

	atomic_set_bit(&data->int_flags, bit);

#if defined(CONFIG_BMI270_TRIGGER_OWN_THREAD)
	k_sem_give(&data->trig_sem);
#elif defined(CONFIG_BMI270_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->trig_work);
#endif
}

static void bmi270_int1_callback(const struct device *dev,
				 struct gpio_callback *cb, uint32_t pins)
{
	struct bmi270_data *data =
		CONTAINER_OF(cb, struct bmi270_data, int1_cb);

	bmi270_raise_int_flag(data->dev, INT_FLAGS_INT1);
}

static void bmi270_int2_callback(const struct device *dev,
				 struct gpio_callback *cb, uint32_t pins)
{
	struct bmi270_data *data =
		CONTAINER_OF(cb, struct bmi270_data, int2_cb);
	bmi270_raise_int_flag(data->dev, INT_FLAGS_INT2);
}

static void bmi270_thread_cb(const struct device *dev)
{
	struct bmi270_data *data = dev->data;
	int ret;

	/* INT1: FIFO by default, feature (motion) interrupts when using INT2 for FIFO */
	if (atomic_test_and_clear_bit(&data->int_flags, INT_FLAGS_INT1)) {
#if defined(CONFIG_BMI270_STREAM) && !defined(CONFIG_BMI270_FIFO_ON_INT2)
		const struct bmi270_config *cfg = dev->config;

		if (bmi270_try_submit_fifo_irq(dev, bmi270_fifo_irq_pin(cfg), "INT1")) {
			return;
		}
#endif
		uint16_t int_status;

		ret = bmi270_reg_read(dev, BMI270_REG_INT_STATUS_0,
			(uint8_t *)&int_status, sizeof(int_status));
		if (ret < 0) {
			LOG_ERR("read interrupt status returned %d", ret);
			return;
		}

		k_mutex_lock(&data->trigger_mutex, K_FOREVER);

		if (data->motion_handler != NULL) {
			if (int_status & BMI270_INT_STATUS_ANY_MOTION) {
				data->motion_handler(dev, data->motion_trigger);
			}
		}

		k_mutex_unlock(&data->trigger_mutex);
	}

	/* INT2: FIFO when CONFIG_BMI270_FIFO_ON_INT2, else data ready only */
	if (atomic_test_and_clear_bit(&data->int_flags, INT_FLAGS_INT2)) {
#if defined(CONFIG_BMI270_STREAM) && defined(CONFIG_BMI270_FIFO_ON_INT2)
		const struct bmi270_config *cfg = dev->config;

		if (bmi270_try_submit_fifo_irq(dev, bmi270_fifo_irq_pin(cfg), "INT2")) {
			return;
		}
#endif
		k_mutex_lock(&data->trigger_mutex, K_FOREVER);

		if (data->drdy_handler != NULL) {
			data->drdy_handler(dev, data->drdy_trigger);
		}

		k_mutex_unlock(&data->trigger_mutex);
	}
}

#ifdef CONFIG_BMI270_TRIGGER_OWN_THREAD
static void bmi270_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct bmi270_data *data = p1;

	while (1) {
		k_sem_take(&data->trig_sem, K_FOREVER);
		bmi270_thread_cb(data->dev);
	}
}
#endif

#ifdef CONFIG_BMI270_TRIGGER_GLOBAL_THREAD
static void bmi270_trig_work_cb(struct k_work *work)
{
	struct bmi270_data *data =
		CONTAINER_OF(work, struct bmi270_data, trig_work);

	bmi270_thread_cb(data->dev);
}
#endif

static int bmi270_feature_reg_write(const struct device *dev,
			     const struct bmi270_feature_reg *reg,
			     uint16_t value)
{
	int ret;
	uint8_t feat_page = reg->page;

	ret = bmi270_reg_write(dev, BMI270_REG_FEAT_PAGE, &feat_page, 1);
	if (ret < 0) {
		LOG_ERR("bmi270_reg_write (0x%02x) failed: %d", BMI270_REG_FEAT_PAGE, ret);
		return ret;
	}

	LOG_DBG("feature reg[0x%02x]@%d = 0x%04x", reg->addr, reg->page, value);

	ret = bmi270_reg_write(dev, reg->addr, (uint8_t *)&value, 2);
	if (ret < 0) {
		LOG_ERR("bmi270_reg_write (0x%02x) failed: %d", reg->addr, ret);
		return ret;
	}

	return 0;
}

static int bmi270_init_int_pin(const struct gpio_dt_spec *pin,
			       struct gpio_callback *pin_cb,
			       gpio_callback_handler_t handler)
{
	int ret;

	if (!pin->port) {
		return 0;
	}

	if (!device_is_ready(pin->port)) {
		LOG_DBG("%s not ready", pin->port->name);
		return -ENODEV;
	}

	gpio_init_callback(pin_cb, handler, BIT(pin->pin));

	ret = gpio_pin_configure_dt(pin, GPIO_INPUT);
	if (ret) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(pin, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret) {
		return ret;
	}

	ret = gpio_add_callback(pin->port, pin_cb);
	if (ret) {
		return ret;
	}

	return 0;
}

static int bmi270_configure_int_io_ctrl(const struct device *dev, const struct gpio_dt_spec *pin,
					uint8_t reg, const char *name)
{
	int ret;
	uint8_t io_ctrl = BMI270_INT_IO_CTRL_OUTPUT_EN | BMI270_INT_IO_CTRL_LVL;

	if (!pin->port) {
		return 0;
	}

	ret = bmi270_reg_write(dev, reg, &io_ctrl, 1);
	if (ret < 0) {
		LOG_ERR("failed configuring %s_IO_CTRL (%d)", name, ret);
		return ret;
	}

	return 0;
}

int bmi270_init_interrupts(const struct device *dev)
{
	const struct bmi270_config *cfg = dev->config;
	struct bmi270_data *data = dev->data;
	int ret;

#if CONFIG_BMI270_TRIGGER_OWN_THREAD
	k_sem_init(&data->trig_sem, 0, 1);
	k_thread_create(&data->thread, data->thread_stack, CONFIG_BMI270_THREAD_STACK_SIZE,
			bmi270_thread, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_BMI270_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif CONFIG_BMI270_TRIGGER_GLOBAL_THREAD
	k_work_init(&data->trig_work, bmi270_trig_work_cb);
#endif

	ret = bmi270_init_int_pin(&cfg->int1, &data->int1_cb,
				  bmi270_int1_callback);
	if (ret) {
		LOG_ERR("Failed to initialize INT1");
		return -EINVAL;
	}

	ret = bmi270_init_int_pin(&cfg->int2, &data->int2_cb,
				  bmi270_int2_callback);
	if (ret) {
		LOG_ERR("Failed to initialize INT2 (required for FIFO and data ready)");
		return -EINVAL;
	}

#if defined(CONFIG_BMI270_STREAM)
	if (!bmi270_fifo_work_q_initialized) {
		k_work_queue_init(&bmi270_fifo_work_q);
		k_work_queue_start(&bmi270_fifo_work_q, bmi270_fifo_work_stack,
				   K_THREAD_STACK_SIZEOF(bmi270_fifo_work_stack),
				   CONFIG_BMI270_THREAD_PRIORITY - 1, NULL);
		bmi270_fifo_work_q_initialized = true;
	}
	k_work_init(&data->fifo_work, bmi270_fifo_work_handler);
#endif

	ret = bmi270_configure_int_io_ctrl(dev, &cfg->int1, BMI270_REG_INT1_IO_CTRL, "INT1");
	if (ret < 0) {
		return ret;
	}

	ret = bmi270_configure_int_io_ctrl(dev, &cfg->int2, BMI270_REG_INT2_IO_CTRL, "INT2");
	if (ret < 0) {
		return ret;
	}

	/*
	 * Permanent latched: pin stays HIGH until INT_STATUS is read AND the
	 * interrupt condition is no longer active.  Non-latched mode leaves
	 * INT1 permanently HIGH on this hardware even with no mapped sources.
	 * With latched mode the pin deasserts after the handler drains the
	 * FIFO and reads INT_STATUS_1, giving the nRF GPIO a clean edge.
	 */
	uint8_t int_latch = BMI270_INT_LATCH_PERMANENT;

	ret = bmi270_reg_write(dev, BMI270_REG_INT_LATCH, &int_latch, 1);
	if (ret < 0) {
		LOG_ERR("failed configuring INT_LATCH (%d)", ret);
		return ret;
	}

	/*
	 * Clear any stale latched status so INT pins deassert before the
	 * GPIO edge interrupt is armed.
	 */
	uint8_t dummy[2];

	bmi270_reg_read(dev, BMI270_REG_INT_STATUS_0, dummy, 2);

	return 0;
}

static int bmi270_anymo_config(const struct device *dev, bool enable)
{
	const struct bmi270_config *cfg = dev->config;
	struct bmi270_data *data = dev->data;
	uint16_t anymo_2;
	int ret;

	if (enable) {
		ret = bmi270_feature_reg_write(dev, cfg->feature->anymo_1,
					       data->anymo_1);
		if (ret < 0) {
			return ret;
		}
	}

	anymo_2 = data->anymo_2;
	if (enable) {
		anymo_2 |= BMI270_ANYMO_2_ENABLE;
	}

	ret = bmi270_feature_reg_write(dev, cfg->feature->anymo_2, anymo_2);
	if (ret < 0) {
		return ret;
	}

	uint8_t int1_map_feat = 0;

	if (enable) {
		int1_map_feat |= BMI270_INT_MAP_ANY_MOTION;
	}

	ret = bmi270_reg_write(dev, BMI270_REG_INT1_MAP_FEAT, &int1_map_feat, 1);
	if (ret < 0) {
		LOG_ERR("failed configuring INT1_MAP_FEAT (%d)", ret);
		return ret;
	}

	return 0;
}

static int bmi270_drdy_config(const struct device *dev, bool enable)
{
	int ret;
	uint8_t int_map_data = 0;

	if (enable) {
		int_map_data |= BMI270_INT_MAP_DATA_DRDY_INT2;
	}

	ret = bmi270_reg_write(dev, BMI270_REG_INT_MAP_DATA, &int_map_data, 1);
	if (ret < 0) {
		LOG_ERR("failed configuring INT_MAP_DATA (%d)", ret);
		return ret;
	}

	return 0;
}

int bmi270_trigger_set(const struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	struct bmi270_data *data = dev->data;
	const struct bmi270_config *cfg = dev->config;

	switch (trig->type) {
	case SENSOR_TRIG_MOTION:
		if (!cfg->int1.port) {
			return -ENOTSUP;
		}

		k_mutex_lock(&data->trigger_mutex, K_FOREVER);
		data->motion_handler = handler;
		data->motion_trigger = trig;
		k_mutex_unlock(&data->trigger_mutex);
		return bmi270_anymo_config(dev, handler != NULL);

	case SENSOR_TRIG_DATA_READY:
		if (!cfg->int2.port) {
			return -ENOTSUP;
		}

		k_mutex_lock(&data->trigger_mutex, K_FOREVER);
		data->drdy_handler = handler;
		data->drdy_trigger = trig;
		k_mutex_unlock(&data->trigger_mutex);
		return bmi270_drdy_config(dev, handler != NULL);
	default:
		return -ENOTSUP;
	}

	return 0;
}
