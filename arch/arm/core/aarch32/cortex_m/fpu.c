/*
 * Copyright (c) 2019,2020 Linaro Limited
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/arm/aarch32/cortex_m/cmsis.h>
#include <zephyr/arch/arm/aarch32/cortex_m/fpu.h>

/**
 * @file @brief Helper functions for saving and restoring the FP context.
 *
 */

void z_arm_save_fp_context(struct fpu_ctx_full *buffer)
{
#if defined(CONFIG_FPU_SHARING)
	__ASSERT_NO_MSG(buffer != NULL);

	uint32_t CONTROL = __get_CONTROL();

	if (CONTROL & CONTROL_FPCA_Msk) {
		/* Store caller-saved and callee-saved FP registers. */
		__asm__ volatile(
			"vstmia %0, {s0-s15}\n"
			"vstmia %1, {s16-s31}\n"
			:: "r" (buffer->caller_saved), "r" (buffer->callee_saved) :
		);

		buffer->fpscr = __get_FPSCR();
		buffer->ctx_saved = true;

		/* Disable FPCA so no stacking of FP registers happens in TFM. */
		__set_CONTROL(CONTROL & ~CONTROL_FPCA_Msk);

		/* ISB is recommended after setting CONTROL. It's not needed
		 * here though, since FPCA should have no impact on instruction
		 * fetching.
		 */
	}
#endif
}

void z_arm_restore_fp_context(const struct fpu_ctx_full *buffer)
{
#if defined(CONFIG_FPU_SHARING)
	if (buffer->ctx_saved) {
		/* Set FPCA first so it is set even if an interrupt happens
		 * during restoration.
		 */
		__set_CONTROL(__get_CONTROL() | CONTROL_FPCA_Msk);

		/* Restore FP state. */
		__set_FPSCR(buffer->fpscr);

		__asm__ volatile(
			"vldmia %0, {s0-s15}\n"
			"vldmia %1, {s16-s31}\n"
			:: "r" (buffer->caller_saved), "r" (buffer->callee_saved) :
		);
	}
#endif
}
