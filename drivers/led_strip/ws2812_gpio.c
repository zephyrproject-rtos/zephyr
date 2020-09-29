/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT worldsemi_ws2812_gpio

#include <drivers/led_strip.h>

#include <string.h>

#define LOG_LEVEL CONFIG_LED_STRIP_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(ws2812_gpio);

#include <zephyr.h>
#include <soc.h>
#include <drivers/gpio.h>
#include <device.h>
#include <drivers/clock_control.h>
#include <drivers/clock_control/nrf_clock_control.h>

struct ws2812_gpio_data {
	const struct device *gpio;
};

struct ws2812_gpio_cfg {
	uint8_t pin;
	bool has_white;
};

static struct ws2812_gpio_data *dev_data(const struct device *dev)
{
	return dev->data;
}

static const struct ws2812_gpio_cfg *dev_cfg(const struct device *dev)
{
	return dev->config;
}

/*
 * This is hard-coded to nRF51 in two ways:
 *
 * 1. The assembly delays T1H, T0H, TxL
 * 2. GPIO set/clear
 */

/*
 * T1H: 1 bit high pulse delay: 12 cycles == .75 usec
 * T0H: 0 bit high pulse delay: 4 cycles == .25 usec
 * TxL: inter-bit low pulse delay: 8 cycles == .5 usec
 *
 * We can't use k_busy_wait() here: its argument is in microseconds,
 * and we need roughly .05 microsecond resolution.
 */
#define DELAY_T1H "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
#define DELAY_T0H "nop\nnop\nnop\nnop\n"
#define DELAY_TxL "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"

/*
 * GPIO set/clear (these make assumptions about assembly details
 * below).
 *
 * This uses OUTCLR == OUTSET+4.
 *
 * We should be able to make this portable using the results of
 * https://github.com/zephyrproject-rtos/zephyr/issues/11917.
 *
 * We already have the GPIO device stashed in ws2812_gpio_data, so
 * this driver can be used as a test case for the optimized API.
 *
 * Per Arm docs, both Rd and Rn must be r0-r7, so we use the "l"
 * constraint in the below assembly.
 */
#define SET_HIGH "str %[p], [%[r], #0]\n" /* OUTSET = BIT(LED_PIN) */
#define SET_LOW "str %[p], [%[r], #4]\n"  /* OUTCLR = BIT(LED_PIN) */

/* Send out a 1 bit's pulse */
#define ONE_BIT(base, pin) do {			\
	__asm volatile (SET_HIGH			\
			DELAY_T1H			\
			SET_LOW			\
			DELAY_TxL			\
			::				\
			[r] "l" (base),		\
			[p] "l" (pin)); } while (0)

/* Send out a 0 bit's pulse */
#define ZERO_BIT(base, pin) do {			\
	__asm volatile (SET_HIGH			\
			DELAY_T0H			\
			SET_LOW			\
			DELAY_TxL			\
			::				\
			[r] "l" (base),		\
			[p] "l" (pin)); } while (0)

static int send_buf(const struct device *dev, uint8_t *buf, size_t len)
{
	volatile uint32_t *base = (uint32_t *)&NRF_GPIO->OUTSET;
	const uint32_t val = BIT(dev_cfg(dev)->pin);
	struct onoff_manager *mgr =
		z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);
	struct onoff_client cli;
	unsigned int key;
	int rc;

	sys_notify_init_spinwait(&cli.notify);
	rc = onoff_request(mgr, &cli);
	if (rc < 0) {
		return rc;
	}

	while (sys_notify_fetch_result(&cli.notify, &rc)) {
		/* pend until clock is up and running */
	}

	key = irq_lock();

	while (len--) {
		uint32_t b = *buf++;
		int32_t i;

		/*
		 * Generate signal out of the bits, MSbit first.
		 *
		 * Accumulator maintenance and branching mean the
		 * inter-bit time will be longer than TxL, but the
		 * wp.josh.com blog post says we have at least 5 usec
		 * of slack time between bits before we risk the
		 * signal getting latched, so this will be fine as
		 * long as the compiler does something minimally
		 * reasonable.
		 */
		for (i = 7; i >= 0; i--) {
			if (b & BIT(i)) {
				ONE_BIT(base, val);
			} else {
				ZERO_BIT(base, val);
			}
		}
	}

	irq_unlock(key);

	rc = onoff_release(mgr);
	/* Returns non-negative value on success. Cap to 0 as API states. */
	rc = MIN(rc, 0);

	return rc;
}

static int ws2812_gpio_update_rgb(const struct device *dev,
				  struct led_rgb *pixels,
				  size_t num_pixels)
{
	const struct ws2812_gpio_cfg *config = dev->config;
	const bool has_white = config->has_white;
	uint8_t *ptr = (uint8_t *)pixels;
	size_t i;

	/* Convert from RGB to on-wire format (GRB or GRBW) */
	for (i = 0; i < num_pixels; i++) {
		uint8_t r = pixels[i].r;
		uint8_t g = pixels[i].g;
		uint8_t b = pixels[i].b;

		*ptr++ = g;
		*ptr++ = r;
		*ptr++ = b;
		if (has_white) {
			*ptr++ = 0; /* white channel is unused */
		}
	}

	return send_buf(dev, (uint8_t *)pixels, num_pixels * (has_white ? 4 : 3));
}

static int ws2812_gpio_update_channels(const struct device *dev,
				       uint8_t *channels,
				       size_t num_channels)
{
	LOG_ERR("update_channels not implemented");
	return -ENOTSUP;
}

static const struct led_strip_driver_api ws2812_gpio_api = {
	.update_rgb = ws2812_gpio_update_rgb,
	.update_channels = ws2812_gpio_update_channels,
};

#define WS2812_GPIO_LABEL(idx) \
	(DT_INST_LABEL(idx))
#define WS2812_GPIO_HAS_WHITE(idx) \
	(DT_INST_PROP(idx, has_white_channel) == 1)
#define WS2812_GPIO_DEV(idx) \
	(DT_INST_GPIO_LABEL(idx, in_gpios))
#define WS2812_GPIO_PIN(idx) \
	(DT_INST_GPIO_PIN(idx, in_gpios))
#define WS2812_GPIO_FLAGS(idx) \
	(DT_INST_GPIO_FLAGS(idx, in_gpios))
/*
 * The inline assembly above is designed to work on nRF51 devices with
 * the 16 MHz clock enabled.
 *
 * TODO: try to make this portable, or at least port to more devices.
 */
#define WS2812_GPIO_CLK(idx) DT_LABEL(DT_INST(0, nordic_nrf_clock))

#define WS2812_GPIO_DEVICE(idx)					\
									\
	static int ws2812_gpio_##idx##_init(const struct device *dev)	\
	{								\
		struct ws2812_gpio_data *data = dev_data(dev);		\
									\
		data->gpio = device_get_binding(WS2812_GPIO_DEV(idx));	\
		if (!data->gpio) {					\
			LOG_ERR("Unable to find GPIO controller %s",	\
				WS2812_GPIO_DEV(idx));			\
			return -ENODEV;				\
		}							\
									\
		return gpio_pin_configure(data->gpio,			\
					  WS2812_GPIO_PIN(idx),	\
					  WS2812_GPIO_FLAGS(idx) |	\
					  GPIO_OUTPUT);		\
	}								\
									\
	static struct ws2812_gpio_data ws2812_gpio_##idx##_data;	\
									\
	static const struct ws2812_gpio_cfg ws2812_gpio_##idx##_cfg = { \
		.pin = WS2812_GPIO_PIN(idx),				\
		.has_white = WS2812_GPIO_HAS_WHITE(idx),		\
	};								\
									\
	DEVICE_AND_API_INIT(ws2812_gpio_##idx, WS2812_GPIO_LABEL(idx),	\
			    ws2812_gpio_##idx##_init,			\
			    &ws2812_gpio_##idx##_data,			\
			    &ws2812_gpio_##idx##_cfg, POST_KERNEL,	\
			    CONFIG_LED_STRIP_INIT_PRIORITY,		\
			    &ws2812_gpio_api);

DT_INST_FOREACH_STATUS_OKAY(WS2812_GPIO_DEVICE)
