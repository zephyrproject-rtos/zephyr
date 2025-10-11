/*
 * Copyright 2023 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_cat1_watchdog

#include "cy_wdt.h"
#include "cy_sysclk.h"

#include <zephyr/devicetree.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wdt_infineon_cat1, CONFIG_WDT_LOG_LEVEL);

#define IFX_CAT1_WDT_IS_IRQ_EN DT_NODE_HAS_PROP(DT_DRV_INST(0), interrupts)

typedef struct {
	/* Minimum period in milliseconds that can be represented with this
	 * many ignored bits
	 */
	uint32_t min_period_ms;
	/* Timeout threshold in milliseconds from which to round up to
	 * the minimum period
	 */
	uint32_t round_threshold_ms;
} wdt_ignore_bits_data_t;

#if defined(CY_IP_MXS40SRSS) || defined(CY_IP_MXS40SSRSS)
#define ifx_wdt_lock()   Cy_WDT_Lock()
#define ifx_wdt_unlock() Cy_WDT_Unlock()
#else
#define ifx_wdt_lock()
#define ifx_wdt_unlock()
#endif

#if defined(SRSS_NUM_WDT_A_BITS)
#define IFX_WDT_MATCH_BITS (SRSS_NUM_WDT_A_BITS)
#elif defined(COMPONENT_CAT1A)
#if defined(CY_IP_MXS40SRSS) && (CY_IP_MXS40SRSS_VERSION < 2)
#define IFX_WDT_MATCH_BITS (16)
#endif
#elif defined(COMPONENT_CAT1B)
#define IFX_WDT_MATCH_BITS (16)
#else
#error Unhandled device type
#endif

/* match range: 0 -> 2^(IFX_WDT_MATCH_BITS - ignore_bits)
 * ignore_bits range: 0 -> (IFX_WDT_MATCH_BITS - 4) (Bottom four bits cannot be ignored)
 * WDT Reset Period (timeout_ms) = CLK_DURATION * (2 * 2^(IFX_WDT_MATCH_BITS - ignore_bits) + match)
 * Max WDT Reset Period = 3 * (2^IFX_WDT_MATCH_BITS) * CLK_DURATION
 */
#if defined(CY_IP_MXS40SRSS)
/* ILO, PILO, BAK all run at 32768 Hz - Period is ~0.030518 ms */
#define IFX_WDT_MAX_TIMEOUT_MS  6000
#define IFX_WDT_MAX_IGNORE_BITS 12
/* ILO Frequency = 32768 Hz, ILO Period = 1 / 32768 Hz = .030518 ms */
static const wdt_ignore_bits_data_t ifx_wdt_ignore_data[] = {
	{4000, 3001}, /*  0 bit(s): min period: 4000ms, max period: 6000ms, round up from 3001+ms */
	{2000, 1501}, /*  1 bit(s): min period: 2000ms, max period: 3000ms, round up from 1501+ms */
	{1000, 751},  /*  2 bit(s): min period: 1000ms, max period: 1500ms, round up from 751+ms */
	{500, 376},   /*  3 bit(s): min period:  500ms, max period:  750ms, round up from 376+ms */
	{250, 188},   /*  4 bit(s): min period:  250ms, max period:  375ms, round up from 188+ms */
	{125, 94},    /*  5 bit(s): min period:  125ms, max period:  187ms, round up from 94+ms */
	{63, 47},     /*  6 bit(s): min period:   63ms, max period:   93ms, round up from 47+ms */
	{32, 24},     /*  7 bit(s): min period:   32ms, max period:   46ms, round up from 24+ms */
	{16, 12},     /*  8 bit(s): min period:   16ms, max period:   23ms, round up from 12+ms */
	{8, 6},       /*  9 bit(s): min period: 8ms, max period:   11ms, round up from 6+ms */
	{4, 3},       /* 10 bit(s): min period: 4ms, max period: 5ms, round up from 3+ms */
	{2, 2},       /* 11 bit(s): min period: 2ms, max period: 2ms, round up from 2+ms */
	{1, 1},       /* 12 bit(s): min period: 1ms, max period: 1ms, round up from 1+ms */
};
#elif defined(CY_IP_MXS40SSRSS) && (IFX_WDT_MATCH_BITS == 22)
/* ILO Frequency = 32768 Hz, ILO Period = 1 / 32768 Hz = .030518 ms */
#define IFX_WDT_MAX_TIMEOUT_MS  384000
#define IFX_WDT_MAX_IGNORE_BITS (IFX_WDT_MATCH_BITS - 4)
static const wdt_ignore_bits_data_t ifx_wdt_ignore_data[] = {
	/* 0 bit(s): min period: 256000ms, max period: 384000ms, round up from 192001+ms */
	{256000, 192001},
	/* 1 bit(s): min period: 128000ms, max period: 192000ms, round up from 96001+ms */
	{128000, 96001},
	/* 2 bit(s): min period:  64000ms, max period:  96000ms, round up from 48001+ms */
	{64000, 48001},
	/* 3 bit(s): min period:  32000ms, max period:  48000ms, round up from 24001+ms */
	{32000, 24001},
	/* 4 bit(s): min period:  16000ms, max period:  24000ms, round up from 12001+ms */
	{16000, 12001},
	/* 5 bit(s): min period:8000ms, max period:  12000ms, round up from 6001+ms */
	{8000, 6001},
	/* 6 bit(s): min period:4000ms, max period:6000ms, round up from 3001+ms */
	{4000, 3001},
	/*  7 bit(s): min period:2000ms, max period:3000ms, round up from 1501+ms */
	{2000, 1501},
	/*  8 bit(s): min period:1000ms, max period:1500ms, round up from 751+ms */
	{1000, 751},
	/*  9 bit(s): min period: 500ms, max period: 750ms, round up from 376+ms */
	{500, 376},
	/* 10 bit(s): min period: 250ms, max period: 375ms, round up from 188+ms */
	{250, 188},
	/* 11 bit(s): min period: 125ms, max period: 187ms, round up from 94+ms */
	{125, 94},
	/* 12 bit(s): min period: 63ms, max period: 93ms, round up from 47+ms */
	{63, 47},
	/* 13 bit(s): min period: 32ms, max period: 46ms, round up from 24+ms */
	{32, 24},
	/* 14 bit(s): min period: 16ms, max period: 23ms, round up from 12+ms */
	{16, 12},
	/* 15 bit(s): min period: 8ms, max period: 11ms, round up from 6+ms */
	{8, 6},
	/* 16 bit(s): min period: 4ms, max period: 5ms, round up from 3+ms */
	{4, 3},
	/* 17 bit(s): min period: 2ms, max period: 2ms, round up from 2+ms */
	{2, 2},
	/* 18 bit(s): min period: 1ms, max period: 1ms, round up from 1+ms */
	{1, 1},
};
#elif defined(CY_IP_MXS40SSRSS) && (IFX_WDT_MATCH_BITS == 32)
/* ILO Frequency = 32768 Hz, ILO Period = 1 / 32768 Hz = .030518 ms */
#define IFX_WDT_MAX_TIMEOUT_MS  393211435
#define IFX_WDT_MAX_IGNORE_BITS (IFX_WDT_MATCH_BITS - 4)
static const wdt_ignore_bits_data_t ifx_wdt_ignore_data[] = {
	/* 0 bit(s): min period: 262147000ms, max period: 393221000ms, round up from 196610001+ms */
	{262147000, 196610001},
	/* 1 bit(s): min period: 131073000ms, max period: 196610000ms, round up from 98305001+ms */
	{131073000, 98305001},
	/* 2 bit(s): min period: 65537000ms, max period: 98305000ms, round up from 49152001+ms */
	{65537000, 49152001},
	/* 3 bit(s): min period: 32768000ms, max period: 49152000ms, round up from 24576001+ms */
	{32768000, 24576001},
	/* 4 bit(s): min period: 16384000ms, max period: 24576000ms, round up from 12288001+ms */
	{16384000, 12288001},
	/* 5 bit(s): min period: 8192000ms, max period: 12288000ms, round up from 6144001+ms */
	{8192000, 6144001},
	/* 6 bit(s): min period: 4096000ms, max period: 6144000ms, round up from 3072001+ms */
	{4096000, 3072001},
	/* 7 bit(s): min period: 2048000ms, max period: 3072000ms, round up from 1536001+ms */
	{2048000, 1536001},
	/* 8 bit(s): min period: 1024000ms, max period: 1536000ms, round up from 768001+ms */
	{1024000, 768001},
	/* 9 bit(s): min period: 512000ms, max period: 768000ms, round up from 384001+ms */
	{512000, 384001},
	/* 10 bit(s): min period: 256000ms, max period: 384000ms, round up from 192001+ms */
	{256000, 192001},
	/* 11 bit(s): min period: 128000ms, max period: 192000ms, round up from 96001+ms */
	{128000, 96001},
	/* 12 bit(s): min period: 64000ms, max period: 96000ms, round up from 48001+ms */
	{64000, 48001},
	/* 13 bit(s): min period: 32000ms, max period: 48000ms, round up from 24001+ms */
	{32000, 24001},
	/* 14 bit(s): min period: 16000ms, max period: 24000ms, round up from 12001+ms */
	{16000, 12001},
	/* 15 bit(s): min period: 8000ms, max period: 12000ms, round up from 6001+ms */
	{8000, 6001},
	/* 16 bit(s): min period: 4000ms, max period: 6000ms, round up from 3001+ms */
	{4000, 3001},
	/* 17 bit(s): min period: 2000ms, max period: 3000ms, round up from 1501+ms */
	{2000, 1501},
	/* 18 bit(s): min period: 1000ms, max period: 1500ms, round up from 751+ms */
	{1000, 751},
	/* 19 bit(s): min period: 500ms, max period: 750ms, round up from 376+ms */
	{500, 376},
	/* 20 bit(s): min period: 250ms, max period: 375ms, round up from 188+ms */
	{250, 188},
	/* 21 bit(s): min period: 125ms, max period: 187ms, round up from 94+ms */
	{125, 94},
	/* 22 bit(s): min period: 63ms, max period: 93ms, round up from 47+ms */
	{63, 47},
	/* 23 bit(s): min period: 32ms, max period: 46ms, round up from 24+ms */
	{32, 24},
	/* 24 bit(s): min period: 16ms, max period: 23ms, round up from 12+ms */
	{16, 12},
	/* 25 bit(s): min period: 8ms, max period: 11ms, round up from 6+ms */
	{8, 6},
	/* 26 bit(s): min period: 4ms, max period: 5ms, round up from 3+ms */
	{4, 3},
	/* 27 bit(s): min period: 2ms, max period: 2ms, round up from 2+ms */
	{2, 2},
	/* 28 bit(s): min period: 1ms, max period: 1ms, round up from 1+ms */
	{1, 1},
};
#endif

struct ifx_cat1_wdt_data {
	bool wdt_initialized;
	uint32_t wdt_initial_timeout_ms;
	uint32_t wdt_rounded_timeout_ms;
	uint32_t wdt_ignore_bits;
#ifdef IFX_CAT1_WDT_IS_IRQ_EN
	wdt_callback_t callback;
#endif /* IFX_CAT1_WDT_IS_IRQ_EN */
	uint32_t timeout;
	bool timeout_installed;
};

static struct ifx_cat1_wdt_data wdt_data;

#define IFX_DETERMINE_MATCH_BITS(bits)      ((IFX_WDT_MAX_IGNORE_BITS) - (bits))
#define IFX_GET_COUNT_FROM_MATCH_BITS(bits) (2UL << IFX_DETERMINE_MATCH_BITS(bits))

__STATIC_INLINE uint32_t ifx_wdt_timeout_to_match(uint32_t timeout_ms, uint32_t ignore_bits)
{
	uint32_t wrap_count_for_ignore_bits = (IFX_GET_COUNT_FROM_MATCH_BITS(ignore_bits));
	uint32_t timeout_count = ((timeout_ms * CY_SYSCLK_ILO_FREQ) / 1000UL);
	/* handle multiple possible wraps of WDT counter */
	timeout_count = ((timeout_count + Cy_WDT_GetCount()) % wrap_count_for_ignore_bits);
	return timeout_count;
}

/* Rounds up *timeout_ms if it's outside of the valid timeout range (ifx_wdt_ignore_data) */
__STATIC_INLINE uint32_t ifx_wdt_timeout_to_ignore_bits(uint32_t *timeout_ms)
{
	for (uint32_t i = 0; i <= IFX_WDT_MAX_IGNORE_BITS; i++) {
		if (*timeout_ms >= ifx_wdt_ignore_data[i].round_threshold_ms) {
			if (*timeout_ms < ifx_wdt_ignore_data[i].min_period_ms) {
				*timeout_ms = ifx_wdt_ignore_data[i].min_period_ms;
			}
			return i;
		}
	}
	return IFX_WDT_MAX_IGNORE_BITS; /* Ideally should never reach this */
}

#ifdef IFX_CAT1_WDT_IS_IRQ_EN
static void ifx_cat1_wdt_isr_handler(const struct device *dev)
{
	struct ifx_cat1_wdt_data *dev_data = dev->data;

	if (dev_data->callback) {
		dev_data->callback(dev, 0);
	}
	Cy_WDT_MaskInterrupt();
}
#endif /* IFX_CAT1_WDT_IS_IRQ_EN */

static int ifx_cat1_wdt_setup(const struct device *dev, uint8_t options)
{
	struct ifx_cat1_wdt_data *dev_data = dev->data;

	/* Initialize the WDT */
	if ((dev_data->timeout == 0) || (dev_data->timeout > IFX_WDT_MAX_TIMEOUT_MS)) {
		return -ENOTSUP;
	}

	if (dev_data->wdt_initialized) {
		return -EBUSY;
	}

	/* Unlock and disable before doing other work */
	ifx_wdt_unlock();
	Cy_WDT_Disable();

	Cy_WDT_ClearInterrupt();
	Cy_WDT_MaskInterrupt();

	dev_data->wdt_initial_timeout_ms = dev_data->timeout;
#if defined(CY_IP_MXS40SRSS) && (CY_IP_MXS40SRSS_VERSION >= 2)
	Cy_WDT_SetUpperLimit(ifx_wdt_timeout_to_match(dev_data->wdt_initial_timeout_ms));
	Cy_WDT_SetUpperAction(CY_WDT_LOW_UPPER_LIMIT_ACTION_RESET);
#else
	dev_data->wdt_ignore_bits = ifx_wdt_timeout_to_ignore_bits(&dev_data->timeout);
	dev_data->wdt_rounded_timeout_ms = dev_data->timeout;
#if defined(SRSS_NUM_WDT_A_BITS) && (SRSS_NUM_WDT_A_BITS == 22)
	/* Cy_WDT_SetMatchBits configures the bit position above which the bits will be ignored for
	 * match, while ifx_wdt_timeout_to_ignore_bits returns number of timer MSB to ignore, so
	 * conversion is needed.
	 */
	Cy_WDT_SetMatchBits(IFX_DETERMINE_MATCH_BITS(dev_data->wdt_ignore_bits));
#else
	Cy_WDT_SetIgnoreBits(dev_data->wdt_ignore_bits);
#endif

#if defined(COMPONENT_CAT1) && (CY_WDT_DRV_VERSION_MAJOR > 1) && (CY_WDT_DRV_VERSION_MINOR > 6)
	/* Reset counter every time - large current counts in WDT can cause problems on some boards
	 */
	Cy_WDT_ResetCounter();
	/* Twice, as reading back after 1 reset gives same value as before single reset */
	Cy_WDT_ResetCounter();
#endif

	Cy_WDT_SetMatch(ifx_wdt_timeout_to_match(dev_data->wdt_rounded_timeout_ms,
						 dev_data->wdt_ignore_bits));
#endif

	Cy_WDT_Enable();
	ifx_wdt_lock();

	dev_data->wdt_initialized = true;

	if (!Cy_WDT_IsEnabled()) {
		return -ENOMSG;
	}

#ifdef IFX_CAT1_WDT_IS_IRQ_EN
	if (dev_data->callback) {
		Cy_WDT_UnmaskInterrupt();
		irq_enable(DT_INST_IRQN(0));
	}
#endif /* IFX_CAT1_WDT_IS_IRQ_EN */

	return 0;
}

static int ifx_cat1_wdt_disable(const struct device *dev)
{
	struct ifx_cat1_wdt_data *dev_data = dev->data;

#ifdef IFX_CAT1_WDT_IS_IRQ_EN
	Cy_WDT_MaskInterrupt();
	irq_disable(DT_INST_IRQN(0));
#endif /* IFX_CAT1_WDT_IS_IRQ_EN */

	ifx_wdt_unlock();
	Cy_WDT_Disable();
	ifx_wdt_lock();

	dev_data->wdt_initialized = false;

	return 0;
}

static int ifx_cat1_wdt_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	struct ifx_cat1_wdt_data *dev_data = dev->data;

	if (dev_data->timeout_installed) {
		LOG_ERR("No more timeouts can be installed");
		return -ENOMEM;
	}

	if (cfg->flags) {
		LOG_WRN("Watchdog behavior is not configurable.");
	}

	if (cfg->callback) {
#ifndef IFX_CAT1_WDT_IS_IRQ_EN
		LOG_WRN("Interrupt is not configured, can't set a callback.");
#else
		dev_data->callback = cfg->callback;
#endif /* IFX_CAT1_WDT_IS_IRQ_EN */
	}

	/* window watchdog not supported */
	if (cfg->window.min != 0U || cfg->window.max == 0U) {
		return -EINVAL;
	}

	dev_data->timeout = cfg->window.max;

	return 0;
}

static int ifx_cat1_wdt_feed(const struct device *dev, int channel_id)
{
	struct ifx_cat1_wdt_data *data = dev->data;

	/* Only channel 0 is supported */
	if (channel_id) {
		return -EINVAL;
	}

	ifx_wdt_unlock();
	Cy_WDT_ClearWatchdog(); /* Clear to prevent reset from WDT */
	Cy_WDT_SetMatch(
		ifx_wdt_timeout_to_match(data->wdt_rounded_timeout_ms, data->wdt_ignore_bits));
	ifx_wdt_lock();

	return 0;
}

static int ifx_cat1_wdt_init(const struct device *dev)
{
#ifdef IFX_CAT1_WDT_IS_IRQ_EN
	/* Connect WDT interrupt to ISR */
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), ifx_cat1_wdt_isr_handler,
		    DEVICE_DT_INST_GET(0), 0);
#endif /* IFX_CAT1_WDT_IS_IRQ_EN */

	return 0;
}

static DEVICE_API(wdt, ifx_cat1_wdt_api) = {
	.setup = ifx_cat1_wdt_setup,
	.disable = ifx_cat1_wdt_disable,
	.install_timeout = ifx_cat1_wdt_install_timeout,
	.feed = ifx_cat1_wdt_feed,
};

DEVICE_DT_INST_DEFINE(0, ifx_cat1_wdt_init, NULL, &wdt_data, NULL, POST_KERNEL,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &ifx_cat1_wdt_api);
