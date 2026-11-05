/* SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors */
/* SPDX-License-Identifier: Apache-2.0 */

#ifndef ZEPHYR_INCLUDE_ARCH_RISCV_IRQ_VECTOR_H_
#define ZEPHYR_INCLUDE_ARCH_RISCV_IRQ_VECTOR_H_

#include <zephyr/sys/util_macro.h>

#ifdef CONFIG_IRQ_VECTOR_TABLE_JUMP_BY_CODE
#define ARCH_IRQ_VECTOR_ENTRY_SIZE 4
#define ARCH_IRQ_VECTOR_JUMP_CODE(v) \
	".option push\n\t.option norvc\n\t.option norelax\n\tj " STRINGIFY(v) \
	"\n\t.option pop\n\t"

/* The operand lets the compiler update a local target's name during LTO. */
#define ARCH_IRQ_VECTOR_JUMP_OPERAND \
	".option push\n\t.option norvc\n\t.option norelax\n\tj %0\n\t.option pop\n\t"

#define ARCH_IRQ_VECTOR_SECTION_OPEN(sect) \
	".ifndef \".Lz_isr_guard." sect "\"\n\t" \
	".set \".Lz_isr_guard." sect "\", 1\n\t" \
	".weak \"z_isr_group." sect "\"\n\t" \
	".pushsection \"" sect "\",\"axG\",@progbits,\"z_isr_group." sect "\",comdat\n\t"
#define ARCH_IRQ_VECTOR_SECTION_CLOSE \
	".popsection\n\t.endif\n\t"
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_RISCV_IRQ_VECTOR_H_ */
