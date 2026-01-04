/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/sys/util.h>
#include <libpic30.h>

size_t arch_tls_stack_setup(struct k_thread *new_thread, char *stack_ptr)
{
	/* Align the memory according to the alignment requirement */
	uint32_t align = _tls_align();
	char *stack_p = stack_ptr;

	/* Calculate how many bytes were added to align the pointer */
	uint32_t align_bytes = (align - (uint32_t)(void *)(stack_p) % align);

	/* this could be optimized based on the range of values of align*/
	stack_p = (void *)((uint32_t)(void *)stack_p + align_bytes);

	/* since the dsPIC33A stack grows upwards, the current tls area is setTo the beginning of
	 * the stack pointer
	 */
	new_thread->tls = POINTER_TO_UINT(stack_p);

	/* setup the TLS data/bss area first.*/
	_init_tls(stack_p);

	return _tls_size() + align_bytes;
}
