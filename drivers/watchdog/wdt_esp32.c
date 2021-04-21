/*
 * Copyright (C) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_watchdog

/* Include esp-idf headers first to avoid redefining BIT() macro */
#include <soc/rtc_cntl_reg.h>
#include <soc/timer_group_reg.h>

#include <soc.h>
#include <string.h>
#include <drivers/watchdog.h>
#include <device.h>

/* FIXME: This struct shall be removed from here, when esp32 timer driver got
 * implemented.
 * That's why the type name starts with `timer` not `wdt`
 */
struct timer_esp32_irq_regs_t {
	uint32_t *timer_int_ena;
	uint32_t *timer_int_clr;
};

struct wdt_esp32_regs_t {
	uint32_t config0;
	uint32_t config1;
	uint32_t config2;
	uint32_t config3;
	uint32_t config4;
	uint32_t config5;
	uint32_t feed;
	uint32_t wprotect;
};

enum wdt_mode {
	WDT_MODE_RESET = 0,
	WDT_MODE_INTERRUPT_RESET
};

struct wdt_esp32_data {
	uint32_t timeout;
	enum wdt_mode mode;
	wdt_callback_t callback;
};

struct wdt_esp32_config {
	void (*connect_irq)(void);
	const struct wdt_esp32_regs_t *base;
	const struct timer_esp32_irq_regs_t irq_regs;

	const struct {
		int source;
		int line;
	} irq;
};

#define DEV_CFG(dev) \
	((const struct wdt_esp32_config *const)(dev)->config)
#define DEV_DATA(dev) \
	((struct wdt_esp32_data *)(dev)->data)
#define DEV_BASE(dev) \
	((volatile struct wdt_esp32_regs_t  *)(DEV_CFG(dev))->base)

/* ESP32 ignores writes to any register if WDTWPROTECT doesn't contain the
 * magic value of TIMG_WDT_WKEY_VALUE.  The datasheet recommends unsealing,
 * making modifications, and sealing for every watchdog modification.
 */
static inline void wdt_esp32_seal(const struct device *dev)
{
	DEV_BASE(dev)->wprotect = 0U;

}

static inline void wdt_esp32_unseal(const struct device *dev)
{
	DEV_BASE(dev)->wprotect = TIMG_WDT_WKEY_VALUE;
}

static void wdt_esp32_enable(const struct device *dev)
{
	wdt_esp32_unseal(dev);
	DEV_BASE(dev)->config0 |= BIT(TIMG_WDT_EN_S);
	wdt_esp32_seal(dev);

}

static int wdt_esp32_disable(const struct device *dev)
{
	wdt_esp32_unseal(dev);
	DEV_BASE(dev)->config0 &= ~BIT(TIMG_WDT_EN_S);
	wdt_esp32_seal(dev);

	return 0;
}

static void adjust_timeout(const struct device *dev, uint32_t timeout)
{
	/* MWDT ticks every 12.5ns.  Set the prescaler to 40000, so the
	 * counter for each watchdog stage is decremented every 0.5ms.
	 */
	DEV_BASE(dev)->config1 = 40000U;
	DEV_BASE(dev)->config2 = timeout;
	DEV_BASE(dev)->config3 = timeout;
}

static void wdt_esp32_isr(const struct device *dev);

static int wdt_esp32_feed(const struct device *dev, int channel_id)
{
	wdt_esp32_unseal(dev);
	DEV_BASE(dev)->feed = 0xABAD1DEA; /* Writing any value to WDTFEED will reload it. */
	wdt_esp32_seal(dev);

	return 0;
}

static void set_interrupt_enabled(const struct device *dev, bool setting)
{
	*DEV_CFG(dev)->irq_regs.timer_int_clr |= TIMG_WDT_INT_CLR;

	if (setting) {
		*DEV_CFG(dev)->irq_regs.timer_int_ena |= TIMG_WDT_INT_ENA;
		irq_enable(DEV_CFG(dev)->irq.line);
	} else {
		*DEV_CFG(dev)->irq_regs.timer_int_ena &= ~TIMG_WDT_INT_ENA;
		irq_disable(DEV_CFG(dev)->irq.line);
	}
}

static int wdt_esp32_set_config(const struct device *dev, uint8_t options)
{
	struct wdt_esp32_data *data = DEV_DATA(dev);
	uint32_t v = DEV_BASE(dev)->config0;

	if (!data) {
		return -EINVAL;
	}

	/* Stages 3 and 4 are not used: disable them. */
	v |= TIMG_WDT_STG_SEL_OFF << TIMG_WDT_STG2_S;
	v |= TIMG_WDT_STG_SEL_OFF << TIMG_WDT_STG3_S;

	/* Wait for 3.2us before booting again. */
	v |= 7 << TIMG_WDT_SYS_RESET_LENGTH_S;
	v |= 7 << TIMG_WDT_CPU_RESET_LENGTH_S;

	if (data->mode == WDT_MODE_RESET) {
		/* Warm reset on timeout */
		v |= TIMG_WDT_STG_SEL_RESET_SYSTEM << TIMG_WDT_STG0_S;
		v |= TIMG_WDT_STG_SEL_OFF << TIMG_WDT_STG1_S;

		/* Disable interrupts for this mode. */
		v &= ~(TIMG_WDT_LEVEL_INT_EN | TIMG_WDT_EDGE_INT_EN);
	} else if (data->mode == WDT_MODE_INTERRUPT_RESET) {
		/* Interrupt first, and warm reset if not reloaded */
		v |= TIMG_WDT_STG_SEL_INT << TIMG_WDT_STG0_S;
		v |= TIMG_WDT_STG_SEL_RESET_SYSTEM << TIMG_WDT_STG1_S;

		/* Use level-triggered interrupts. */
		v |= TIMG_WDT_LEVEL_INT_EN;
		v &= ~TIMG_WDT_EDGE_INT_EN;
	} else {
		return -EINVAL;
	}

	wdt_esp32_unseal(dev);
	DEV_BASE(dev)->config0 = v;
	adjust_timeout(dev, data->timeout);
	set_interrupt_enabled(dev, data->mode == WDT_MODE_INTERRUPT_RESET);
	wdt_esp32_seal(dev);

	wdt_esp32_feed(dev, 0);

	return 0;
}

static int wdt_esp32_install_timeout(const struct device *dev,
				     const struct wdt_timeout_cfg *cfg)
{
	struct wdt_esp32_data *data = DEV_DATA(dev);

	if (cfg->flags != WDT_FLAG_RESET_SOC) {
		return -ENOTSUP;
	}

	if (cfg->window.min != 0U || cfg->window.max == 0U) {
		return -EINVAL;
	}

	data->timeout = cfg->window.max;

	data->mode = (cfg->callback == NULL) ?
		     WDT_MODE_RESET : WDT_MODE_INTERRUPT_RESET;

	data->callback = cfg->callback;

	return 0;
}

static int wdt_esp32_init(const struct device *dev)
{
#ifdef CONFIG_WDT_DISABLE_AT_BOOT
	wdt_esp32_disable(dev);
#endif

	/* This is a level 4 interrupt, which is handled by _Level4Vector,
	 * located in xtensa_vectors.S.
	 */
	irq_disable(DEV_CFG(dev)->irq.line);
	DEV_CFG(dev)->connect_irq();

	wdt_esp32_enable(dev);

	return 0;
}

static const struct wdt_driver_api wdt_api = {
	.setup = wdt_esp32_set_config,
	.disable = wdt_esp32_disable,
	.install_timeout = wdt_esp32_install_timeout,
	.feed = wdt_esp32_feed
};

#define ESP32_WDT_INIT(idx)										   \
	static void wdt_esp32_connect_irq_func##idx(void)						   \
	{												   \
		esp32_rom_intr_matrix_set(0, ETS_TG##idx##_WDT_LEVEL_INTR_SOURCE,			   \
					  CONFIG_WDT##idx##_ESP32_IRQ);					   \
		IRQ_CONNECT(CONFIG_WDT##idx##_ESP32_IRQ,						   \
			    4,										   \
			    wdt_esp32_isr,								   \
			    DEVICE_DT_INST_GET(idx),							   \
			    0);										   \
	}												   \
													   \
	static struct wdt_esp32_data wdt##idx##_data;							   \
	static struct wdt_esp32_config wdt_esp32_config##idx = {					   \
		.base = (struct wdt_esp32_regs_t *) DT_INST_REG_ADDR(idx), \
		.irq_regs = {										   \
			.timer_int_ena = (uint32_t *)TIMG_INT_ENA_TIMERS_REG(idx),				   \
			.timer_int_clr = (uint32_t *)TIMG_INT_CLR_TIMERS_REG(idx),				   \
		},											   \
		.irq = {										   \
			.source =  ETS_TG##idx##_WDT_LEVEL_INTR_SOURCE,					   \
			.line =  CONFIG_WDT##idx##_ESP32_IRQ,						   \
		},											   \
		.connect_irq = wdt_esp32_connect_irq_func##idx						   \
	};												   \
													   \
	DEVICE_DT_INST_DEFINE(idx,									   \
			    wdt_esp32_init,								   \
			    device_pm_control_nop,							   \
			    &wdt##idx##_data,								   \
			    &wdt_esp32_config##idx,							   \
			    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,				   \
			    &wdt_api)

static void wdt_esp32_isr(const struct device *dev)
{
	struct wdt_esp32_data *data = DEV_DATA(dev);

	if (data->callback) {
		data->callback(dev, 0);
	}

	*DEV_CFG(dev)->irq_regs.timer_int_clr |= TIMG_WDT_INT_CLR;
}


#if DT_NODE_HAS_STATUS(DT_NODELABEL(wdt0), okay)
ESP32_WDT_INIT(0);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(wdt1), okay)
ESP32_WDT_INIT(1);
#endif
