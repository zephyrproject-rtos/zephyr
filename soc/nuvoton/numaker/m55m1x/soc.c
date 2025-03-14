/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/cache.h>
#include <zephyr/drivers/clock_control/clock_control_numaker.h>
/* Hardware and starter kit includes. */
#include <NuMicro.h>

static void memory_setup(void)
{
	/* Enable SRAM1/2 functions are only available in secure mode. */
	if (SCU_IS_CPU_NS(SCU_NS) == 0) {
		uint32_t wait_cnt;

		/* To assign __HIRC value directly before BSS initialization. */
		SystemCoreClock = __HIRC;
		wait_cnt = SystemCoreClock;

		/* Unlock protected registers */
		do {
			SYS->REGLCTL = 0x59UL;
			SYS->REGLCTL = 0x16UL;
			SYS->REGLCTL = 0x88UL;
		} while (SYS->REGLCTL == 0UL);

		/* Switch SRAM1 to normal power mode */
		if (PMC->SYSRB1PC != 0) {
			PMC->SYSRB1PC = 0;
		}

		/* Switch SRAM2 to normal power mode */
		if (PMC->SYSRB2PC != 0) {
			PMC->SYSRB2PC = 0;
		}

		/* Wait SRAM1/2 power mode change finish */
		while (1) {
			if ((PMC->SYSRB1PC & PMC_SYSRB1PC_PCBUSY_Msk) == 0 &&
			    (PMC->SYSRB2PC & PMC_SYSRB2PC_PCBUSY_Msk) == 0) {
				break;
			}

			if (wait_cnt-- == 0) {
				break;
			}
		}

		/* Enable SRAM1/2 clock */
		CLK->SRAMCTL |= (CLK_SRAMCTL_SRAM1CKEN_Msk | CLK_SRAMCTL_SRAM2CKEN_Msk);
	}

#if (defined(__FPU_PRESENT) && (__FPU_PRESENT == 1U))
	/* Enable CP10 & CP11 Full Access */
	SCB->CPACR |= ((3U << 10U * 2U) | (3U << 11U * 2U));

	/* Set low-power state for PDEPU
	 * 0b00  | ON, PDEPU is not in low-power state
	 * 0b01  | ON, but the clock is off
	 * 0b10  | RET(ention)
	 * 0b11  | OFF
	 * Clear ELPSTATE, value is 0b11 on Cold reset
	 */
	PWRMODCTL->CPDLPSTATE &= ~(PWRMODCTL_CPDLPSTATE_ELPSTATE_Msk <<
				   PWRMODCTL_CPDLPSTATE_ELPSTATE_Pos);

	/* PDEPU ON with clock off, value is 0b01 */
	PWRMODCTL->CPDLPSTATE |= 0x1 << PWRMODCTL_CPDLPSTATE_ELPSTATE_Pos;
#endif

	/* Enable only if configured to do so. */
	SCB_InvalidateICache();
	sys_cache_instr_enable();

	/* Enable d-cache only if configured to do so. */
	SCB_InvalidateDCache();
	sys_cache_data_enable();
}

void soc_early_init_hook(void)
{
	/* To ensure H/W I/O buffer with correct init data in SRAM,
	 * to clean D-Cache here to let all .bss & .data flush to SRAM.
	 */
#ifdef CONFIG_DCACHE
	SCB_CleanDCache();
#endif

	SystemInit();
}

void soc_reset_hook(void)
{
	memory_setup();

	/* Unlock protected registers */
	SYS_UnlockReg();

	/* Release GPIO hold status */
	PMC_RELEASE_GPIO();

	/*
	 * -------------------
	 * Init System Clock
	 * -------------------
	 */

#if DT_NODE_HAS_PROP(DT_NODELABEL(scc), hxt)
	/* Enable/disable 4~24 MHz external crystal oscillator (HXT) */
	if (DT_ENUM_IDX(DT_NODELABEL(scc), hxt) == NUMAKER_SCC_CLKSW_ENABLE) {
		CLK_EnableXtalRC(CLK_SRCCTL_HXTEN_Msk);
		/* Wait for HXT clock ready */
		CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);
	} else if (DT_ENUM_IDX(DT_NODELABEL(scc), hxt) == NUMAKER_SCC_CLKSW_DISABLE) {
		CLK_DisableXtalRC(CLK_SRCCTL_HXTEN_Msk);
	}
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(scc), lxt)
	/* Enable/disable 32.768 kHz low-speed external crystal oscillator (LXT) */
	if (DT_ENUM_IDX(DT_NODELABEL(scc), lxt) == NUMAKER_SCC_CLKSW_ENABLE) {
		CLK_EnableXtalRC(CLK_SRCCTL_LXTEN_Msk);
		/* Wait for LXT clock ready */
		CLK_WaitClockReady(CLK_STATUS_LXTSTB_Msk);
	} else if (DT_ENUM_IDX(DT_NODELABEL(scc), lxt) == NUMAKER_SCC_CLKSW_DISABLE) {
		CLK_DisableXtalRC(CLK_SRCCTL_LXTEN_Msk);
	}
#endif

	/* Enable 12 MHz high-speed internal RC oscillator (HIRC) */
	CLK_EnableXtalRC(CLK_SRCCTL_HIRCEN_Msk);
	/* Wait for HIRC clock ready */
	CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

	/* Enable 32 KHz low-speed internal RC oscillator (LIRC) */
	CLK_EnableXtalRC(CLK_SRCCTL_LIRCEN_Msk);
	/* Wait for LIRC clock ready */
	CLK_WaitClockReady(CLK_STATUS_LIRCSTB_Msk);

#if DT_NODE_HAS_PROP(DT_NODELABEL(scc), hirc48)
	/* Enable/disable 48 MHz high-speed internal RC oscillator (HIRC48) */
	if (DT_ENUM_IDX(DT_NODELABEL(scc), hirc48) == NUMAKER_SCC_CLKSW_ENABLE) {
		CLK_EnableXtalRC(CLK_SRCCTL_HIRC48MEN_Msk);
		/* Wait for HIRC48 clock ready */
		CLK_WaitClockReady(CLK_STATUS_HIRC48MSTB_Msk);
	} else if (DT_ENUM_IDX(DT_NODELABEL(scc), hirc48) == NUMAKER_SCC_CLKSW_DISABLE) {
		CLK_DisableXtalRC(CLK_SRCCTL_HIRC48MEN_Msk);
	}
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(scc), clk_pclkdiv)
	/* Set CLK_PCLKDIV register on request */
	CLK->PCLKDIV = DT_PROP(DT_NODELABEL(scc), clk_pclkdiv);
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(scc), core_clock)
	/* Set core clock (HCLK) on request */
	CLK_SetCoreClock(DT_PROP(DT_NODELABEL(scc), core_clock));
#endif

	/*
	 * Update System Core Clock
	 * User can use SystemCoreClockUpdate() to calculate SystemCoreClock.
	 */
	SystemCoreClockUpdate();

	/* Lock protected registers */
	SYS_LockReg();
}
