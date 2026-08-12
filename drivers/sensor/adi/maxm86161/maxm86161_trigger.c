/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>

#include "maxm86161.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(MAXM86161, CONFIG_SENSOR_LOG_LEVEL);

#define TRIG_INDEX(trig_type) ((trig_type) - SENSOR_TRIG_PRIV_START)

struct maxm86161_trigger_mapping {
	uint8_t reg_index;
	uint8_t bit_mask;
	uint8_t enable_mask;
	bool is_standard;
};

static const struct maxm86161_trigger_mapping custom_trigger_map[] = {
	[TRIG_INDEX(SENSOR_TRIG_MAXM86161_ALC_OVERFLOW)] = {
		0, MAXM86161_MSK_INT_STATUS1_ALC_OVF, MAXM86161_MSK_INT_ENABLE1_ALC_OVF_EN,
		false},
	[TRIG_INDEX(SENSOR_TRIG_MAXM86161_PROXIMITY)] = {
		0, MAXM86161_MSK_INT_STATUS1_PROX_INT, MAXM86161_MSK_INT_ENABLE1_PROX_INT_EN,
		false},
	[TRIG_INDEX(SENSOR_TRIG_MAXM86161_LED_COMPB)] = {
		0, MAXM86161_MSK_INT_STATUS1_LED_COMPB, MAXM86161_MSK_INT_ENABLE1_LED_COMPB_EN,
		false},
	[TRIG_INDEX(SENSOR_TRIG_MAXM86161_SHA_DONE)] = {
		1, MAXM86161_MSK_INT_STATUS2_SHA_DONE, MAXM86161_MSK_INT_ENABLE2_SHA_DONE_EN,
		false},
};

static const struct maxm86161_trigger_mapping standard_trigger_map[] = {
	[SENSOR_TRIG_DATA_READY] = {
		0, MAXM86161_MSK_INT_STATUS1_DATA_RDY, MAXM86161_MSK_INT_ENABLE1_DATA_RDY_EN,
		true},
	[SENSOR_TRIG_FIFO_WATERMARK] = {
		0, MAXM86161_MSK_INT_STATUS1_A_FULL, MAXM86161_MSK_INT_ENABLE1_A_FULL_EN, true},
};

static void maxm86161_thread_cb(const struct device *dev)
{
	const struct maxm86161_config *config = dev->config;
	struct maxm86161_data *data = dev->data;
	uint8_t status[2] = { 0 };
	int ret;

	/* Read and clear the interrupt status registers (STATUS1, STATUS2) */
	if (data->status1_cache_ready) {
		status[0] = data->status1_cache;
		ret = maxm86161_i2c_read_byte(dev, MAXM86161_REG_INT_STATUS2, &status[1]);
		if (ret) {
			LOG_ERR("Failed to read STATUS2 register: %d", ret);
			goto re_enable;
		}
		data->status1_cache_ready = false;
	} else {
		ret = maxm86161_i2c_burst_read(dev, MAXM86161_REG_INT_STATUS1,
					       status, sizeof(status));
		if (ret < 0) {
			LOG_ERR("Failed to read interrupt status: %d", ret);
			goto re_enable;
		}
	}
	memcpy(&data->status1_cache, status, sizeof(status));

	k_mutex_lock(&data->trigger_mutex, K_FOREVER);

	/* Track proximity object-detection transitions */
	if (status[0] & MAXM86161_MSK_INT_STATUS1_PROX_INT) {
		data->prox_attr.object_detected = !data->prox_attr.object_detected;
		if (data->prox_attr.object_detected) {
			data->prox_attr.prox_transition_time = k_uptime_get();
		}
	}

	/* Die-temperature data-ready */
	if ((status[0] & MAXM86161_MSK_INT_STATUS1_DIE_TEMP_RDY) &&
	    data->die_drdy_trigger.trig_handler != NULL) {
		data->die_drdy_trigger.trig_handler(dev, data->die_drdy_trigger.trig);
	}

	/* Data-ready (common trigger slot 0) */
	if ((status[0] & MAXM86161_MSK_INT_STATUS1_DATA_RDY) &&
	    data->common_trigs[0].trig_handler != NULL) {
		data->common_trigs[0].trig_handler(dev, data->common_trigs[0].trig);
	}

	/* FIFO almost-full / watermark (common trigger slot 1) */
	if ((status[0] & MAXM86161_MSK_INT_STATUS1_A_FULL) &&
	    data->common_trigs[1].trig_handler != NULL) {
		/*
		 * Picket-fence settling: while proximity mode is active, suppress
		 * the FIFO burst that immediately follows a proximity transition
		 * since those samples are not yet stable.
		 */
		if (data->prox_attr.enabled &&
		    (k_uptime_get() - data->prox_attr.prox_transition_time) <
		    MAXM86161_PROX_SETTLE_MS) {
			maxm86161_i2c_update_byte(dev, MAXM86161_REG_FIFO_CONFIG2,
						  MAXM86161_MSK_FIFO_FLUSH,
						  true);
		} else {
			data->common_trigs[1].trig_handler(dev, data->common_trigs[1].trig);
		}
	}

	/* Custom (private) triggers */
	for (int i = 0; i < MAXM86161_CUSTOM_TRIGGER_COUNT; i++) {
		const struct maxm86161_trigger_mapping *map = &custom_trigger_map[i];

		if ((status[map->reg_index] & map->bit_mask) &&
		    data->custom_trigs[i].trig_handler != NULL) {
			data->custom_trigs[i].trig_handler(dev, data->custom_trigs[i].trig);
		}
	}

	k_mutex_unlock(&data->trigger_mutex);

re_enable:
	ret = gpio_pin_interrupt_configure_dt(&config->interrupt_gpio, GPIO_INT_EDGE_FALLING);

	__ASSERT(ret == 0, "Interrupt configuration failed");
}

static void maxm86161_gpio_callback(const struct device *dev, struct gpio_callback *cb,
				    uint32_t pins)
{
	struct maxm86161_data *data = CONTAINER_OF(cb, struct maxm86161_data, gpio_cb);
	const struct maxm86161_config *config = data->dev->config;

	gpio_pin_interrupt_configure_dt(&config->interrupt_gpio, GPIO_INT_DISABLE);
#ifdef CONFIG_MAXM86161_STREAM
	int ret;

	if (atomic_test_bit(&data->stream_mode, 0)) {
		data->status1_cache_ready = false;

		if (data->iodev_sqe != NULL) {
			maxm86161_stream_irq_handler(data->dev);
		} else {
			LOG_DBG("Stream IRQ with no pending SQE - interrupt dropped");
			ret = gpio_pin_interrupt_configure_dt(&config->interrupt_gpio,
							      GPIO_INT_EDGE_TO_ACTIVE);
			if (ret) {
				LOG_ERR("Failed to re-enable interrupt: %d", ret);
			}
		}
		return;
	}
#endif

#if defined(CONFIG_MAXM86161_TRIGGER_OWN_THREAD)
	k_sem_give(&data->gpio_sem);
#elif defined(CONFIG_MAXM86161_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

#if defined(CONFIG_MAXM86161_TRIGGER_OWN_THREAD)
/**
 * @brief Dedicated interrupt processing thread entry point.
 *
 * @param data Pointer to the driver data structure.
 */
static void maxm86161_thread(struct maxm86161_data *data)
{
	while (true) {
		k_sem_take(&data->gpio_sem, K_FOREVER);
		maxm86161_thread_cb(data->dev);
	}
}

#elif defined(CONFIG_MAXM86161_TRIGGER_GLOBAL_THREAD)
/**
 * @brief System workqueue callback for interrupt processing.
 *
 * @param work Pointer to the work item.
 */
static void maxm86161_work_cb(struct k_work *work)
{
	struct maxm86161_data *data = CONTAINER_OF(work, struct maxm86161_data, work);

	maxm86161_thread_cb(data->dev);
}
#endif

static inline bool maxm86161_is_custom_trigger(enum sensor_trigger_type type)
{
	return type >= SENSOR_TRIG_PRIV_START &&
	       type < (SENSOR_TRIG_PRIV_START + MAXM86161_CUSTOM_TRIGGER_COUNT);
}

static int maxm86161_get_trigger_mapping(enum sensor_trigger_type type,
					 const struct maxm86161_trigger_mapping **map)
{
	if (type == SENSOR_TRIG_DATA_READY || type == SENSOR_TRIG_FIFO_WATERMARK) {
		*map = &standard_trigger_map[type];
		return 0;
	}

	if (maxm86161_is_custom_trigger(type)) {
		*map = &custom_trigger_map[TRIG_INDEX(type)];
		return 0;
	}

	return -ENOTSUP;
}

static struct maxm86161_trigger *maxm86161_get_trigger_slot(struct maxm86161_data *data,
							    enum sensor_trigger_type type)
{
	if (maxm86161_is_custom_trigger(type)) {
		return &data->custom_trigs[TRIG_INDEX(type)];
	}

	switch (type) {
	case SENSOR_TRIG_DATA_READY:
		return &data->common_trigs[0];
	case SENSOR_TRIG_FIFO_WATERMARK:
		return &data->common_trigs[1];
	default:
		return NULL;
	}
}

int maxm86161_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler)
{
	struct maxm86161_data *data = dev->data;
	const struct maxm86161_config *config = dev->config;
	const struct maxm86161_trigger_mapping *map;
	struct maxm86161_trigger *slot;
	uint8_t int_en_reg;
	uint8_t enable_mask;
	int ret;

	if (!config->interrupt_gpio.port) {
		LOG_ERR("No interrupt GPIO configured");
		return -ENOTSUP;
	}

	/* Die-temperature data-ready is a dedicated interrupt source. */
	if (trig->type == SENSOR_TRIG_DATA_READY && trig->chan == SENSOR_CHAN_DIE_TEMP) {
		slot = &data->die_drdy_trigger;
		int_en_reg = MAXM86161_REG_INT_ENABLE1;
		enable_mask = MAXM86161_MSK_INT_ENABLE1_DIE_TEMP_RDY_EN;
	} else {
		ret = maxm86161_get_trigger_mapping(trig->type, &map);
		if (ret < 0) {
			LOG_ERR("Unsupported trigger type: %d", trig->type);
			return ret;
		}

		slot = maxm86161_get_trigger_slot(data, trig->type);
		if (slot == NULL) {
			return -ENOTSUP;
		}

		int_en_reg = MAXM86161_REG_INT_ENABLE1 + map->reg_index;
		enable_mask = map->enable_mask;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->interrupt_gpio, GPIO_INT_DISABLE);
	if (ret < 0) {
		LOG_ERR("Failed to disable interrupt: %d", ret);
		return ret;
	}

	slot->trig = handler ? trig : NULL;
	slot->trig_handler = handler;

	ret = maxm86161_i2c_update_byte(dev, int_en_reg, enable_mask, handler ? 1U : 0U);
	if (ret < 0) {
		LOG_ERR("Failed to update INT_ENABLE register 0x%02x: %d", int_en_reg, ret);
		return ret;
	}

	/*
	 * Proximity mode is active only while PROX_INT is enabled. Track it so
	 * settle-suppression and picket-fence handling apply at runtime.
	 */
	if (trig->type == (enum sensor_trigger_type)SENSOR_TRIG_MAXM86161_PROXIMITY) {
		data->prox_attr.enabled = (handler != NULL);
		if (!data->prox_attr.enabled) {
			data->prox_attr.object_detected = false;
			data->prox_attr.prox_transition_time = 0;
		}
	}

	return gpio_pin_interrupt_configure_dt(&config->interrupt_gpio, GPIO_INT_EDGE_FALLING);
}

int maxm86161_init_interrupt(const struct device *dev)
{
	const struct maxm86161_config *config = dev->config;
	struct maxm86161_data *data = dev->data;
	int ret = 0;

	if (!gpio_is_ready_dt(&config->interrupt_gpio)) {
		LOG_ERR("GPIO port %s is not ready", config->interrupt_gpio.port->name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->interrupt_gpio, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, maxm86161_gpio_callback,
			   BIT(config->interrupt_gpio.pin));

	ret = gpio_add_callback(config->interrupt_gpio.port, &data->gpio_cb);
	if (ret < 0) {
		return ret;
	}

	data->dev = dev;

#if defined(CONFIG_MAXM86161_TRIGGER_OWN_THREAD)
	k_sem_init(&data->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&data->thread, data->thread_stack, CONFIG_MAXM86161_THREAD_STACK_SIZE,
			(k_thread_entry_t)maxm86161_thread, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_MAXM86161_THREAD_PRIORITY), 0, K_NO_WAIT);

	k_thread_name_set(&data->thread, dev->name);
#elif defined(CONFIG_MAXM86161_TRIGGER_GLOBAL_THREAD)
	data->work.handler = maxm86161_work_cb;
#endif

	return ret;
}
