/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NUVOTON_NPCX_REG_ACCESS_H
#define _NUVOTON_NPCX_REG_ACCESS_H

/*
 * NPCX register bit/field access operations
 */
#define IS_BIT_SET(reg, bit)        ((reg >> bit) & (0x1))

#define GET_POS_FIELD(pos, size)    pos
#define GET_SIZE_FIELD(pos, size)   size
#define FIELD_POS(field)            GET_POS_##field
#define FIELD_SIZE(field)           GET_SIZE_##field

#define GET_FIELD(reg, field) \
	_GET_FIELD_(reg, FIELD_POS(field), FIELD_SIZE(field))
#define _GET_FIELD_(reg, f_pos, f_size) (((reg)>>(f_pos)) & ((1<<(f_size))-1))

#define SET_FIELD(reg, field, value) \
	_SET_FIELD_(reg, FIELD_POS(field), FIELD_SIZE(field), value)
#define _SET_FIELD_(reg, f_pos, f_size, value) \
	((reg) = ((reg) & (~(((1 << (f_size))-1) << (f_pos)))) \
			| ((value) << (f_pos)))

#endif /* _NUVOTON_NPCX_REG_ACCESS_H */
