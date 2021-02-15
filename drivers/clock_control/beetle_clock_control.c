/*
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT arm_cortex_m3

/**
 * @file
 * @brief Driver for Clock Control of Beetle MCUs.
 *
 * This file contains the Clock Control driver implementation for the
 * Beetle MCUs.
 */

#include <soc.h>
#include <drivers/clock_control.h>
#include <sys/util.h>
#include <drivers/clock_control/arm_clock_control.h>

#define MAINCLK_BASE_FREQ 24000000

struct beetle_clock_control_cfg_t {
	/* Clock Control ID */
	uint32_t clock_control_id;
	/* Clock control freq */
	uint32_t freq;
};

static inline void beetle_set_clock(volatile uint32_t *base,
				    uint8_t bit, enum arm_soc_state_t state)
{
	uint32_t key;

	key = irq_lock();

	switch (state) {
	case SOC_ACTIVE:
		base[0] |= (1 << bit);
		break;
	case SOC_SLEEP:
		base[2] |= (1 << bit);
		break;
	case SOC_DEEPSLEEP:
		base[4] |= (1 << bit);
		break;
	default:
		break;
	}

	irq_unlock(key);
}

static inline void beetle_ahb_set_clock_on(uint8_t bit,
					   enum arm_soc_state_t state)
{
	beetle_set_clock((volatile uint32_t *)&(__BEETLE_SYSCON->ahbclkcfg0set),
			 bit, state);
}

static inline void beetle_ahb_set_clock_off(uint8_t bit,
					    enum arm_soc_state_t state)
{
	beetle_set_clock((volatile uint32_t *)&(__BEETLE_SYSCON->ahbclkcfg0clr),
			 bit, state);
}

static inline void beetle_apb_set_clock_on(uint8_t bit,
					   enum arm_soc_state_t state)
{
	beetle_set_clock((volatile uint32_t *)&(__BEETLE_SYSCON->apbclkcfg0set),
			 bit, state);
}

static inline void beetle_apb_set_clock_off(uint8_t bit,
					    enum arm_soc_state_t state)
{
	beetle_set_clock((volatile uint32_t *)&(__BEETLE_SYSCON->apbclkcfg0clr),
			 bit, state);
}

static inline int beetle_clock_control_on(const struct device *dev,
					  clock_control_subsys_t sub_system)
{
	struct arm_clock_control_t *beetle_cc =
				(struct arm_clock_control_t *)(sub_system);

	uint8_t bit = 0U;

	switch (beetle_cc->bus) {
	case CMSDK_AHB:
		bit = (beetle_cc->device - _BEETLE_AHB_BASE) >> 12;
		beetle_ahb_set_clock_on(bit, beetle_cc->state);
		break;
	case CMSDK_APB:
		bit = (beetle_cc->device - _BEETLE_APB_BASE) >> 12;
		beetle_apb_set_clock_on(bit, beetle_cc->state);
		break;
	default:
		break;
	}

	return 0;
}

static inline int beetle_clock_control_off(const struct device *dev,
					   clock_control_subsys_t sub_system)
{
	struct arm_clock_control_t *beetle_cc =
				(struct arm_clock_control_t *)(sub_system);

	uint8_t bit = 0U;

	switch (beetle_cc->bus) {
	case CMSDK_AHB:
		bit = (beetle_cc->device - _BEETLE_AHB_BASE) >> 12;
		beetle_ahb_set_clock_off(bit, beetle_cc->state);
		break;
	case CMSDK_APB:
		bit = (beetle_cc->device - _BEETLE_APB_BASE) >> 12;
		beetle_apb_set_clock_off(bit, beetle_cc->state);
		break;
	default:
		break;
	}
	return 0;
}

static int beetle_clock_control_get_subsys_rate(const struct device *clock,
						clock_control_subsys_t sub_system,
						uint32_t *rate)
{
#ifdef CONFIG_CLOCK_CONTROL_BEETLE_ENABLE_PLL
	const struct beetle_clock_control_cfg_t * const cfg =
						clock->config;
	uint32_t nc_mainclk = beetle_round_freq(cfg->freq);

	*rate = nc_mainclk;
#else
	ARG_UNUSED(clock);
	ARG_UNUSED(sub_system);

	*rate = MAINCLK_BASE_FREQ;
#endif /* CONFIG_CLOCK_CONTROL_BEETLE_ENABLE_PLL */

	return 0;
}

static const struct clock_control_driver_api beetle_clock_control_api = {
	.on = beetle_clock_control_on,
	.off = beetle_clock_control_off,
	.get_rate = beetle_clock_control_get_subsys_rate,
};

#ifdef CONFIG_CLOCK_CONTROL_BEETLE_ENABLE_PLL
static uint32_t beetle_round_freq(uint32_t mainclk)
{
	uint32_t nc_mainclk = 0U;

	/*
	 * Verify that the frequency is in the supported range otherwise
	 * round it to the next closer one.
	 */
	if (mainclk <= BEETLE_PLL_FREQUENCY_12MHZ) {
		nc_mainclk = BEETLE_PLL_FREQUENCY_12MHZ;
	} else if (mainclk <= BEETLE_PLL_FREQUENCY_24MHZ) {
		nc_mainclk = BEETLE_PLL_FREQUENCY_24MHZ;
	} else if (mainclk <= BEETLE_PLL_FREQUENCY_36MHZ) {
		nc_mainclk = BEETLE_PLL_FREQUENCY_36MHZ;
	} else {
		nc_mainclk = BEETLE_PLL_FREQUENCY_48MHZ;
	}

	return nc_mainclk;
}

static uint32_t beetle_get_prescaler(uint32_t mainclk)
{
	uint32_t pre_mainclk = 0U;

	/*
	 * Verify that the frequency is in the supported range otherwise
	 * round it to the next closer one.
	 */
	if (mainclk <= BEETLE_PLL_FREQUENCY_12MHZ) {
		pre_mainclk = BEETLE_PLL_PRESCALER_12MHZ;
	} else if (mainclk <= BEETLE_PLL_FREQUENCY_24MHZ) {
		pre_mainclk = BEETLE_PLL_PRESCALER_24MHZ;
	} else if (mainclk <= BEETLE_PLL_FREQUENCY_36MHZ) {
		pre_mainclk = BEETLE_PLL_PRESCALER_36MHZ;
	} else {
		pre_mainclk = BEETLE_PLL_PRESCALER_48MHZ;
	}

	return pre_mainclk;
}

static int beetle_pll_enable(uint32_t mainclk)
{

	uint32_t pre_mainclk = beetle_get_prescaler(mainclk);

	/* Set PLLCTRL Register */
	__BEETLE_SYSCON->pllctrl = BEETLE_PLL_CONFIGURATION;

	/* Switch the the Main clock to PLL and set prescaler */
	__BEETLE_SYSCON->mainclk = pre_mainclk;

	while (!__BEETLE_SYSCON->pllstatus) {
		/* Wait for PLL to lock */
	}

	return 0;
}
#endif /* CONFIG_CLOCK_CONTROL_BEETLE_ENABLE_PLL */

static int beetle_clock_control_init(const struct device *dev)
{
#ifdef CONFIG_CLOCK_CONTROL_BEETLE_ENABLE_PLL
	const struct beetle_clock_control_cfg_t * const cfg =
						dev->config;

	/*
	 * Enable PLL if Beetle is configured to run at a different
	 * frequency than 24Mhz.
	 */
	if (cfg->freq != MAINCLK_BASE_FREQ) {
		beetle_pll_enable(cfg->freq);
	}
#endif /* CONFIG_CLOCK_CONTROL_BEETLE_ENABLE_PLL */

	return 0;
}

static const struct beetle_clock_control_cfg_t beetle_cc_cfg = {
	.clock_control_id = 0,
	.freq = DT_INST_PROP(0, clock_frequency),
};

/**
 * @brief Clock Control device init
 *
 */
DEVICE_DEFINE(clock_control_beetle, CONFIG_ARM_CLOCK_CONTROL_DEV_NAME,
		    &beetle_clock_control_init,
		    device_pm_control_nop,
		    NULL, &beetle_cc_cfg,
		    PRE_KERNEL_1,
		    CONFIG_CLOCK_CONTROL_BEETLE_DEVICE_INIT_PRIORITY,
		    &beetle_clock_control_api);
