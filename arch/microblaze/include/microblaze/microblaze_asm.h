/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. (AMD)
 * Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#ifndef ZEPHYR_ARCH_MICROBLAZE_INCLUDE_MICROBLAZE_MICROBLAZE_ASM_H_
#define ZEPHYR_ARCH_MICROBLAZE_INCLUDE_MICROBLAZE_MICROBLAZE_ASM_H_

#define KERNEL_REF_REG			     r11
#define CURRENT_THREAD_REG		     r12
#define NEXT_THREAD_REG			     r4
#define TEMP_DATA_REG			     r10
#define ADD_IMM(rx, imm)		     addik rx, rx, imm
#define SUB_IMM(rx, imm)		     addik rx, rx, -(imm)
#define SET_BITS(rd, mask)		     ori rd, rd, mask
#define CLEAR_BITS(rd, mask)		     andi rd, rd, ~(mask)
#define MASK_BITS(rd, mask)		     andi rd, rd, mask
#define COPY_REG(rd, rx)		     ori rd, rx, 0
#define SET_REG(rd, imm)		     ori rd, r0, imm
#define STORE(rx, rd, imm)		     swi rx, rd, imm
#define LOAD(rx, rd, imm)		     lwi rx, rd, imm
#define STORE_REG_TO_ADDR(rx, imm)	     STORE(rx, r0, imm)
#define LOAD_REG_FROM_ADDR(rx, imm)	     LOAD(rx, r0, imm)
#define STORE_TO_STACK(rx, imm)		     STORE(rx, r1, imm)
#define PUSH_CONTEXT_TO_STACK(rx)	     STORE_TO_STACK(rx, ESF_OFFSET(rx))
#define POP_CONTEXT_FROM_STACK(rx)	     LOAD_FROM_STACK(rx, ESF_OFFSET(rx))
#define STACK_ALLOC(imm)		     SUB_IMM(r1, imm)
#define STACK_FREE(imm)			     ADD_IMM(r1, imm)
#define LOAD_FROM_STACK(rx, imm)	     LOAD(rx, r1, imm)
#define LOAD_FROM_KERNEL(rd, offset)	     LOAD(rd, KERNEL_REF_REG, offset)
#define SWITCH_TO_IRQ_STACK(rx)		     LOAD_FROM_KERNEL(rx, _kernel_offset_to_irq_stack)
#define LOAD_CURRENT_THREAD(rx)		     LOAD_FROM_KERNEL(rx, _kernel_offset_to_current)
#define LOAD_NEXT_THREAD(rx)		     LOAD_FROM_KERNEL(rx, _kernel_offset_to_ready_q_cache)
#define WRITE_TO_KERNEL_CURRENT(rx)	     STORE(rx, KERNEL_REF_REG, _kernel_offset_to_current)
#define STORE_TO_CURRENT_THREAD(rx, offset)  STORE(rx, CURRENT_THREAD_REG, offset)
#define LOAD_FROM_CURRENT_THREAD(rx, offset) LOAD(rx, CURRENT_THREAD_REG, offset)
#define LOAD_FROM_NEXT_THREAD(rx, offset)    LOAD(rx, NEXT_THREAD_REG, offset)
#define DELAY_SLOT(instr, ...)		     instr __VA_ARGS__
#define JUMP(target, dslot)                                                                        \
	brid target;                                                                               \
	dslot;
#define CALL(target, dslot)                                                                        \
	brlid r15, target;                                                                         \
	dslot;
#define JUMP_IF_ZERO(rx, target, dslot)                                                            \
	beqid rx, target;                                                                          \
	dslot;
#define JUMP_IF_NONZERO(rx, target, dslot)                                                         \
	bneid rx, target;                                                                          \
	dslot;
/* "assert" macro is written for checking stack overflows; not advised to use for other purposes */
#define ASSERT_GT_ZERO(rx, target)                                                                 \
	bgti rx, 4 * (5 + 1);                                                                      \
	mfs r17, rmsr;                                                                             \
	ori r17, r17, MSR_EIP_MASK;                                                                \
	mts rmsr, r17;                                                                             \
	bralid r17, target;                                                                        \
	nop;

#endif /* ZEPHYR_ARCH_MICROBLAZE_INCLUDE_MICROBLAZE_MICROBLAZE_ASM_H_ */
