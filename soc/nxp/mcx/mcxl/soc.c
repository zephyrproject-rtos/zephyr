/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for nxp_mcxl family
 *
 * This module provides routines to initialize and support board-level
 * hardware for the nxp_mcxl family.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>

#if defined(CONFIG_SECOND_CORE_MCUX) && defined(CONFIG_SOC_MCXL255_CPU0)
#include <zephyr_image_info.h>

#define MCXL_AON_PER_CLK_ENABLE             0x7ee7U
#define MCXL_AON_CLOCK_STABILIZATION_CYCLES 1000000U
#define MCXL_CM0_RESET_DELAY_CYCLES         10000U
#define MCXL_AON_SRAM_CM33_BASE (DT_REG_ADDR(DT_CHOSEN(zephyr_code_cpu1_partition)) & ~BIT(28))
#endif

/* SRAM_XEN / SRAM_XEN_DP grant execute permission per SRAM bank. */
#define MCXL_SRAM_XEN_ALL_BANKS                                                 \
	(SYSCON_SRAM_XEN_RAMX0_XEN_MASK | SYSCON_SRAM_XEN_RAMX1_XEN_MASK |      \
	 SYSCON_SRAM_XEN_RAMA0_XEN_MASK | SYSCON_SRAM_XEN_RAMA1_XEN_MASK |      \
	 SYSCON_SRAM_XEN_RAMA2_XEN_MASK | SYSCON_SRAM_XEN_RAMA3_XEN_MASK |      \
	 SYSCON_SRAM_XEN_RAMB0_XEN_MASK | SYSCON_SRAM_XEN_RAMB1_XEN_MASK |      \
	 SYSCON_SRAM_XEN_RAMB2_XEN_MASK | SYSCON_SRAM_XEN_RAMB3_XEN_MASK)

/* GLIKEY index that guards SRAM_XEN_DP, and its "unlocked" SFR_LOCK code. */
#define MCXL_GLIKEY_SRAM_XEN_DP_INDEX 2U
#define MCXL_GLIKEY_SFR_UNLOCK        0xAU

void soc_reset_hook(void)
{
#if defined(CONFIG_SECOND_CORE_MCUX) && defined(CONFIG_SOC_MCXL255_CPU0)
	/*
	 * A debug connection may release CM0+ before the MBOX ISR is installed.
	 * Mask its MU interrupts until the driver enables them.
	 */
	NVIC_DisableIRQ(MU_A_TX_IRQn);
	NVIC_DisableIRQ(MU_A_RX_IRQn);
	NVIC_DisableIRQ(MU_A_INT_IRQn);
	__DSB();
#endif /* CONFIG_SECOND_CORE_MCUX && CONFIG_SOC_MCXL255_CPU0 */

	SystemInit();

#if defined(CONFIG_SECOND_CORE_MCUX) && defined(CONFIG_SOC_MCXL255_CPU0)
	/* Allow CM33 to access the non-secure aliases of AON SRAM and peripherals. */
	AHBSC__AHBSC0->AON_DOMAIN_PERIPHERAL_MEM_RULE0 = 0U;
	AHBSC__AHBSC0->AON_DOMAIN_PERIPHERAL_MEM_RULE1 = 0U;
	AHBSC__AHBSC0->AON_DOMAIN_PERIPHERAL_MEM_RULE2 = 0U;
	AHBSC__AHBSC0->AON_DOMAIN_PERIPHERAL_MEM_RULE3 = 0U;
	AHBSC__AHBSC0->AON_DOMAIN_SRAM_MEM_RULE[0] = 0U;
	__DSB();
#endif

	/* The GLIKEY write-enable state machine to unlock SRAM_XEN_DP */
	GLIKEY0->CTRL_0 = GLIKEY_CTRL_0_SFT_RST(1U) | GLIKEY_CTRL_0_WR_EN_0(2U);
	GLIKEY0->CTRL_0 = GLIKEY_CTRL_0_WRITE_INDEX(MCXL_GLIKEY_SRAM_XEN_DP_INDEX) |
			  GLIKEY_CTRL_0_WR_EN_0(2U);
	GLIKEY0->CTRL_0 = GLIKEY_CTRL_0_WRITE_INDEX(MCXL_GLIKEY_SRAM_XEN_DP_INDEX) |
			  GLIKEY_CTRL_0_WR_EN_0(1U);
	GLIKEY0->CTRL_1 = GLIKEY_CTRL_1_WR_EN_1(1U) |
			  GLIKEY_CTRL_1_SFR_LOCK(MCXL_GLIKEY_SFR_UNLOCK);
	GLIKEY0->CTRL_0 = GLIKEY_CTRL_0_WRITE_INDEX(MCXL_GLIKEY_SRAM_XEN_DP_INDEX) |
			  GLIKEY_CTRL_0_WR_EN_0(2U);
	GLIKEY0->CTRL_1 = GLIKEY_CTRL_1_SFR_LOCK(MCXL_GLIKEY_SFR_UNLOCK);
	GLIKEY0->CTRL_0 = GLIKEY_CTRL_0_WRITE_INDEX(MCXL_GLIKEY_SRAM_XEN_DP_INDEX);

	/* Enable execute permission for SRAM banks */
	SYSCON->SRAM_XEN = MCXL_SRAM_XEN_ALL_BANKS;
	SYSCON->SRAM_XEN_DP = MCXL_SRAM_XEN_ALL_BANKS;

	__DSB();
	__ISB();
}

#if defined(CONFIG_SECOND_CORE_MCUX) && defined(CONFIG_SOC_MCXL255_CPU0)

/*
 * Use the AON CGU non-secure alias. The secure alias faults because the
 * AON domain is configured as non-secure.
 */
#if defined(AON__CGU_NS)
#define MCXL_AON_CGU AON__CGU_NS
#else
#define MCXL_AON_CGU AON__CGU
#endif

/*
 * Copy one ELF segment from its flash LMA (SEGMENT_LMA_ADDRESS_n) to
 * AON SRAM through the CM33 non-secure alias.
 */
#define MEMCPY_SEGMENT(n, _)                                                                       \
	memcpy((void *)(MCXL_AON_SRAM_CM33_BASE + (SEGMENT_LMA_ADDRESS_##n) - ADJUSTED_LMA),       \
	       (const void *)(SEGMENT_LMA_ADDRESS_##n), (SEGMENT_SIZE_##n))

static int second_core_init(void)
{
	/*
	 * RST_SUB_BLK is on the AON APB bus. Enable its clock and allow the
	 * bridge to stabilize before accessing the register.
	 */
	MCXL_AON_CGU->PER_CLK_EN = MCXL_AON_PER_CLK_ENABLE;
	__DSB();

	for (volatile uint32_t i = 0U; i < MCXL_AON_CLOCK_STABILIZATION_CYCLES; i++) {
		__NOP();
	}
	__DSB();
	__ISB();

	/* A debug connection may already have released CM0+; stop it before copying. */
	MCXL_AON_CGU->RST_SUB_BLK &= ~CGU_RST_SUB_BLK_CM0P_RST_REL_MASK;
	__DSB();
	for (volatile uint32_t i = 0U; i < MCXL_CM0_RESET_DELAY_CYCLES; i++) {
		__NOP();
	}
	__DSB();

	/* Discard MU interrupts raised before CM0+ was returned to reset. */
	NVIC_ClearPendingIRQ(MU_A_TX_IRQn);
	NVIC_ClearPendingIRQ(MU_A_RX_IRQn);
	NVIC_ClearPendingIRQ(MU_A_INT_IRQn);
	__DSB();

	/* Copy cpu1 ELF segments from CM33 flash LMA to AON SRAM. */
	LISTIFY(SEGMENT_NUM, MEMCPY_SEGMENT, (;));
	__DSB();

	/* Release CM0+ while preserving the other active-low reset bits. */
	MCXL_AON_CGU->RST_SUB_BLK |= CGU_RST_SUB_BLK_CM0P_RST_REL_MASK;
	__DSB();
	__ISB();

	return 0;
}

SYS_INIT(second_core_init, PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif /* CONFIG_SECOND_CORE_MCUX && CONFIG_SOC_MCXL255_CPU0 */
