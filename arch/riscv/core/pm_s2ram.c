/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/linker/sections.h>
#include <zephyr/sys/util.h>

#include <zephyr/arch/common/pm_s2ram.h>

#include <pm_s2ram_struct.h>

/* Refuse to build under configurations the asm in pm_s2ram.S does not
 * (yet) handle, rather than silently corrupting state at runtime.
 */
BUILD_ASSERT(!IS_ENABLED(CONFIG_SMP), "RISC-V arch_pm_s2ram_suspend() does not yet support SMP "
				      "(would need a per-hart _cpu_context).");

BUILD_ASSERT(!IS_ENABLED(CONFIG_USERSPACE),
	     "RISC-V arch_pm_s2ram_suspend() does not yet handle userspace "
	     "(no mstatus.MPP / user-stack handling on resume).");

BUILD_ASSERT(!IS_ENABLED(CONFIG_RISCV_PMP),
	     "RISC-V arch_pm_s2ram_suspend() does not save PMP CSR state "
	     "across suspend; SoCs needing PMP across resume must extend "
	     "the framework or rebuild PMP from their wake handler.");

BUILD_ASSERT(!IS_ENABLED(CONFIG_RISCV_SOC_CONTEXT_SAVE),
	     "RISC-V arch_pm_s2ram_suspend() does not invoke the "
	     "CONFIG_RISCV_SOC_CONTEXT_SAVE hook; SoC custom CSRs must "
	     "be preserved by the SoC's own suspend/resume code.");

/* Place context and marker in DT pm_s2ram region if defined; otherwise
 * __noinit (SoC linker script must map it to retained SRAM).
 */
#if DT_NODE_EXISTS(DT_NODELABEL(pm_s2ram)) &&                                                      \
	DT_NODE_HAS_COMPAT(DT_NODELABEL(pm_s2ram), zephyr_memory_region)
#define PM_S2RAM_RETAINED                                                                          \
	__attribute__((section(DT_PROP(DT_NODELABEL(pm_s2ram), zephyr_memory_region))))
#else
#define PM_S2RAM_RETAINED __noinit
#endif

PM_S2RAM_RETAINED _cpu_context_t _cpu_context;

#ifndef CONFIG_HAS_PM_S2RAM_CUSTOM_MARKING

#define MAGIC 0xDABBAD00U

static PM_S2RAM_RETAINED uint32_t marker;

void pm_s2ram_mark_set(void)
{
	marker = MAGIC;
}

bool pm_s2ram_mark_check_and_clear(void)
{
	if (marker == MAGIC) {
		marker = 0U;
		return true;
	}
	return false;
}

#endif /* CONFIG_HAS_PM_S2RAM_CUSTOM_MARKING */
