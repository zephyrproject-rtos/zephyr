/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/arch/riscv/csr.h>
#include <zephyr/cache.h>
#include <zephyr/devicetree.h>
#include <zephyr/platform/hooks.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#define CCU_NODE                 DT_NODELABEL(ccu)
#define UART3_BUS_CTRL_OFFSET    0xE0CU
#define UART3_BUS_GATE           BIT(0)
#define UART3_BUS_RESET_DEASSERT BIT(16)

#define THEAD_E907_MXSTATUS            0x7c0
#define THEAD_E907_MXSTATUS_THEADISAEE BIT(22)

void arch_cache_init(void)
{
	/* Enable XuanTie instructions for the cache helpers. */
	csr_set(THEAD_E907_MXSTATUS, THEAD_E907_MXSTATUS_THEADISAEE);

	/*
	 * The remoteproc boot path supplies cacheable private DRAM with caches
	 * disabled. Native LR/SC faults on that DRAM until D-cache is enabled.
	 */
	sys_cache_data_enable();
	sys_cache_instr_enable();
}

void soc_early_init_hook(void)
{
	if (DT_NODE_HAS_STATUS(DT_NODELABEL(uart3), okay) && DT_NODE_HAS_STATUS(CCU_NODE, okay)) {
		const mem_addr_t reg = DT_REG_ADDR(CCU_NODE) + UART3_BUS_CTRL_OFFSET;

		sys_set_bits(reg, UART3_BUS_RESET_DEASSERT | UART3_BUS_GATE);
	}
}
