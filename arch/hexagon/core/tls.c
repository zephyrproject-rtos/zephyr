/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Hexagon Thread-Local Storage (TLS) support
 *
 * TLS uses the UGP (User Global Pointer) register, which is saved and
 * restored in the switch frame by switch.S.
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <kernel_internal.h>
#include <kernel_tls.h>
#include <zephyr/sys/util.h>
#include <switch_frame.h>

size_t arch_tls_stack_setup(struct k_thread *new_thread, char *stack_ptr)
{
	/*
	 * Allocate the TLS data/bss area on the stack, growing downward.
	 */
	stack_ptr -= z_tls_data_size();
	z_tls_copy(stack_ptr);

	/*
	 * Set the thread's TLS pointer used by the kernel.
	 */
	new_thread->tls = POINTER_TO_UINT(stack_ptr);

	/*
	 * Update UGP in the fake switch frame so the first context switch
	 * into this thread loads the correct TLS pointer via switch.S.
	 */
	uint32_t *frame = (uint32_t *)new_thread->switch_handle;

	frame[SWITCH_UGP / 4] = POINTER_TO_UINT(stack_ptr);

	return z_tls_data_size();
}
