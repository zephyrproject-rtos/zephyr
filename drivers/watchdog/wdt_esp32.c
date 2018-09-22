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
#include <watchdog.h>
#include <device.h>

struct wdt_esp32_data {
	struct wdt_config config;
};

static struct wdt_esp32_data shared_data;

/* ESP32 ignores writes to any register if WDTWPROTECT doesn't contain the
 * magic value of TIMG_WDT_WKEY_VALUE.  The datasheet recommends unsealing,
 * making modifications, and sealing for every watchdog modification.
 */
static inline void wdt_esp32_seal(void)
{
	volatile u32_t *reg = (u32_t *)TIMG_WDTWPROTECT_REG(1);

	*reg = 0;
}

static inline void wdt_esp32_unseal(void)
{
	volatile u32_t *reg = (u32_t *)TIMG_WDTWPROTECT_REG(1);

	*reg = TIMG_WDT_WKEY_VALUE;
}

static void wdt_esp32_enable(struct device *dev)
{
	volatile u32_t *reg = (u32_t *)TIMG_WDTCONFIG0_REG(1);

	ARG_UNUSED(dev);

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
	enum wdt_clock_timeout_cycles cycles =
		(enum wdt_clock_timeout_cycles)timeout;
	u32_t ticks;

	/* The watchdog API in Zephyr was modeled after the QMSI drivers,
	 * and those were modeled after the Quark MCUs.  The possible
	 * values of enum wdt_clock_timeout_cycles maps 1:1 to what the
	 * Quark D2000 expects.  At 32MHz, the timeout value in ms is given
	 * by the following formula, according to the D2000 datasheet:
	 *
	 *                   2^(cycles + 11)
	 *      timeout_ms = ---------------
	 *                         1000
	 *
	 * (e.g. 2.048ms for 2^16 cycles, or the WDT_2_16_CYCLES value.)
	 *
	 * While this is sort of backwards (this should be given units of
	 * time and converted to what the hardware expects), try to map this
	 * value to what the ESP32 expects.  Use the same timeout value for
	 * stages 0 and 1, regardless of the configuration mode, in order to
	 * simplify things.
	 */

	 /* MWDT ticks every 12.5ns.  Set the prescaler to 40000, so the
	  * counter for each watchdog stage is decremented every 0.5ms.
	  */
	 reg = (u32_t *)TIMG_WDTCONFIG1_REG(1);
	 *reg = 40000;

	 ticks = 1<<(cycles + 2);

	 /* Correct the value: this is an integer-only approximation of
	  *    0.114074 * exp(0.67822 * cycles)
	  * Which calculates the difference in ticks from the D2000 values to
	  * the value calculated by the previous expression.
	  */
	 ticks += (1<<cycles) / 10;

	 reg = (u32_t *)TIMG_WDTCONFIG2_REG(1);
	 *reg = ticks;

	 reg = (u32_t *)TIMG_WDTCONFIG3_REG(1);
	 *reg = ticks;
}

static void wdt_esp32_isr(void *param);

static void wdt_esp32_reload(struct device *dev)
{
	volatile u32_t *reg = (u32_t *)TIMG_WDTFEED_REG(1);

	ARG_UNUSED(dev);

	wdt_esp32_unseal();
	*reg = 0xABAD1DEA; /* Writing any value to WDTFEED will reload it. */
	wdt_esp32_seal();
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

static int wdt_esp32_set_config(struct device *dev, struct wdt_config *config)
{
	struct wdt_esp32_data *data = dev->driver_data;
	volatile u32_t *reg = (u32_t *)TIMG_WDTCONFIG0_REG(1);
	u32_t v;

	if (!config) {
		return -EINVAL;
	}

	v = *reg;

	/* Stages 3 and 4 are not used: disable them. */
	v |= TIMG_WDT_STG_SEL_OFF<<TIMG_WDT_STG2_S;
	v |= TIMG_WDT_STG_SEL_OFF<<TIMG_WDT_STG3_S;

	/* Wait for 3.2us before booting again. */
	v |= 7<<TIMG_WDT_SYS_RESET_LENGTH_S;
	v |= 7<<TIMG_WDT_CPU_RESET_LENGTH_S;

	if (config->mode == WDT_MODE_RESET) {
		/* Warm reset on timeout */
		v |= TIMG_WDT_STG_SEL_RESET_SYSTEM<<TIMG_WDT_STG0_S;
		v |= TIMG_WDT_STG_SEL_OFF<<TIMG_WDT_STG1_S;

		/* Disable interrupts for this mode. */
		v &= ~(TIMG_WDT_LEVEL_INT_EN | TIMG_WDT_EDGE_INT_EN);
	} else if (config->mode == WDT_MODE_INTERRUPT_RESET) {
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
	adjust_timeout(config->timeout & WDT_TIMEOUT_MASK);
	set_interrupt_enabled(config->mode == WDT_MODE_INTERRUPT_RESET);
	wdt_esp32_seal();

	wdt_esp32_reload(dev);

	memcpy(&data->config, config, sizeof(*config));
	return 0;
}

static void wdt_esp32_get_config(struct device *dev, struct wdt_config *config)
{
	struct wdt_esp32_data *data = dev->driver_data;

	memcpy(config, &data->config, sizeof(*config));
}

static int wdt_esp32_init(struct device *dev)
{
	struct wdt_esp32_data *data = dev->driver_data;

	(void)memset(&data->config, 0, sizeof(data->config));

#ifdef CONFIG_WDT_DISABLE_AT_BOOT
	wdt_esp32_disable(dev);
#endif

	/* This is a level 4 interrupt, which is handled by _Level4Vector,
	 * located in xtensa_vectors.S.
	 */
	irq_disable(CONFIG_WDT_ESP32_IRQ);
	esp32_rom_intr_matrix_set(0, ETS_TG1_WDT_LEVEL_INTR_SOURCE,
				  CONFIG_WDT_ESP32_IRQ);

	return 0;
}

static const struct wdt_driver_api wdt_api = {
	.enable = wdt_esp32_enable,
	.disable = wdt_esp32_disable,
	.get_config = wdt_esp32_get_config,
	.set_config = wdt_esp32_set_config,
	.reload = wdt_esp32_reload
};

DEVICE_AND_API_INIT(wdt_esp32, CONFIG_WDT_0_NAME, wdt_esp32_init,
		    &shared_data, NULL,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &wdt_api);

static void wdt_esp32_isr(void *param)
{
	struct wdt_esp32_data *data = param;
	volatile u32_t *reg = (u32_t *)TIMG_INT_CLR_TIMERS_REG(1);

	if (data->config.interrupt_fn) {
		data->config.interrupt_fn(DEVICE_GET(wdt_esp32));
	}

	*reg |= TIMG_WDT_INT_CLR;
}
