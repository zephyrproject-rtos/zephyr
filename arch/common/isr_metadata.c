/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <zephyr/isr_table_metadata.h>
#include <zephyr/sw_isr_table.h>

#if !defined(CONFIG_NUM_IRQS) || \
	(defined(CONFIG_GEN_IRQ_VECTOR_TABLE) && defined(CONFIG_IRQ_VECTOR_TABLE_JUMP_BY_CODE))
#include <zephyr/arch/cpu.h>
#endif

#if defined(CONFIG_ISR_TABLES_LOCAL_DECLARATION)
#if defined(Z_ISR_LOCAL_METADATA_NONALLOC)
static __used void z_isr_metadata_capabilities(void)
{
	GEN_ABSOLUTE_SYM(z_isr_metadata_nonallocated, 1);
}
#endif
Z_ISR_LOCAL_METADATA_STORAGE Z_GENERIC_SECTION(.irq_info) __used __noasan
const struct z_isr_local_header _iheader = {
	.version = Z_ISR_LOCAL_METADATA_VERSION,
	.header_size = sizeof(struct z_isr_local_header),
	.table_size = IRQ_TABLE_SIZE,
	.offset = CONFIG_GEN_IRQ_START_VECTOR,
	.entry_size = sizeof(struct _isr_table_entry),
	.entry_alignment = __alignof(struct _isr_table_entry),
	.arg_offset = offsetof(struct _isr_table_entry, arg),
	.arg_size = sizeof(((struct _isr_table_entry *)0)->arg),
	.isr_offset = offsetof(struct _isr_table_entry, isr),
	.isr_size = sizeof(((struct _isr_table_entry *)0)->isr),
#if defined(CONFIG_SHARED_INTERRUPTS)
	.shared_size = sizeof(struct z_shared_isr_table_entry),
	.shared_alignment = __alignof(struct z_shared_isr_table_entry),
	.shared_count_offset = offsetof(struct z_shared_isr_table_entry, client_num),
	.shared_count_size = sizeof(((struct z_shared_isr_table_entry *)0)->client_num),
	.shared_count_alignment = __alignof(size_t),
#endif
#if defined(CONFIG_GEN_IRQ_VECTOR_TABLE)
#if defined(CONFIG_IRQ_VECTOR_TABLE_JUMP_BY_CODE)
	.vector_size = ARCH_IRQ_VECTOR_ENTRY_SIZE,
	.vector_alignment = ARCH_IRQ_VECTOR_ENTRY_SIZE,
#else
	.vector_size = sizeof(uintptr_t),
	.vector_alignment = __alignof(uintptr_t),
#endif
#endif
};
#else
Z_GENERIC_SECTION(.irq_info) __used const struct {
	uint32_t table_size;
	uint32_t offset;
} _iheader = {
	.table_size = IRQ_TABLE_SIZE,
	.offset = CONFIG_GEN_IRQ_START_VECTOR,
};
#endif
