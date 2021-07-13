/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <drivers/entropy.h>
#include <drivers/counter.h>
#include <drivers/gpio.h>
#include "busy_sim.h"
#include <sys/ring_buffer.h>

struct busy_sim {
	uint32_t idle_avg;
	uint32_t active_avg;
	uint16_t idle_delta;
	uint16_t active_delta;
	uint32_t us_tick;
	struct counter_alarm_cfg alarm_cfg;
};

struct busy_sim_config {
	const struct device *entropy;
	const struct device *counter;
	const struct device *gpio_port;
	uint8_t pin;
};

static const struct busy_sim_config config = {
	.entropy = DEVICE_DT_GET(DT_CHOSEN(zephyr_entropy)),
	.counter = DEVICE_DT_GET(DT_PARENT(DT_NODELABEL(busy_sim))),
#if DT_NODE_HAS_PROP(DT_NODELABEL(busy_sim), active_gpios)
	.gpio_port = DEVICE_DT_GET(DT_GPIO_CTLR(DT_NODELABEL(busy_sim), active_gpios)),
	.pin = DT_GPIO_PIN(DT_NODELABEL(busy_sim), active_gpios)
#endif
};

static struct busy_sim sim;

#define BUFFER_SIZE 32
RING_BUF_DECLARE(rbuf, BUFFER_SIZE);
static void rng_pool_work_handler(struct k_work *work)
{
	uint8_t *data;
	uint32_t len;

	len = ring_buf_put_claim(&rbuf, &data, BUFFER_SIZE - 1);
	if (len) {
		int err = entropy_get_entropy(config.entropy, data, len);

		if (err == 0) {
			ring_buf_put_finish(&rbuf, len);
			return;
		}
	}

	k_work_submit(work);
}

static K_WORK_DEFINE(rng_pool_work, rng_pool_work_handler);

static uint32_t get_timeout(bool idle)
{
	uint32_t avg = idle ? sim.idle_avg : sim.active_avg;
	uint32_t delta = idle ? sim.idle_delta : sim.active_delta;
	uint16_t rand_val;
	uint32_t len;

	len = ring_buf_get(&rbuf, (uint8_t *)&rand_val, sizeof(rand_val));
	if (len < sizeof(rand_val)) {
		k_work_submit(&rng_pool_work);
		rand_val = 0;
	}

	avg *= sim.us_tick;
	delta *= sim.us_tick;

	return avg - delta + 2 * (rand_val % delta);
}

static void counter_alarm_callback(const struct device *dev,
				   uint8_t chan_id, uint32_t ticks,
				   void *user_data)
{
	int err;

	sim.alarm_cfg.ticks = get_timeout(true);

	if (config.gpio_port) {
		err = gpio_pin_set(config.gpio_port, config.pin, 1);
		__ASSERT_NO_MSG(err >= 0);
	}

	/* Busy loop */
	k_busy_wait(get_timeout(false) / sim.us_tick);

	if (config.gpio_port) {
		err = gpio_pin_set(config.gpio_port, config.pin, 0);
		__ASSERT_NO_MSG(err >= 0);
	}

	err = counter_set_channel_alarm(config.counter, 0, &sim.alarm_cfg);
	__ASSERT_NO_MSG(err == 0);

}

void busy_sim_start(uint32_t active_avg, uint32_t active_delta,
		    uint32_t idle_avg, uint32_t idle_delta)
{
	int err;

	sim.active_avg = active_avg;
	sim.active_delta = active_delta;
	sim.idle_avg = idle_avg;
	sim.idle_delta = idle_delta;

	err = k_work_submit(&rng_pool_work);
	__ASSERT_NO_MSG(err >= 0);

	sim.alarm_cfg.ticks = counter_us_to_ticks(config.counter, 100);
	err = counter_set_channel_alarm(config.counter, 0, &sim.alarm_cfg);
	__ASSERT_NO_MSG(err == 0);

	err = counter_start(config.counter);
	__ASSERT_NO_MSG(err == 0);
}

void busy_sim_stop(void)
{
	int err;

	k_work_cancel(&rng_pool_work);
	err = counter_stop(config.counter);
	__ASSERT_NO_MSG(err == 0);
}

static int busy_sim_init(const struct device *unused)
{
	ARG_UNUSED(unused);
	uint32_t freq;

	if ((config.gpio_port && !device_is_ready(config.gpio_port)) ||
	    !device_is_ready(config.counter) || !device_is_ready(config.entropy)) {
		__ASSERT(0, "Devices needed by busy simulator not ready.");
		return -EIO;
	}

	if (config.gpio_port) {
		static struct gpio_dt_spec pin =
			GPIO_DT_SPEC_GET_OR(DT_NODELABEL(busy_sim), active_gpios, {0});

		gpio_pin_configure_dt(&pin, GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW);
	}

	freq = counter_get_frequency(config.counter);
	if (freq < 1000000) {
		__ASSERT(0, "Counter device has too low frequency for busy simulator.");
		return -EINVAL;
	}

	sim.us_tick = freq / 1000000;
	sim.alarm_cfg.callback = counter_alarm_callback;
	sim.alarm_cfg.flags = COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE;

	return 0;
}

SYS_INIT(busy_sim_init, APPLICATION, 0);
