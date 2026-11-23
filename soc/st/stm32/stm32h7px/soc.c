/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cmsis_core.h>
#include <stm32_bitops.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_dbgmcu.h>
#include <stm32_ll_pwr.h>

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/cache.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

#define PWRC_NODE DT_INST(0, st_stm32h7p_pwr)

/*
 * Initialize power supply according to the sequence described in
 * RM0533 Rev 1 §6.4.1 "System supply startup": write to the bit
 * PWR_CSR2.BYPASS a zero or one depending on selected source.
 *
 * This hook must be implemented in assembly because it runs without a stack
 * and while the system is in Run* mode (write accesses to RAM are forbidden).
 */
__attribute__((naked))
void soc_early_reset_hook(void)
{
	/*
	 * Constants used in this function:
	 * - #0x0C = offsetof(PWR, CSR2)
	 * - #1 = PWR_CSR2_BYPASS
	 * - #0x04 = offsetof(PWR, SR1)
	 * - #4 = PWR_SR1_ACTVOSRDY
	 *
	 * Per GCC documentation, only asm blocks without operands
	 * are allowed in a naked function, so raw constants must
	 * be used instead of Extended ASM arguments.
	 */
	__asm__ volatile (
		"ldr	r0, =" STRINGIFY(DT_REG_ADDR_RAW(PWRC_NODE)) "\n"
		"ldr	r1, [r0, #0x0C]\n"
#if DT_ENUM_HAS_VALUE(PWRC_NODE, st_power_supply, external_source)
		"orrs	r1, r1, #1\n"	/* External source: BYPASS = 1*/
#else
		"bic	r1, r1, #1\n"	/* Internal regulator: BYPASS = 0 */
#endif /* DT_ENUM_HAS_VALUE(...) */
		"str	r1, [r0, #0x0C]\n"
		"1:\n"
		"ldr r1, [r0, #0x04]\n"
		"tst r1, #4\n"
		"beq 1b\n"
		"bx lr\n"
	);
}

void soc_early_init_hook(void)
{
	sys_cache_instr_enable();
	sys_cache_data_enable();

	/* Enable SBS clock for other drivers */
	LL_APB4_GRP1_EnableClock(LL_APB4_GRP1_PERIPH_SBS);

	/* Switch to VOS0 voltage scale */
	LL_PWR_SetRegulatorVoltageScaling(LL_PWR_REGU_VOLT_SCALE_0);

	while (LL_PWR_IsActiveFlag_VOS() == 0) {
		/* Wait until voltage scale change is effective */
	}
}
