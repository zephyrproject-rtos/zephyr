/*
 * Copyright (c) 2019 Philippe Retornaz <philippe@shapescale.com>
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for STM32G0 processor
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/arm/aarch32/cortex_m/cmsis.h>
#include <zephyr/linker/linker-defs.h>
#include <string.h>
#if defined(SYSCFG_CFGR1_UCPD1_STROBE) || defined(SYSCFG_CFGR1_UCPD2_STROBE)
#include <stm32_ll_system.h>
#include <stm32_ll_bus.h>
#endif /* SYSCFG_CFGR1_UCPD1_STROBE || SYSCFG_CFGR1_UCPD2_STROBE */

/**
 * @brief Disable the internal Pull-Up in Dead Battery pins of UCPD peripherals
 *
 * The internal Pull-Up in Dead Battery pins of UCPD peripherals are disabled,
 * unless the UCPD driver and the corresponding peripheral is enabled. In that
 * case this will be taken care of by the driver.
 */
static void stm32g0_disable_dead_battery(void)
{
#if defined(SYSCFG_CFGR1_UCPD1_STROBE) || defined(SYSCFG_CFGR1_UCPD2_STROBE)
	uint32_t strobe = 0;

#if defined(CONFIG_USBC_TCPC_STM32)
#define DT_DRV_COMPAT st_stm32_ucpd
#define DEV_REG_ADDR_INIT(n) addr_inst[n] = DT_INST_REG_ADDR(n);
	uint32_t addr_inst[DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)];

	DT_INST_FOREACH_STATUS_OKAY(DEV_REG_ADDR_INIT);
#else
	uint32_t addr_inst[0];
#endif

#if defined(SYSCFG_CFGR1_UCPD1_STROBE)
	strobe |= LL_SYSCFG_UCPD1_STROBE;
#endif /* SYSCFG_CFGR1_UCPD1_STROBE */

#if defined(SYSCFG_CFGR1_UCPD2_STROBE)
	strobe |= LL_SYSCFG_UCPD2_STROBE;
#endif /* SYSCFG_CFGR1_UCPD2_STROBE */

	for (int n = 0; n < ARRAY_SIZE(addr_inst); n++) {
#if defined(SYSCFG_CFGR1_UCPD1_STROBE) && defined(UCPD1_BASE)
		if (addr_inst[n] == UCPD1_BASE) {
			strobe &= ~LL_SYSCFG_UCPD1_STROBE;
		}
#endif /* SYSCFG_CFGR1_UCPD1_STROBE && UCPD1_BASE */
#if defined(SYSCFG_CFGR1_UCPD2_STROBE) && defined(UCPD2_BASE)
		if (addr_inst[n] == UCPD2_BASE) {
			strobe &= ~LL_SYSCFG_UCPD2_STROBE;
		}
#endif /* SYSCFG_CFGR1_UCPD2_STROBE && UCPD2_BASE */
	}

	if (strobe != 0) {
		LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
		LL_SYSCFG_DisableDBATT(strobe);
	}
#endif /* SYSCFG_CFGR1_UCPD1_STROBE || SYSCFG_CFGR1_UCPD2_STROBE */
}

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 * So the init priority has to be 0 (zero).
 *
 * @return 0
 */
static int stm32g0_init(const struct device *arg)
{
	uint32_t key;

	ARG_UNUSED(arg);

	key = irq_lock();

	/* Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	irq_unlock(key);

	/* Update CMSIS SystemCoreClock variable (HCLK) */
	/* At reset, system core clock is set to 16 MHz from HSI */
	SystemCoreClock = 16000000;

	/* Disable the internal Pull-Up in Dead Battery pins of UCPD peripheral */
	stm32g0_disable_dead_battery();

	return 0;
}

SYS_INIT(stm32g0_init, PRE_KERNEL_1, 0);
