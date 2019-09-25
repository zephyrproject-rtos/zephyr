/*
 * Copyright (C) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Include esp-idf headers first to avoid redefining BIT() macro */
#include <soc/rtc_cntl_reg.h>
#include <soc/timer_group_reg.h>

#include <soc.h>
#include <string.h>
#include <drivers/watchdog.h>
#include <device.h>

enum wdt_mode {
	WDT_MODE_RESET = 0,
	WDT_MODE_INTERRUPT_RESET
};

struct wdt_esp32_data {
	u32_t timeout;
	enum wdt_mode mode;
	wdt_callback_t callback;
	struct device *dev;
};

static struct wdt_esp32_data shared_data;

/* ESP32 ignores writes to any register if WDTWPROTECT doesn't contain the
 * magic value of TIMG_WDT_WKEY_VALUE.  The datasheet recommends unsealing,
 * making modifications, and sealing for every watchdog modification.
 */
static inline void wdt_esp32_seal(void)
{
	volatile u32_t *reg = (u32_t *)TIMG_WDTWPROTECT_REG(1);

	*reg = 0U;
}

static inline void wdt_esp32_unseal(void)
{
	volatile u32_t *reg = (u32_t *)TIMG_WDTWPROTECT_REG(1);

	*reg = TIMG_WDT_WKEY_VALUE;
}

static void wdt_esp32_enable(void)
{
	volatile u32_t *reg = (u32_t *)TIMG_WDTCONFIG0_REG(1);

	wdt_esp32_unseal();
	*reg |= BIT(TIMG_WDT_EN_S);
	wdt_esp32_seal();
}

static int wdt_esp32_disable(struct device *dev)
{
	volatile u32_t *reg = (u32_t *)TIMG_WDTCONFIG0_REG(1);

	ARG_UNUSED(dev);

	wdt_esp32_unseal();
	*reg &= ~BIT(TIMG_WDT_EN_S);
	wdt_esp32_seal();

	return 0;
}

static void adjust_timeout(u32_t timeout)
{
	volatile u32_t *reg;
	u32_t ticks = timeout;

	 reg = (u32_t *)TIMG_WDTCONFIG1_REG(1);
	/* MWDT ticks every 12.5ns.  Set the prescaler to 40000, so the
	 * counter for each watchdog stage is decremented every 0.5ms.
	 */
	 *reg = 40000U;

	 reg = (u32_t *)TIMG_WDTCONFIG2_REG(1);
	 *reg = ticks;

	 reg = (u32_t *)TIMG_WDTCONFIG3_REG(1);
	 *reg = ticks;
}

static void wdt_esp32_isr(void *param);

static int wdt_esp32_reload(struct device *dev, int channel_id)
{
	volatile u32_t *reg = (u32_t *)TIMG_WDTFEED_REG(1);

	ARG_UNUSED(dev);

	wdt_esp32_unseal();
	*reg = 0xABAD1DEA; /* Writing any value to WDTFEED will reload it. */
	wdt_esp32_seal();

	return 0;
}

static void set_interrupt_enabled(bool setting)
{
	volatile u32_t *intr_enable_reg = (u32_t *)TIMG_INT_ENA_TIMERS_REG(1);
	volatile u32_t *intr_clear_timers = (u32_t *)TIMG_INT_CLR_TIMERS_REG(1);

	*intr_clear_timers |= TIMG_WDT_INT_CLR;

	if (setting) {
		*intr_enable_reg |= TIMG_WDT_INT_ENA;

		IRQ_CONNECT(CONFIG_WDT_ESP32_IRQ, 4, wdt_esp32_isr,
			    &shared_data, 0);
		irq_enable(CONFIG_WDT_ESP32_IRQ);
	} else {
		*intr_enable_reg &= ~TIMG_WDT_INT_ENA;

		irq_disable(CONFIG_WDT_ESP32_IRQ);
	}
}

static int wdt_esp32_set_config(struct device *dev, u8_t options)
{
	struct wdt_esp32_data *data = dev->driver_data;
	volatile u32_t *reg = (u32_t *)TIMG_WDTCONFIG0_REG(1);
	u32_t v;

	if (!data) {
		return -EINVAL;
	}

	v = *reg;

	/* Stages 3 and 4 are not used: disable them. */
	v |= TIMG_WDT_STG_SEL_OFF<<TIMG_WDT_STG2_S;
	v |= TIMG_WDT_STG_SEL_OFF<<TIMG_WDT_STG3_S;

	/* Wait for 3.2us before booting again. */
	v |= 7<<TIMG_WDT_SYS_RESET_LENGTH_S;
	v |= 7<<TIMG_WDT_CPU_RESET_LENGTH_S;

	if (data->mode == WDT_MODE_RESET) {
		/* Warm reset on timeout */
		v |= TIMG_WDT_STG_SEL_RESET_SYSTEM<<TIMG_WDT_STG0_S;
		v |= TIMG_WDT_STG_SEL_OFF<<TIMG_WDT_STG1_S;

		/* Disable interrupts for this mode. */
		v &= ~(TIMG_WDT_LEVEL_INT_EN | TIMG_WDT_EDGE_INT_EN);
	} else if (data->mode == WDT_MODE_INTERRUPT_RESET) {
		/* Interrupt first, and warm reset if not reloaded */
		v |= TIMG_WDT_STG_SEL_INT<<TIMG_WDT_STG0_S;
		v |= TIMG_WDT_STG_SEL_RESET_SYSTEM<<TIMG_WDT_STG1_S;

		/* Use level-triggered interrupts. */
		v |= TIMG_WDT_LEVEL_INT_EN;
		v &= ~TIMG_WDT_EDGE_INT_EN;
	} else {
		return -EINVAL;
	}

	wdt_esp32_unseal();
	*reg = v;
	adjust_timeout(data->timeout);
	set_interrupt_enabled(data->mode == WDT_MODE_INTERRUPT_RESET);
	wdt_esp32_seal();

	wdt_esp32_reload(dev, 0);

	return 0;
}

static int wdt_esp32_install_timeout(struct device *dev,
				    const struct wdt_timeout_cfg *cfg)
{
	struct wdt_esp32_data *data = dev->driver_data;

	ARG_UNUSED(dev);

	if (cfg->flags != WDT_FLAG_RESET_SOC) {
		return -ENOTSUP;
	}

	if (cfg->window.min != 0U || cfg->window.max == 0U) {
		return -EINVAL;
	}

	data->dev = dev;

	data->timeout = cfg->window.max;

	data->mode = (cfg->callback == NULL) ?
			WDT_MODE_RESET : WDT_MODE_INTERRUPT_RESET;

	data->callback = cfg->callback;

	return 0;
}

static int wdt_esp32_init(struct device *dev)
{
#ifdef CONFIG_WDT_DISABLE_AT_BOOT
	wdt_esp32_disable(dev);
#endif

	/* This is a level 4 interrupt, which is handled by _Level4Vector,
	 * located in xtensa_vectors.S.
	 */
	irq_disable(CONFIG_WDT_ESP32_IRQ);
	esp32_rom_intr_matrix_set(0, ETS_TG1_WDT_LEVEL_INTR_SOURCE,
				  CONFIG_WDT_ESP32_IRQ);

	wdt_esp32_enable();

	return 0;
}

static const struct wdt_driver_api wdt_api = {
	.setup = wdt_esp32_set_config,
	.disable = wdt_esp32_disable,
	.install_timeout = wdt_esp32_install_timeout,
	.feed = wdt_esp32_reload
};

DEVICE_AND_API_INIT(wdt_esp32, CONFIG_WDT_0_NAME, wdt_esp32_init,
		    &shared_data, NULL,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &wdt_api);

static void wdt_esp32_isr(void *param)
{
	struct wdt_esp32_data *data = param;
	volatile u32_t *reg = (u32_t *)TIMG_INT_CLR_TIMERS_REG(1);


	if (shared_data.callback) {
		shared_data.callback(data->dev, 0);
	}

	*reg |= TIMG_WDT_INT_CLR;
}
