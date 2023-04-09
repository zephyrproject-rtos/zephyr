/*
 * Copyright (c) 2023 Ionut Pavel <iocapa@iocapa.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MISC_PIO_RPI_PICO_UTIL_H_
#define ZEPHYR_DRIVERS_MISC_PIO_RPI_PICO_UTIL_H_

#include <stdint.h>
#include <zephyr/sys/util.h>

/* PIO registers */
#include <hardware/structs/pio.h>

/** Indexed interrupt access macros TODO: should be indexed in HW structs */

struct pio_irq_hw {
	io_rw_32 inte;
	io_rw_32 intf;
	io_ro_32 ints;
};

#define PIO_IRQ_HW_INDEX(hw, index)								\
	(&(((struct pio_irq_hw *)&((hw)->inte0))[(index)]))

/** Shared registers atomic bit ops */

#define PIO_ATOMIC_SET(reg, mask)								\
	(*((io_wo_32 *)(((io_wo_8 *)(&(reg))) + REG_ALIAS_SET_BITS)) = (mask))

#define PIO_ATOMIC_CLR(reg, mask)								\
	(*((io_wo_32 *)(((io_wo_8 *)(&(reg))) + REG_ALIAS_CLR_BITS)) = (mask))

#define PIO_ATOMIC_XOR(reg, mask)								\
	(*((io_wo_32 *)(((io_wo_8 *)(&(reg))) + REG_ALIAS_XOR_BITS)) = (mask))

/** Instruction macros */

#define PIO_ISTR_IS_JMP(instr)									\
	(((instr) >> 13) == 0x00u)

/** Prescaler macros */

#define PIO_SM_CLKDIV(fin, fout)								\
	(uint32_t)(((((uint64_t)(fin)) * 65536u) / (fout)) & 0xFFFFFF00u)

/** Index macros */

#define PIO_ASM_INDEX(rel, idx)									\
	((!!(rel) ? BIT(4) : 0) | ((idx) & BIT_MASK(4)))

/** Bit count macros */

#define PIO_ASM_BITCOUNT(count)									\
	((count) & BIT_MASK(5))

/** Address macros */

#define PIO_ASM_ADDR(base, offs)								\
	(((base) + (offs)) & BIT_MASK(5))

/** Side set macros */

#define PIO_ASM_SIDE(opt, opt_val, ss_cnt, ss_val, delay)					\
	(((!!(opt) ? (										\
		((opt_val) << ((ss_cnt) - 1)) | ((ss_val) & BIT_MASK((ss_cnt) - 1))		\
	) : (											\
		(ss_val) & BIT_MASK((ss_cnt))							\
	)) << (5 - (ss_cnt))) | ((delay) & BIT_MASK(5 - (ss_cnt))))

/** JMP Instruction macros */

#define PIO_ASM_JMP_COND_ALWAYS		0u
#define PIO_ASM_JMP_COND_NOTX		1u
#define PIO_ASM_JMP_COND_DECX		2u
#define PIO_ASM_JMP_COND_NOTY		3u
#define PIO_ASM_JMP_COND_DECY		4u
#define PIO_ASM_JMP_COND_XNOTEQY	5u
#define PIO_ASM_JMP_COND_PIN		6u
#define PIO_ASM_JMP_COND_NOTOSRE	7u

#define PIO_ASM_JMP(cond, addr, dss)								\
	(uint16_t)((0x0u << 13)									\
	| (((dss)	& BIT_MASK(5))	<< 8)							\
	| (((cond)	& BIT_MASK(3))	<< 5)							\
	| (((addr)	& BIT_MASK(5))	<< 0))

/** WAIT Instruction macros */

#define PIO_ASM_WAIT_SRC_GPIO		0u
#define PIO_ASM_WAIT_SRC_PIN		1u
#define PIO_ASM_WAIT_SRC_IRQ		2u

#define PIO_ASM_WAIT(pol, src, ind, dss)							\
	(uint16_t)((0x1u << 13)									\
	| (((dss)	& BIT_MASK(5))	<< 8)							\
	| (((pol)	& BIT_MASK(1))	<< 7)							\
	| (((src)	& BIT_MASK(2))	<< 5)							\
	| (((ind)	& BIT_MASK(5))	<< 0))

/** IN Instruction macros */

#define PIO_ASM_IN_SRC_PINS		0u
#define PIO_ASM_IN_SRC_X		1u
#define PIO_ASM_IN_SRC_Y		2u
#define PIO_ASM_IN_SRC_NULL		3u
#define PIO_ASM_IN_SRC_ISR		6u
#define PIO_ASM_IN_SRC_OSR		7u

#define PIO_ASM_IN(src, bcnt, dss)								\
	(uint16_t)((0x2u << 13)									\
	| (((dss)	& BIT_MASK(5))	<< 8)							\
	| (((src)	& BIT_MASK(3))	<< 5)							\
	| (((bcnt)	& BIT_MASK(5))	<< 0))

/** OUT Instruction macros */

#define PIO_ASM_OUT_DST_PINS		0u
#define PIO_ASM_OUT_DST_X		1u
#define PIO_ASM_OUT_DST_Y		2u
#define PIO_ASM_OUT_DST_NULL		3u
#define PIO_ASM_OUT_DST_PINDIRS		4u
#define PIO_ASM_OUT_DST_PC		5u
#define PIO_ASM_OUT_DST_ISR		6u
#define PIO_ASM_OUT_DST_EXEC		7u

#define PIO_ASM_OUT(dst, bcnt, dss)								\
	(uint16_t)((0x3u << 13)									\
	| (((dss)	& BIT_MASK(5))	<< 8)							\
	| (((dst)	& BIT_MASK(3))	<< 5)							\
	| (((bcnt)	& BIT_MASK(5))	<< 0))

/** PUSH Instruction macros */

#define PIO_ASM_PUSH(iff, blk, dss)								\
	(uint16_t)((0x4u << 13)									\
	| (((dss)	& BIT_MASK(5))	<< 8)							\
	| (((0u)	& BIT_MASK(1))	<< 7)							\
	| (((iff)	& BIT_MASK(1))	<< 6)							\
	| (((blk)	& BIT_MASK(1))	<< 5)							\
	| (((0u)	& BIT_MASK(5))	<< 0))

/** PULL Instruction macros */

#define PIO_ASM_PULL(ife, blk, dss)								\
	(uint16_t)((0x4u << 13)									\
	| (((dss)	& BIT_MASK(5))	<< 8)							\
	| (((1u)	& BIT_MASK(1))	<< 7)							\
	| (((ife)	& BIT_MASK(1))	<< 6)							\
	| (((blk)	& BIT_MASK(1))	<< 5)							\
	| (((0u)	& BIT_MASK(5))	<< 0))

/** MOV Instruction macros */

#define PIO_ASM_MOV_DST_PINS		0u
#define PIO_ASM_MOV_DST_X		1u
#define PIO_ASM_MOV_DST_Y		2u
#define PIO_ASM_MOV_DST_EXEC		4u
#define PIO_ASM_MOV_DST_PC		5u
#define PIO_ASM_MOV_DST_ISR		6u
#define PIO_ASM_MOV_DST_OSR		7u

#define PIO_ASM_MOV_OP_NONE		0u
#define PIO_ASM_MOV_OP_INVERT		1u
#define PIO_ASM_MOV_OP_BITREVERSE	2u

#define PIO_ASM_MOV_SRC_PINS		0u
#define PIO_ASM_MOV_SRC_X		1u
#define PIO_ASM_MOV_SRC_Y		2u
#define PIO_ASM_MOV_SRC_NULL		3u
#define PIO_ASM_MOV_SRC_STATUS		5u
#define PIO_ASM_MOV_SRC_ISR		6u
#define PIO_ASM_MOV_SRC_OSR		7u

#define PIO_ASM_MOV(dst, op, src, dss)								\
	(uint16_t)((0x5u << 13)									\
	| (((dss)	& BIT_MASK(5))	<< 8)							\
	| (((dst)	& BIT_MASK(3))	<< 5)							\
	| (((op)	& BIT_MASK(2))	<< 3)							\
	| (((src)	& BIT_MASK(3))	<< 0))

/** IRQ Instruction macros */

#define PIO_ASM_IRQ(clr, wait, ind, dss)							\
	(uint16_t)((0x6u << 13)									\
	| (((dss)	& BIT_MASK(5))	<< 8)							\
	| (((0u)	& BIT_MASK(1))	<< 7)							\
	| (((clr)	& BIT_MASK(1))	<< 6)							\
	| (((wait)	& BIT_MASK(1))	<< 5)							\
	| (((ind)	& BIT_MASK(5))	<< 0))

/** SET Instruction macros */

#define PIO_ASM_SET_DST_PINS		0u
#define PIO_ASM_SET_DST_X		1u
#define PIO_ASM_SET_DST_Y		2u
#define PIO_ASM_SET_DST_PINDIRS		4u

#define PIO_ASM_SET(dst, data, dss)								\
	(uint16_t)((0x7u << 13)									\
	| (((dss)	& BIT_MASK(5))	<< 8)							\
	| (((dst)	& BIT_MASK(3))	<< 5)							\
	| (((data)	& BIT_MASK(5))	<< 0))

/**
 * @brief Load and patch program
 *
 * @param imem Pointer to the instruction memory
 * @param base Base address
 * @param src Pointer to the source data
 * @param size Source data size
 *
 * @note User should assure no overflow will happen
 */
static inline void pio_rpi_pico_util_load_prg(io_wo_32 *imem, uint8_t base,
					      const uint16_t *src, size_t size)
{
	io_wo_32 *dst = &imem[base];
	uint16_t instr;
	size_t i;

	for (i = 0u; i < size; i++) {
		instr = src[i];
		if (PIO_ISTR_IS_JMP(instr)) {
			instr += base;
		}
		dst[i] = instr;
	}
}

#endif /* ZEPHYR_DRIVERS_MISC_PIO_RPI_PICO_UTIL_H_ */
