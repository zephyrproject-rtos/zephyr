/*
 * Copyright (c) 2018, Christian Taedcke
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Common SoC initialization for the Silabs products
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <em_chip.h>
#include <em_emu.h>
#include <soc.h>
#include <soc_common.h>
#include <cmsis_core.h>
#include <sl_sleeptimer.h>

LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

#ifdef CONFIG_SILABS_GECKO_EMU_DCDC
static ALWAYS_INLINE void dcdc_init(void)
{
#if defined(CONFIG_SILABS_GECKO_EMU_DCDC_MODE_UNCONFIGURED)
	/* Nothing to do, leave DC/DC converter in unconfigured, safe state. */
#elif defined(CONFIG_SILABS_GECKO_EMU_DCDC_MODE_ON) ||                                             \
	defined(CONFIG_SILABS_GECKO_EMU_DCDC_MODE_BYPASS)
	EMU_DCDCInit_TypeDef init_cfg = EMU_DCDCINIT_DEFAULT;
#if defined(CONFIG_SILABS_GECKO_EMU_DCDC_MODE_BYPASS)
	init_cfg.dcdcMode = emuDcdcMode_Bypass;
#endif
	EMU_DCDCInit(&init_cfg);
#elif defined(CONFIG_SILABS_GECKO_EMU_DCDC_MODE_OFF)
	EMU_DCDCPowerOff();
#else
#error "Unsupported power configuration mode of the on chip DC/DC converter."
#endif
}
#endif

#ifdef CONFIG_LOG_BACKEND_SWO
static void swo_init(void)
{
	struct soc_gpio_pin pin_swo = PIN_SWO;

	/* Select HFCLK as the debug trace clock */
	CMU->DBGCLKSEL = CMU_DBGCLKSEL_DBG_HFCLK;

#if defined(_GPIO_ROUTEPEN_MASK)
	/* Enable Serial wire output pin */
	GPIO->ROUTEPEN |= GPIO_ROUTEPEN_SWVPEN;
	/* Set SWO location */
	GPIO->ROUTELOC0 = SWO_LOCATION << _GPIO_ROUTELOC0_SWVLOC_SHIFT;
#else
	GPIO->ROUTE = GPIO_ROUTE_SWOPEN | (SWO_LOCATION << 8);
#endif

	GPIO_PinModeSet(pin_swo.port, pin_swo.pin, pin_swo.mode, pin_swo.out);
}
#endif /* CONFIG_LOG_BACKEND_SWO */

/**
 * @brief Perform basic hardware initialization
 *
 * Initialize the interrupt controller device drivers.
 * Also initialize the timer device driver, if required.
 *
 * @return 0
 */
void soc_early_init_hook(void)
{
	/* handle chip errata */
	CHIP_Init();

#ifdef CONFIG_SILABS_GECKO_EMU_DCDC
	dcdc_init();
#endif

	silabs_gecko_init_clocks();

	if (IS_ENABLED(CONFIG_SILABS_GECKO_SLEEPTIMER)) {
		sl_sleeptimer_init();
	}

#ifdef CONFIG_LOG_BACKEND_SWO
	/* Configure SWO debug output */
	swo_init();
#endif
}
