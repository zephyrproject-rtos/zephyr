/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
 * @file common dynamic irq/exception-relation functions for IA-32 arch
 */

#include <nanokernel.h>
#include <arch/cpu.h>
#include <nano_private.h>

#if ALL_DYN_STUBS > 0

/**
 * @brief Allocate dynamic interrupt stub
 *
 * @param sp Pointer to integer tracking stub allocation
 * @param limit Max number of stubs to allow
 * @return index of the first available element of the STUB array or -1
 *          if all elements are used
 */
int _stub_alloc(unsigned int *sp, unsigned int limit)
{
	int rv, key;

	key = irq_lock();
	if (*sp == limit) {
		rv = -1;
	} else {
		rv = (*sp)++;
	}
	irq_unlock(key);

	return rv;
}

/**
 * @brief Get the memory address of an unused dynamic IRQ or exception stub
 *
 * We generate at build time a set of dynamic stubs which push
 * a stub index onto the stack for use as an argument by
 * common handling code. These don't each have individual labels,
 * but it's possible to compute an offset to any particular one
 *
 * @param stub_idx Stub number to fetch the corresponding stub function
 * @param base_ptr Memory location in ROM where stubs begin
 * @return Pointer to the stub code to install into the IDT
 */
void *_get_dynamic_stub(int stub_idx, void *base_ptr)
{
	uint32_t offset;

	/*
	 * Because we want the sizes of the stubs to be consisent and minimized,
	 * stubs are grouped into blocks, each containing a push and subsequent
	 * 2-byte jump instruction to the end of the block, which then contains
	 * a larger jump instruction to common dynamic IRQ handling code
	 */
	offset = (stub_idx * DYN_STUB_SIZE) + ((stub_idx / DYN_STUB_PER_BLOCK) *
					       DYN_STUB_JMP_SIZE);

	return (void *)((uint32_t)base_ptr + offset);
}


/**
 * @brief Map an IRQ/exception vector back to the corresponding stub index
 *
 * This is used to fetch a reference to a stub when all we have is the IRQ
 * vector.
 *
 * @param IRQ vector as installed in the IDT
 * @return stub index
 */
uint8_t _stub_idx_from_vector(int vector)
{
	IDT_ENTRY *idt_entry;
	uint8_t *stub_addr;

	/*
	 * We need to do a reverse map from the vector number to the stub
	 * index. Look in the IDT for the provided vector and find the memory
	 * address of the handler function, which should be one of
	 * the dynamic stubs
	 */
	idt_entry = (IDT_ENTRY *)(_idt_base_address + (vector << 3));
	stub_addr = (uint8_t *)((uint32_t)idt_entry->lowOffset +
		     ((uint32_t)idt_entry->hiOffset << 16));

	/*
	 * Return the specific byte in the handler code which contains
	 * the stub index, the argument to the initial push operation
	 */
	return stub_addr[DYN_STUB_IDX_OFFSET];
}

#endif /* ALL_DYN_STUBS */

