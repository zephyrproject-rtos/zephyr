/*
 * Copyright (c) 2026 Igalia S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_WIFI_RPI_PICO_CYW43_CYW43_CONFIGPORT_H
#define ZEPHYR_DRIVERS_WIFI_RPI_PICO_CYW43_CYW43_CONFIGPORT_H

/*************************
 * Zephyr-specific headers
 *************************/

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>

#include "rpi_pico_cyw43_wifi.h"


/****************************
 * General Macro requirements
 ****************************/

#define CYW43_EINVAL    EINVAL
#define CYW43_EIO       EIO
#define CYW43_EPERM     EPERM
#define CYW43_ETIMEDOUT ETIMEDOUT

#define CYW43_ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

/*
 * Zephyr macro for compile-time assertions, for driver code that
 * doesn't use assert.h.
 */
#ifndef static_assert
#define static_assert(expr, msg...) BUILD_ASSERT((expr), "" msg)
#endif

/*
 * Replace assert() calls in the cyw43 driver with the Zephyr assert macro.
 */
#undef assert
#define assert(x) __ASSERT_NO_MSG(x)


/**********************
 * Driver configuration
 **********************/

#define CYW43_ENABLE_WARN   1
#define CYW43_ENABLE_DEBUG  0
#define CYW43_VERBOSE_DEBUG 0

#if (CYW43_ENABLE_WARN || CYW43_ENABLE_DEBUG || CYW43_VERBOSE_DEBUG)
#define CYW43_PRINTF(...) printk(__VA_ARGS__)
#else
#define CYW43_PRINTF(...)
#endif

#if CYW43_ENABLE_WARN
#define CYW43_WARN(...) CYW43_PRINTF("[CYW43] " __VA_ARGS__)
#else
#define CYW43_WARN(...)
#endif

#if CYW43_ENABLE_DEBUG
#define CYW43_DEBUG(...) CYW43_PRINTF(__VA_ARGS__)
#else
#define CYW43_DEBUG(...)
#endif

#if CYW43_VERBOSE_DEBUG
#define CYW43_VDEBUG(...) CYW43_PRINTF(__VA_ARGS__)
#else
#define CYW43_VDEBUG(...)
#endif

#define CYW43_LWIP    0
#define CYW43_USE_SPI 1
#define CYW43_GPIO    1
#ifdef CONFIG_WIFI_RPI_PICO_CYW43_USE_FIXED_MAC_ADDRESS
#define CYW43_USE_OTP_MAC   0
#else
#define CYW43_USE_OTP_MAC   1
#endif
#define CYW43_HAL_MAC_WLAN0 0


/***********************************************************
 * Zephyr-specific definitions and  function implementations
 ***********************************************************/

/* GPIO handling */

#define CYW43_WL_GPIO_COUNT 3
#define CYW43_NUM_GPIOS     3

#define CYW43_PIN_WL_REG_ON 0
/*
 * This one _needs_ to be defined as a macro, since the core driver
 * checks it at preprocessor time.
 */
#define CYW43_PIN_WL_HOST_WAKE 1
#define RPI_CYW43_GPIOS        2

#define CYW43_HAL_PIN_MODE_INPUT  GPIO_INPUT
#define CYW43_HAL_PIN_MODE_OUTPUT GPIO_OUTPUT
/*
 * GPIO pull up/down config done in the device tree, if necessary. This
 * macro is used in the core driver code, define it with a dummy value.
 */
#define CYW43_HAL_PIN_PULL_NONE 0

extern struct gpio_dt_spec *rp_gpio[RPI_CYW43_GPIOS];

static inline void cyw43_hal_pin_config(int pin, int mode, int pull,
					int alt)
{
	LOG_MODULE_DECLARE(rpi_pico_cyw43_drv, CONFIG_WIFI_LOG_LEVEL);

	LOG_DBG("pin: %d, mode: 0x%0x, pull: %d", pin, mode, pull);
	/*
	 * Ignore the <pull> parameter here, internal pin resistors
	 * should be defined in the device tree.
	 */
	__ASSERT_NO_MSG((mode == CYW43_HAL_PIN_MODE_INPUT ||
			mode == CYW43_HAL_PIN_MODE_OUTPUT) && alt == 0);

	gpio_pin_configure_dt(rp_gpio[pin], mode);
}

static inline void cyw43_hal_pin_low(int pin)
{
	LOG_MODULE_DECLARE(rpi_pico_cyw43_drv, CONFIG_WIFI_LOG_LEVEL);

	LOG_DBG("pin: %d", pin);
	gpio_pin_set_dt(rp_gpio[pin], 0);
}

static inline void cyw43_hal_pin_high(int pin)
{
	LOG_MODULE_DECLARE(rpi_pico_cyw43_drv, CONFIG_WIFI_LOG_LEVEL);

	LOG_DBG("pin: %d", pin);
	gpio_pin_set_dt(rp_gpio[pin], 1);
}

static inline int cyw43_hal_pin_read(int pin)
{
	LOG_MODULE_DECLARE(rpi_pico_cyw43_drv, CONFIG_WIFI_LOG_LEVEL);

	/*
	 * Note:
	 * For some reason it's necessary to set the host-wake GPIO
	 * direction to INPUT (in the SIO block registers) after it's
	 * been muxed and used for PIO and back to SIO in order to read
	 * it reliably, although the pinctrl driver doesn't touch the SIO
	 * registers.
	 */
	LOG_DBG("pin: %d", pin);
	if (pin == CYW43_PIN_WL_HOST_WAKE) {
		gpio_pin_configure_dt(rp_gpio[pin], GPIO_INPUT);
	}
	return gpio_pin_get_dt(rp_gpio[pin]);
}

/* Timing and waiting */

static inline void cyw43_delay_ms(uint32_t ms)
{
	k_sleep(K_MSEC(ms));
}

static inline void cyw43_delay_us(uint32_t us)
{
	k_sleep(K_USEC(us));
}

static inline uint32_t cyw43_hal_ticks_ms(void)
{
	return k_uptime_get_32();
}

static inline uint32_t cyw43_hal_ticks_us(void)
{
	return k_ticks_to_us_floor32(k_uptime_ticks());
}

/*
 * We don't need these macros to put the threads to sleep in this
 * implementation.
 */
#define CYW43_SDPCM_SEND_COMMON_WAIT
#define CYW43_DO_IOCTL_WAIT

/* Wlan interface */

void cyw43_hal_get_mac(int interface, uint8_t mac[6]);

/* Delayed work, scheduling, locking */

/* async_ctx included from cyw43_drv.h */
#define CYW43_THREAD_ENTER do {				\
	k_mutex_lock(&async_ctx.mutex, K_FOREVER);	\
	} while (0)
#define CYW43_THREAD_EXIT do {				\
	k_mutex_unlock(&async_ctx.mutex);		\
	} while (0)
#define CYW43_THREAD_LOCK_CHECK

#define CYW43_POST_POLL_HOOK cyw43_post_poll_hook();
#define CYW43_EVENT_POLL_HOOK k_yield();

/* We're using Zephyr threads so we won't need this */
static inline void cyw43_schedule_internal_poll_dispatch(void (*func)(void))
{
}

#endif /* ZEPHYR_DRIVERS_WIFI_RPI_PICO_CYW43_CYW43_CONFIGPORT_H */
