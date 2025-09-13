/*
 * Copyright (c) 2022, Carlo Caione <ccaione@baylibre.com>
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/linker/sections.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>

#include <zephyr/arch/common/pm_s2ram.h>
#include <zephyr/arch/arm/cortex_m/scb.h>
#include <zephyr/arch/arm/cortex_m/fpu.h>

BUILD_ASSERT(CONFIG_MP_MAX_NUM_CPUS == 1,
	"Suspend-to-RAM not yet supported on multi-core SoCs");

#if DT_NODE_EXISTS(DT_NODELABEL(pm_s2ram)) &&\
	DT_NODE_HAS_COMPAT(DT_NODELABEL(pm_s2ram), zephyr_memory_region)

/* Linker section name is given by `zephyr,memory-region` property of
 * `zephyr,memory-region` compatible DT node with nodelabel `pm_s2ram`.
 */
#define __s2ram_ctx Z_GENERIC_SECTION(DT_PROP(DT_NODELABEL(pm_s2ram), zephyr_memory_region))
#else
#define __s2ram_ctx __noinit
#endif

/* CPU state preserved across S2RAM */
__s2ram_ctx _cpu_context_t _cpu_context;
__s2ram_ctx struct scb_context _scb_context;
__s2ram_ctx struct z_mpu_context_retained _mpu_context;
IF_ENABLED(CONFIG_FPU, (__s2ram_ctx struct fpu_ctx_full _fpu_context;))
/* TODO: assert section (if selected) is large enough */

/**
 * These functions are helpers invoked from `pm_s2ram.S`
 * to save/restore CPU state other than general-purpose
 * and special registers (which are handled in assembly)
 */
void z_arm_pm_s2ram_save_additional_state(void)
{
	z_arm_save_scb_context(&_scb_context);
	z_arm_save_mpu_context(&_mpu_context);
	IF_ENABLED(CONFIG_FPU, (z_arm_save_fp_context(&_fpu_context);))
}

void z_arm_pm_s2ram_restore_additional_state(void)
{
	z_arm_restore_scb_context(&_scb_context);
	z_arm_restore_mpu_context(&_mpu_context);
	IF_ENABLED(CONFIG_FPU, z_arm_restore_fp_context(&_fpu_context));
}

#ifndef CONFIG_PM_S2RAM_CUSTOM_MARKING
/**
 * S2RAM Marker
 */
static __noinit uint32_t marker;

#define MAGIC (0xDABBAD00)

void pm_s2ram_mark_set(void)
{
	marker = MAGIC;
}

bool pm_s2ram_mark_check_and_clear(void)
{
	if (marker == MAGIC) {
		marker = 0;

		return true;
	}

	return false;
}

#endif /* CONFIG_PM_S2RAM_CUSTOM_MARKING */
