/*
 * Copyright (c) 2026 Nainaiu Rakhaine
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT maxim_max30003

#include <string.h>

#include <zephyr/logging/log.h>

#include "max30003.h"

LOG_MODULE_DECLARE(MAX30003, CONFIG_SENSOR_LOG_LEVEL);

#define MAX30003_INT_MAX_LOOPS 4
/* INTB still active after the loops: look again after this delay. */
#define MAX30003_INT_RETRY  K_MSEC(10)
/* While a level condition is masked: check whether it ended this often. */
#define MAX30003_LEVEL_POLL K_MSEC(100)

#define NUM_TRIGGERS 7

/* STATUS bit for each trigger slot (see trig_index()). */
static const uint32_t trig_bit[NUM_TRIGGERS] = {
	MAX30003_INT_SAMP,   MAX30003_INT_EINT,   MAX30003_INT_RRINT,  MAX30003_INT_DCLOFFINT,
	MAX30003_INT_LONINT, MAX30003_INT_FSTINT, MAX30003_INT_PLLINT,
};

#ifdef CONFIG_MAX30003_TRIGGER_OWN_THREAD
static K_KERNEL_STACK_DEFINE(max30003_stack, CONFIG_MAX30003_THREAD_STACK_SIZE);
static struct k_work_q max30003_work_q;
static bool max30003_work_q_started;
#endif

static int trig_index(int type)
{
	switch (type) {
	case SENSOR_TRIG_DATA_READY:
		return 0;
	case SENSOR_TRIG_FIFO_WATERMARK:
		return 1;
	case SENSOR_TRIG_MAX30003_RR:
	case SENSOR_TRIG_MAX30003_LEAD_OFF:
	case SENSOR_TRIG_MAX30003_LEAD_ON:
	case SENSOR_TRIG_MAX30003_FAST_RECOVERY:
	case SENSOR_TRIG_MAX30003_PLL_UNLOCK:
		return 2 + (type - SENSOR_TRIG_MAX30003_RR);
	default:
		return -ENOTSUP;
	}
}

static void max30003_submit(struct max30003_data *data, k_timeout_t delay)
{
#ifdef CONFIG_MAX30003_TRIGGER_OWN_THREAD
	(void)k_work_reschedule_for_queue(&max30003_work_q, &data->work, delay);
#else
	(void)k_work_reschedule(&data->work, delay);
#endif
}

static void max30003_gpio_callback(const struct device *port, struct gpio_callback *cb,
				   gpio_port_pins_t pins)
{
	struct max30003_data *data = CONTAINER_OF(cb, struct max30003_data, gpio_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);
	max30003_submit(data, K_NO_WAIT);
}

int max30003_write_int_enable_locked(const struct device *dev)
{
	struct max30003_data *data = dev->data;
	uint32_t en = data->trig_enable & ~data->level_masked;

	/* Overflow is always handled while any trigger is active. */
	if (data->trig_enable != 0) {
		en |= MAX30003_INT_EOVF;
	}
	return max30003_reg_write(dev, MAX30003_REG_EN_INT,
				  en | FIELD_PREP(MAX30003_EN_INT_INTB_TYPE,
						  MAX30003_INTB_OD_PULLUP));
}

/*
 * Read STATUS once and work out which handlers to call. Level conditions are
 * masked after they fire and unmasked once STATUS shows they ended.
 */
static int process_status_locked(const struct device *dev, uint32_t *fire)
{
	struct max30003_data *data = dev->data;
	uint32_t status, ended, started;
	int ret;

	ret = max30003_reg_read(dev, MAX30003_REG_STATUS, &status);
	if (ret < 0) {
		return ret;
	}
	data->status_cache |= status;

	ended = data->level_masked & ~status;
	started = status & data->trig_enable & MAX30003_INT_LEVEL & ~data->level_masked;
	*fire = (status & data->trig_enable & ~MAX30003_INT_LEVEL) | started;

	if ((status & MAX30003_INT_RRINT) && data->cur.rtor_enable) {
		ret = max30003_update_rr_locked(dev);
	}
	if (ret == 0 && (status & MAX30003_INT_EOVF)) {
		ret = max30003_handle_overflow(dev);
	}
	if (ret == 0 && (ended | started) != 0) {
		data->level_masked = (data->level_masked & ~ended) | started;
		ret = max30003_write_int_enable_locked(dev);
	}
	return ret;
}

static void max30003_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct max30003_data *data = CONTAINER_OF(dwork, struct max30003_data, work);
	const struct device *dev = data->dev;
	const struct max30003_config *cfg = dev->config;
	sensor_trigger_handler_t handler[NUM_TRIGGERS];
	const struct sensor_trigger *trigger[NUM_TRIGGERS];
	bool active = false;
	bool level_masked;

	for (int i = 0; i < MAX30003_INT_MAX_LOOPS; i++) {
		uint32_t fire = 0;
		int ret;

		/*
		 * Copy the handler/trigger pairs under the lock: trigger_set() may
		 * change them while the handlers run without the lock held.
		 */
		k_mutex_lock(&data->lock, K_FOREVER);
		ret = process_status_locked(dev, &fire);
		memcpy(handler, data->handler, sizeof(handler));
		memcpy(trigger, data->trigger, sizeof(trigger));
		k_mutex_unlock(&data->lock);
		if (ret < 0) {
			LOG_ERR("%s: interrupt handling failed (%d)", dev->name, ret);
			return;
		}
		if (fire & MAX30003_INT_PLLINT) {
			LOG_WRN("%s: PLL unlocked: check FCLK", dev->name);
		}
		for (int t = 0; t < NUM_TRIGGERS; t++) {
			if ((fire & trig_bit[t]) && handler[t] != NULL) {
				handler[t](dev, trigger[t]);
			}
		}
		active = gpio_pin_get_dt(&cfg->int_gpio) > 0;
		if (!active) {
			break;
		}
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	level_masked = data->level_masked != 0;
	k_mutex_unlock(&data->lock);

	if (active) {
		max30003_submit(data, MAX30003_INT_RETRY);
	} else if (level_masked) {
		max30003_submit(data, MAX30003_LEVEL_POLL);
	}
}

int max30003_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	const struct max30003_config *cfg = dev->config;
	struct max30003_data *data = dev->data;
	int idx = trig_index((int)trig->type);
	uint32_t status;
	int ret;

	if (cfg->int_gpio.port == NULL) {
		LOG_ERR("%s: int-gpios not set in devicetree", dev->name);
		return -ENOTSUP;
	}
	if (idx < 0) {
		return idx;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	data->handler[idx] = handler;
	data->trigger[idx] = trig;
	if (handler != NULL) {
		data->trig_enable |= trig_bit[idx];
	} else {
		data->trig_enable &= ~trig_bit[idx];
		data->level_masked &= ~trig_bit[idx];
	}
	ret = max30003_write_int_enable_locked(dev);
	if (ret == 0) {
		/*
		 * Reading STATUS clears the latched flags, so the next INTB edge
		 * belongs to a new event. The flags are kept for
		 * max30003_read_status() (not lost to this internal read).
		 */
		ret = max30003_reg_read(dev, MAX30003_REG_STATUS, &status);
		if (ret == 0) {
			data->status_cache |= status;
		}
	}
	k_mutex_unlock(&data->lock);

	if (ret == 0 && gpio_pin_get_dt(&cfg->int_gpio) > 0) {
		max30003_submit(data, K_NO_WAIT);
	}
	return ret;
}

int max30003_init_interrupt(const struct device *dev)
{
	const struct max30003_config *cfg = dev->config;
	struct max30003_data *data = dev->data;
	int ret;

	data->dev = dev;
	k_work_init_delayable(&data->work, max30003_work_handler);

#ifdef CONFIG_MAX30003_TRIGGER_OWN_THREAD
	if (!max30003_work_q_started) {
		const struct k_work_queue_config wq_cfg = {.name = "max30003"};

		k_work_queue_start(&max30003_work_q, max30003_stack,
				   K_KERNEL_STACK_SIZEOF(max30003_stack),
				   CONFIG_MAX30003_THREAD_PRIORITY, &wq_cfg);
		max30003_work_q_started = true;
	}
#endif

	if (cfg->int_gpio.port == NULL) {
		return 0;
	}
	if (!gpio_is_ready_dt(&cfg->int_gpio)) {
		LOG_ERR("%s: INT GPIO not ready", dev->name);
		return -ENODEV;
	}
	ret = gpio_pin_configure_dt(&cfg->int_gpio, GPIO_INPUT);
	if (ret == 0) {
		gpio_init_callback(&data->gpio_cb, max30003_gpio_callback,
				   BIT(cfg->int_gpio.pin));
		ret = gpio_add_callback(cfg->int_gpio.port, &data->gpio_cb);
	}
	if (ret == 0) {
		ret = gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	}
	if (ret < 0) {
		LOG_ERR("%s: INT GPIO setup failed (%d)", dev->name, ret);
	}
	return ret;
}
