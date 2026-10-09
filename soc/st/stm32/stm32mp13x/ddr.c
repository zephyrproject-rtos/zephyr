/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/util.h>

#include <cmsis_core.h>
#include <stm32mp13xx_hal.h>

/* STM32MP13 DDR (Double Data Rate) initialization. */
#define STM32MP13_DDR_NODE     DT_COMPAT_GET_ANY_STATUS_OKAY(st_stm32mp13_ddr)
#define STM32MP13_DDR_MEMORY   DT_PHANDLE(STM32MP13_DDR_NODE, memory_region)
#define STM32MP13_DDR_BASE     DT_REG_ADDR(STM32MP13_DDR_MEMORY)
#define STM32MP13_DDR_SIZE     DT_PROP(STM32MP13_DDR_NODE, st_mem_size)
#define STM32MP13_DDRCTRL_BASE DT_REG_ADDR_BY_NAME(STM32MP13_DDR_NODE, controller)
#define STM32MP13_DDRCTRL_SIZE DT_REG_SIZE_BY_NAME(STM32MP13_DDR_NODE, controller)
#define STM32MP13_DDRPHYC_BASE DT_REG_ADDR_BY_NAME(STM32MP13_DDR_NODE, phy)
#define STM32MP13_DDRPHYC_SIZE DT_REG_SIZE_BY_NAME(STM32MP13_DDR_NODE, phy)

/* TZC (TrustZone Address Space Controller) region attribute bits. */
#define TZC_REGION_ATTR_SECURE_READ  BIT(30)
#define TZC_REGION_ATTR_SECURE_WRITE BIT(31)
#define TZC_REGION_ATTR_FILTER0      BIT(0)

BUILD_ASSERT(CONFIG_SOC_SERIES_STM32MP13X_DDR_INIT_PRIORITY > CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
	     "DDR must be initialized after the clock controller");
BUILD_ASSERT((STM32MP13_DDR_BASE & (MB(1) - 1U)) == 0U, "DDR base address must be 1 MB aligned");
BUILD_ASSERT((STM32MP13_DDR_SIZE & (MB(1) - 1U)) == 0U, "DDR size must be a multiple of 1 MB");
BUILD_ASSERT(STM32MP13_DDRCTRL_BASE == DDRCTRL_BASE,
	     "DDR controller address does not match the HAL");
BUILD_ASSERT(STM32MP13_DDRCTRL_SIZE >= sizeof(DDRCTRL_TypeDef),
	     "DDR controller range does not cover the HAL registers");
BUILD_ASSERT(STM32MP13_DDRPHYC_BASE == DDRPHYC_BASE, "DDR PHY address does not match the HAL");
BUILD_ASSERT(STM32MP13_DDRPHYC_SIZE >= sizeof(DDRPHYC_TypeDef),
	     "DDR PHY range does not cover the HAL registers");

/**
 * Permit DDR access through the TZC.
 * This function called by code configured to run as the FSBL (First-Stage Boot Loader) only.
 */
static void stm32mp13_configure_ddr_security(void)
{
	/*
	 * The STM32MP13 places security hardware between the processors and peripherals
	 * that issue memory requests and the external DDR. Initializing the DDR controller
	 * does not by itself make the memory accessible: the TZC can still reject each
	 * request according to its access policy.
	 *
	 * This first-stage image treats DDR as one shared memory pool instead of dividing it
	 * into separate security domains. Enable the clocks needed to program the memory
	 * security blocks, then open TZC filter 0 while its policy is being changed. Region 0
	 * permits secure reads and writes and non-secure access
	 * from every bus-master ID; a bus master is a processor or peripheral that can
	 * initiate a memory transfer. The attributes register holds permissions and filter
	 * selection. Closing filter 0 then enables enforcement of the new policy.
	 */
	__HAL_RCC_MCE_CLK_ENABLE(); /* Enable memory cipher register access. */
	__HAL_RCC_TZC_CLK_ENABLE(); /* Enable TZC register access. */
	/* Enable ETZPC (Enhanced TrustZone Protection Controller). */
	__HAL_RCC_ETZPC_CLK_ENABLE();

	TZC->GATE_KEEPER = 0U;            /* Open filter 0 for reconfiguration. */
	TZC->REG_ID_ACCESSO = UINT32_MAX; /* Permit every non-secure bus-master ID. */
	/* Permit secure reads and writes for region 0 on filter 0. */
	TZC->REG_ATTRIBUTESO = TZC_REGION_ATTR_SECURE_READ | TZC_REGION_ATTR_SECURE_WRITE |
			       TZC_REGION_ATTR_FILTER0;
	TZC->GATE_KEEPER = 1U; /* Close filter 0 to enforce the policy. */
}

/**
 * Change the hardware data-cache state while preserving memory contents.
 * @param enable True to enable the data cache, false to clean and disable it.
 */
static void stm32mp13_ddr_set_dcache(bool enable)
{
	uint32_t sctlr = __get_SCTLR();

	if (((sctlr & SCTLR_C_Msk) != 0U) == enable) {
		return;
	}

	if (enable) {
		L1C_InvalidateDCacheAll(); /* Discard stale lines before enabling the data cache. */
		sctlr |= SCTLR_C_Msk;
	} else {
		L1C_CleanInvalidateDCacheAll(); /* Write back dirty lines before disabling. */
		sctlr &= ~SCTLR_C_Msk;
	}

	__DSB();            /* Complete cache maintenance before changing the control register. */
	__set_SCTLR(sctlr); /* Change only the data-cache enable bit. */
	__ISB();            /* Make subsequent instructions observe the new cache state. */
}

/**
 * Configure, train, and test the DDR device.
 * @param dev unused
 * @return 0 on success, otherwise -EIO.
 */
static int stm32mp13_ddr_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	DDR_InitTypeDef ddr = {0};
	HAL_StatusTypeDef ret;
	bool dcache_enabled;

	stm32mp13_configure_ddr_security();

	/*
	 * Work around STM32Cube HAL_DDR_Init() cache-state handling. The HAL clears
	 * the data-cache bit in SCTLR (System Control Register) directly before its
	 * DDR memory tests, and several error exits can return without restoring it.
	 * Clean and disable the data cache first, then restore the entry state.
	 * Use hardware operations: the MMU (Memory Management Unit) startup enables
	 * the cache even when CONFIG_DCACHE or CONFIG_CACHE_MANAGEMENT is disabled,
	 * in which case Zephyr's sys_cache_data_*() wrappers are no-ops.
	 */
	dcache_enabled = (__get_SCTLR() & SCTLR_C_Msk) != 0U;
	stm32mp13_ddr_set_dcache(false);

	/* Run cold-boot DDR initialization and tests using the HAL (Hardware Abstraction Layer). */
	ret = HAL_DDR_Init(&ddr);

	stm32mp13_ddr_set_dcache(dcache_enabled);

	if (ret != HAL_OK) {
		return -EIO;
	}

	return 0;
}

DEVICE_DT_DEFINE(STM32MP13_DDR_NODE, stm32mp13_ddr_init, NULL, NULL, NULL, PRE_KERNEL_1,
		 CONFIG_SOC_SERIES_STM32MP13X_DDR_INIT_PRIORITY, NULL);
