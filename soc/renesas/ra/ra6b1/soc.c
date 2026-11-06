/*
 * SPDX-FileCopyrightText: 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/init.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/platform/hooks.h>
#include <cmsis_core.h>

#include <bsp_api.h>

#define DEADBEEF_ADDR (0x23010000)
#define DBG_DELAY_ITER (0x15000)
LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

extern void SystemInit(void);
extern void Wakeup_Reset_Handler(void);

void sys_arch_reboot(int type)
{
	if (type == SYS_REBOOT_WARM) {
		NVIC_SystemReset();
	} else if (type == SYS_REBOOT_COLD) {
		if (WDTSYS->WDTSYS_REG_b.WDTSYS_VAL_NEG == 0) {
			/* Cannot write WATCHDOG_REG while WRITE_BUSY */
			while (1U == WDTSYS->WDTSYS_CTRL_REG_b.WDTSYS_WRITE_BUSY) {
			}
			/* Write WATCHDOG_REG */
			WDTSYS->WDTSYS_REG = BIT(WDTSYS_WDTSYS_REG_WDTSYS_VAL_Pos);

			R_BSP_PeripheralUnFreeze(BSP_FREEZE_PERIPHERAL_SYS_WDOG);
			WDTSYS->WDTSYS_CTRL_REG_b.WDTSYS_FREEZE_EN = 0U;
		}
		/* Wait */
		for (;;) {
			__NOP();
		}
	}
}

void soc_early_reset_hook(void)
{
	const uint32_t deadbeef[] = {0xDEADBEEF, 0xDEADBEEF, 0xDEADBEEF, 0xDEAD10CC};

	volatile uint32_t * const deadbeef_address = (volatile uint32_t * const)DEADBEEF_ADDR;

	/* Make sure that WDT is freezed until enabled, explicitly */
	R_BSP_PeripheralFreeze(BSP_FREEZE_PERIPHERAL_SYS_WDOG);
	WDTSYS->WDTSYS_REG = WDTSYS_WDTSYS_REG_WDTSYS_VAL_Msk;

	/*
	 * If the Magic Word {0xDEADBEEF, 0xDEADBEEF, 0xDEADBEEF, 0xDEAD10CC} is found at 0x23010000
	 * then the execution will block for a while in order to give time to a debugger to attach.
	 */
	if ((deadbeef[0] == *deadbeef_address) &&
	    (deadbeef[1] == *(deadbeef_address + 1)) &&
	    (deadbeef[2] == *(deadbeef_address + 2)) &&
	    (deadbeef[3] == *(deadbeef_address + 3))) {

		volatile uint32_t loops = DBG_DELAY_ITER;

		do {
			loops--;
		} while (loops > 0);
		*deadbeef_address = 0;
	}

	/*
	 * These are platform-specific registers that are used to indicate
	 * the relocated IVT table when sleep is enabled.
	 */
	SYSB->INITSVTOR_REG = (uintptr_t)_sram_vector_start & SCB_VTOR_TBLOFF_Msk;
}

void soc_early_init_hook(void)
{
	SystemInit();
	/* FSP sets MSPLIM register. Clear it here to allow zephyr to handle it. */
	__set_MSPLIM(0);
#if FSP_PRIV_TZ_USE_SECURE_REGS
	/* The secure Attribute managed within the ARM CPU NVIC must match the
	 * security attribution of IELSEn registers
	 */
	for (int i = 0; i < CONFIG_NUM_IRQS; i++) {
		uint32_t index = i / NUM_BITS(uint32_t);
		uint32_t bit   = i % NUM_BITS(uint32_t);

		NVIC->ITNS[index] &= ~(1U << bit);
	}
#endif
}
