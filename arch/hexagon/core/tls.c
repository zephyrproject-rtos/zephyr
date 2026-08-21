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

#include <stdint.h>
#include <stddef.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <kernel_internal.h>
#include <kernel_tls.h>
#include <zephyr/sys/util.h>

size_t arch_tls_stack_setup(struct k_thread *new_thread, char *stack_ptr)
{
	/*
	 * Allocate the TLS data/bss area on the stack, growing downward.
	 */
	stack_ptr -= z_tls_data_size();
	z_tls_copy(stack_ptr);

	/*
	 * Set the thread's TLS pointer used by the kernel.
	 * The UGP slot in the switch frame is set by arch_new_thread()
	 * after the frame is constructed (switch_handle is not yet
	 * initialized at this point).
	 */
	new_thread->tls = POINTER_TO_UINT(stack_ptr);

	return z_tls_data_size();
}
