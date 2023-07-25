/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inttypes.h>

#ifndef ZEPHYR_INCLUDE_ARCH_XTENSA_GDBSTUB_SYS_H_
#define ZEPHYR_INCLUDE_ARCH_XTENSA_GDBSTUB_SYS_H_

#ifdef CONFIG_GDBSTUB

#define XTREG_GRP_MASK			0x0F00
#define XTREG_GRP_GENERAL		0x0000
#define XTREG_GRP_ADDR			0x0100
#define XTREG_GRP_SPECIAL		0x0200
#define XTREG_GRP_USER			0x0300

/*
 * Register description fot GDB stub.
 *
 * Values are based on gdb/gdb/xtensa-config.c in the Xtensa overlay,
 * where registers are defined using XTREG() macro:
 *  XTREG(index,ofs,bsz,sz,al,tnum,flg,cp,ty,gr,name,fet,sto,mas,ct,x,y)
 *
 * Translation:
 * idx         : index
 * regno       : tnum
 *               0x00xx : General Registers (A0 - A15, PC)
 *               0x01xx : Address Registers (AR0 - AR31/AR63)
 *               0x02xx : Special Registers (access via RSR/WSR)
 *               0x03xx : User Registers (access via RUR/WUR)
 * byte_size   : sz
 * gpkt_offset : ofs
 */
struct xtensa_register {
	/* Register value */
	uint32_t	val;

	/* GDB register index (for p/P packets) */
	uint8_t		idx;

	/* Size of register */
	uint8_t		byte_size;

	/* Xtensa register number */
	uint16_t	regno;

	/* Offset of this register in GDB G-packet.
	 * -1 if register is not in G-packet.
	 */
	int16_t		gpkt_offset;

	/* Offset of saved register in stack frame.
	 * 0 if not saved in stack frame.
	 */
	int8_t		stack_offset;

	/* Sequence number */
	uint8_t		seqno;

	/* Set 1 to if register should not be written
	 * to during debugging.
	 */
	uint8_t		is_read_only:1;
};

/* Due to Xtensa SoCs being highly configurable,
 * the register files between SoCs are not identical.
 *
 * This means generic registers can, sometimes, have
 * different offsets from start of register files
 * needed to communicate with GDB.
 *
 * Therefore, it is better to defer to the SoC layer
 * for proper support for GDB.
 */
#include <gdbstub/soc.h>

struct gdb_ctx {
	/* Exception reason */
	unsigned int		exception;

	/* Register descriptions */
	struct xtensa_register	*regs;

	/* Number of registers */
	uint8_t			num_regs;

	/* Sequence number */
	uint8_t			seqno;

	/* Index in register descriptions of A0 register */
	uint8_t			a0_idx;

	/* Index in register descriptions of AR0 register */
	uint8_t			ar_idx;

	/* Index in register descriptions of WINDOWBASE register */
	uint8_t			wb_idx;
};

/**
 * Test if the register is a logical address register (A0 - A15).
 *
 * @retval true  if register is A0 - A15
 * @retval false if register is not A0 - A15
 */
static inline bool gdb_xtensa_is_logical_addr_reg(struct xtensa_register *reg)
{
	if (reg->regno < 16) {
		return true;
	} else {
		return false;
	}
}

/**
 * Test if the register is a address register (AR0 - AR31/AR63).
 *
 * @retval true  if register is AR0 - AR31/AR63
 * @retval false if not
 */
static inline bool gdb_xtensa_is_address_reg(struct xtensa_register *reg)
{
	if ((reg->regno & XTREG_GRP_MASK) == XTREG_GRP_ADDR) {
		return true;
	} else {
		return false;
	}
}

/**
 * Test if the register is a special register that needs to be
 * accessed via RSR/WSR.
 *
 * @retval true  if special register
 * @retval false if not
 */
static inline bool gdb_xtensa_is_special_reg(struct xtensa_register *reg)
{
	if ((reg->regno & XTREG_GRP_MASK) == XTREG_GRP_SPECIAL) {
		return true;
	} else {
		return false;
	}
}

/**
 * Test if the register is a user register that needs to be
 * accessed via RUR/WUR.
 *
 * @retval true  if user register
 * @retval false if not
 */
static inline bool gdb_xtensa_is_user_reg(struct xtensa_register *reg)
{
	if ((reg->regno & XTREG_GRP_MASK) == XTREG_GRP_USER) {
		return true;
	} else {
		return false;
	}
}

#endif /* CONFIG_GDBSTUB */

#endif /* ZEPHYR_INCLUDE_ARCH_XTENSA_GDBSTUB_SYS_H_ */
