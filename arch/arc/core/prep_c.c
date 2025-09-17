/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Full C support initialization
 *
 *
 * Initialization of full C support: zero the .bss, copy the .data if XIP,
 * call z_cstart().
 *
 * Stack is available in this module, but not the global data/bss until their
 * initialization is performed.
 */

#include <zephyr/types.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/arch/arc/v2/aux_regs.h>
#include <zephyr/arch/arc/cluster.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/arch/common/xip.h>
#include <zephyr/arch/common/init.h>
#include <zephyr/platform/hooks.h>
#include <zephyr/arch/cache.h>

#ifdef CONFIG_ISA_ARCV3
/* NOTE: it will be called from early C code - we must NOT use global / static variables in it! */
static void arc_cluster_scm_enable(void)
{
	unsigned int cluster_version;

	/* Check that we have cluster and its version is supported */
	cluster_version = z_arc_v2_aux_reg_read(_ARC_REG_CLN_BCR) & _ARC_CLN_BCR_VER_MAJOR_MASK;
	if (cluster_version < _ARC_REG_CLN_BCR_VER_MAJOR_ARCV3_MIN) {
		return;
	}

	/* Check that we have shared cache in cluster */
	if (!(z_arc_v2_aux_reg_read(_ARC_CLNR_BCR_0) & _ARC_CLNR_BCR_0_HAS_SCM)) {
		return;
	}

	/* Disable SCM, just in case. */
	arc_cln_write_reg_nolock(ARC_CLN_CACHE_STATUS, 0);

	/* Invalidate SCM before enabling. */
	arc_cln_write_reg_nolock(ARC_CLN_CACHE_CMD,
				 ARC_CLN_CACHE_CMD_OP_REG_INV | ARC_CLN_CACHE_CMD_INCR);
	while (arc_cln_read_reg_nolock(ARC_CLN_CACHE_STATUS) & ARC_CLN_CACHE_STATUS_BUSY) {
	}

	arc_cln_write_reg_nolock(ARC_CLN_CACHE_STATUS, ARC_CLN_CACHE_STATUS_EN);
}
#endif /* CONFIG_ISA_ARCV3 */

#ifdef __CCAC__
extern char __device_states_start[];
extern char __device_states_end[];
/**
 * @brief Clear device_states section
 *
 * This routine clears the device_states section,
 * as MW compiler marks the section with NOLOAD flag.
 */
static void dev_state_zero(void)
{
	arch_early_memset(__device_states_start, 0, __device_states_end - __device_states_start);
}
#endif

extern FUNC_NORETURN void z_cstart(void);
extern void arc_mpu_init(void);
extern void arc_secureshield_init(void);

/**
 * @brief Prepare to and run C code
 *
 * This routine prepares for the execution of and runs C code.
 */

FUNC_NORETURN void z_prep_c(void)
{
#if defined(CONFIG_SOC_PREP_HOOK)
	soc_prep_hook();
#endif

#ifdef CONFIG_ISA_ARCV3
	arc_cluster_scm_enable();
#endif

	arch_bss_zero();
#ifdef __CCAC__
	dev_state_zero();
#endif
	arch_data_copy();
#if CONFIG_ARCH_CACHE
	arch_cache_init();
#endif
#ifdef CONFIG_ARC_MPU
	arc_mpu_init();
#endif
#ifdef CONFIG_ARC_SECURE_FIRMWARE
	arc_secureshield_init();
#endif
	z_cstart();
	CODE_UNREACHABLE;
}
