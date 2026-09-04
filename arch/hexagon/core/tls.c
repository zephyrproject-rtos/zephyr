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

/*
 * Alignment of the thread-local storage block, matching the p_align of the
 * PT_TLS segment the toolchain emits (and ARCH_STACK_PTR_ALIGN).
 */
#define TLS_BLOCK_ALIGN 8

size_t arch_tls_stack_setup(struct k_thread *new_thread, char *stack_ptr)
{
	/*
	 * Hexagon puts the thread pointer at the END of the TLS block, and
	 * biases by the block size rounded up to the block alignment. The
	 * compiler emits a local-exec access as
	 *
	 *     ugp + (symbol_offset_in_tls_segment - ROUND_UP(p_memsz, p_align))
	 *
	 * so UGP has to point one past the rounded-up block, not at its
	 * start; pointing it at the start places every thread-local a whole
	 * block too low, on top of the switch frame that arch_new_thread()
	 * builds just below.
	 */
	size_t block_size = ROUND_UP(z_tls_data_size(), TLS_BLOCK_ALIGN);
	uintptr_t block;

	/*
	 * Allocate the block on the stack, growing downward, and align its
	 * base: an 8-byte thread-local at offset 0 is loaded with memd, which
	 * faults (HVM_GE_C_RMAL) on a misaligned block.
	 */
	block = ROUND_DOWN((uintptr_t)stack_ptr - block_size, TLS_BLOCK_ALIGN);
	z_tls_copy((char *)block);

	/*
	 * The UGP slot in the switch frame is set by arch_new_thread() after
	 * the frame is constructed (switch_handle is not yet initialized at
	 * this point).
	 */
	new_thread->tls = POINTER_TO_UINT(block + block_size);

	/* Report everything consumed, including any alignment padding. */
	return (size_t)((uintptr_t)stack_ptr - block);
}
