/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NUVOTON_NPCX_REG_ACCESS_H
#define _NUVOTON_NPCX_REG_ACCESS_H

/*
 * NPCX register structure size/offset checking macro function to mitigate
 * the risk of unexpected compiling results. All addresses of NPCX registers
 * must meet the alignment requirement of cortex-m4.
 * DO NOT use 'packed' attribute if module contains different length ie.
 * 8/16/32 bits registers.
 */
#define NPCX_REG_SIZE_CHECK(reg_def, size)                                                         \
	BUILD_ASSERT(sizeof(struct reg_def) == size, "Failed in size check of register "           \
						     "structure!")
#define NPCX_REG_OFFSET_CHECK(reg_def, member, offset)                                             \
	BUILD_ASSERT(offsetof(struct reg_def, member) == offset,                                   \
		     "Failed in offset check of register structure member!")

/*
 * NPCX register access checking via structure macro function to mitigate the
 * risk of unexpected compiling results if module contains different length
 * registers. For example, a word register access might break into two byte
 * register accesses by adding 'packed' attribute.
 *
 * For example, add this macro for word register 'PRSC' of PWM module in its
 * device init function for checking violation. Once it occurred, core will be
 * stalled forever and easy to find out what happens.
 */
#define NPCX_REG_WORD_ACCESS_CHECK(reg, val)                                                       \
	{                                                                                          \
		uint16_t placeholder = reg;                                                        \
		reg = val;                                                                         \
		__ASSERT(reg == val, "16-bit reg access failed!");                                 \
		reg = placeholder;                                                                 \
	}
#define NPCX_REG_DWORD_ACCESS_CHECK(reg, val)                                                      \
	{                                                                                          \
		uint32_t placeholder = reg;                                                        \
		reg = val;                                                                         \
		__ASSERT(reg == val, "32-bit reg access failed!");                                 \
		reg = placeholder;                                                                 \
	}

/*
 * NPCX register bit/field access operations
 */

#define GET_POS_FIELD(pos, size)  pos
#define GET_SIZE_FIELD(pos, size) size
#define FIELD_POS(field)          GET_POS_##field
#define FIELD_SIZE(field)         GET_SIZE_##field

#define GET_FIELD(reg, field)           _GET_FIELD_(reg, FIELD_POS(field), FIELD_SIZE(field))
#define _GET_FIELD_(reg, f_pos, f_size) (((reg) >> (f_pos)) & ((1 << (f_size)) - 1))

#define SET_FIELD(reg, field, value) _SET_FIELD_(reg, FIELD_POS(field), FIELD_SIZE(field), value)
#define _SET_FIELD_(reg, f_pos, f_size, value)                                                     \
	((reg) = ((reg) & (~(((1 << (f_size)) - 1) << (f_pos)))) | ((value) << (f_pos)))

#define GET_FIELD_POS(field)   _GET_FIELD_POS_(FIELD_POS(field))
#define _GET_FIELD_POS_(f_ops) f_ops

#define GET_FIELD_SZ(field)   _GET_FIELD_SZ_(FIELD_SIZE(field))
#define _GET_FIELD_SZ_(f_ops) f_ops

#endif /* _NUVOTON_NPCX_REG_ACCESS_H */
