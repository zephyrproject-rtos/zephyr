/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */
#include <cmsis_core.h>
#include <stm32_ll_bus.h>

#include <stdint.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

static void stm32h7_flush_64b_ram_ecc(mem_addr_t addr)
{
	uint64_t word = sys_read64(addr);
	sys_write8(word & BIT_MASK(8), addr);
	sys_write64(word, addr);
}

static void stm32h7_flush_32b_ram_ecc(mem_addr_t addr)
{
	uint32_t word = sys_read32(addr);
	sys_write8(word & BIT_MASK(8), addr);
	sys_write32(word, addr);
}

#define FLUSH_RAM_IF_ENABLED(addr, flush_method)					\
	IF_ENABLED(DT_NODE_HAS_STATUS_OKAY(DT_PATH(soc, CONCAT(memory_, addr))),	\
		(flush_method(CONCAT(0x, addr))))

static void stm32h7_flush_sram_ecc(void)
{
	/*
	 * SRAM can only be written using word-sized accesses (due to ECC).
	 * The SRAM bus interface hides this HW limitation by implementing
	 * a one-word cache used for sub-word-sized accesses but this cache
	 * is not flushed before reset...
	 *
	 * To flush the cache:
	 *  (1) read any word from target SRAM using word-size access
	 *  (2) write back the word's LSB using an 8-bit sized access
	 *      - this ensures the in-cache value is known (= word we just read)
	 *  (3) write back the word using native size
	 *      - this ensures that the in-cache value is flushed to memory
	 *        (the word we read in (1) may have been the one already in cache!)
	 *
	 * Refer to AN5342 for more details.
	 *
	 * Note: STM32H7Ax/STM32H7Bx SoCs have ECC only on ICTM/DTCM.
	 */

	/* 64-bit ITCM and 32-bit DTCM, common to all STM32H7x SoCs */
	FLUSH_RAM_IF_ENABLED(0, stm32h7_flush_64b_ram_ecc);
	FLUSH_RAM_IF_ENABLED(20000000, stm32h7_flush_32b_ram_ecc);

#if !defined(CONFIG_SOC_STM32H7A3XX) && !defined(CONFIG_SOC_STM32H7A3XXQ)	\
    && !defined(CONFIG_SOC_STM32H7B0XX) && !defined(CONFIG_SOC_STM32H7B0XXQ)	\
    && !defined(CONFIG_SOC_STM32H7B3XX) && !defined(CONFIG_SOC_STM32H7B3XXQ)
	/*
	 * Enable SRAM and Backup RAM AHB interfaces.
	 * The system will be reset soon after so
	 * don't bother disabling clocks afterwards.
	 */
#if !defined(LL_AHB2_GRP1_PERIPH_D2SRAM3)
#define LL_AHB2_GRP1_PERIPH_D2SRAM3 0
#endif /* LL_AHB2_GRP1_PERIPH_D2SRAM3 */
	LL_AHB2_GRP1_EnableClock(
		LL_AHB2_GRP1_PERIPH_D2SRAM1 |
		LL_AHB2_GRP1_PERIPH_D2SRAM2 |
		LL_AHB2_GRP1_PERIPH_D2SRAM3);
	LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_BKPRAM);

	/* 64-bit "D1 domain AXI SRAM" */
	FLUSH_RAM_IF_ENABLED(24000000, stm32h7_flush_64b_ram_ecc);

	/* Use FOR_EACH for all 32-bit memories */
	FOR_EACH_FIXED_ARG(FLUSH_RAM_IF_ENABLED, (;),
		stm32h7_flush_32b_ram_ecc,
		20000000, 	/* DTCM */
		30000000,	/* D2 domain AHB SRAM1 (all SoCs)  */
		30004000,	/* D2 domain AHB SRAM2 (H72x/H73x) */
		30020000,	/* D2 domain AHB SRAM2 (H74x/H75x) */
		30040000,	/* D2 domain AHB SRAM3 (H74x/H75x) */
		38000000,	/* D3 domain AHB SRAM4 (all SoCs)  */
		38800000,	/* Backup RAM */
	);
#endif /* !CONFIG_SOC_STM32H7A* && !CONFIG_SOC_STM32H7B* */
}

/*
 * Override weak sys_arch_reboot() from Cortex-M arch code
 * (arch/arm/core/cortex_m/scb.c) to flush ECC cache of all
 * enabled memories before actually resetting the system.
 */
FUNC_NORETURN void sys_arch_reboot(int type)
{
	stm32h7_flush_sram_ecc();

	NVIC_SystemReset();

	while (1) {
		arch_nop();
	}
}
