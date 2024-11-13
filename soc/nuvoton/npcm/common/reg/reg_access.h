/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NUVOTON_NPCM_REG_ACCESS_H
#define _NUVOTON_NPCM_REG_ACCESS_H

/*
 * NPCM register bit/field access operations
 */
#define IS_BIT_SET(reg, bit)        (((reg >> bit) & (0x1)) != 0)

#define GET_POS_FIELD(pos, size)    pos
#define GET_SIZE_FIELD(pos, size)   size
#define FIELD_POS(field)            GET_POS_##field
#define FIELD_SIZE(field)           GET_SIZE_##field

#define GET_FIELD_SZ(field) \
	_GET_FIELD_SZ_(FIELD_SIZE(field))
#define _GET_FIELD_SZ_(f_ops) f_ops

#define GET_FIELD(reg, field) \
	_GET_FIELD_(reg, FIELD_POS(field), FIELD_SIZE(field))
#define _GET_FIELD_(reg, f_pos, f_size) (((reg)>>(f_pos)) & ((1<<(f_size))-1))

#define SET_FIELD(reg, field, value) \
	_SET_FIELD_(reg, FIELD_POS(field), FIELD_SIZE(field), value)
#define _SET_FIELD_(reg, f_pos, f_size, value) \
	((reg) = ((reg) & (~(((1 << (f_size))-1) << (f_pos)))) \
			| ((value) << (f_pos)))
/* Define one bit mask */
#define BIT0 (0x00000001) /* < Bit 0 mask of an 32 bit integer */
#define BIT1 (0x00000002) /* < Bit 1 mask of an 32 bit integer */
#define BIT2 (0x00000004) /* < Bit 2 mask of an 32 bit integer */
#define BIT3 (0x00000008) /* < Bit 3 mask of an 32 bit integer */
#define BIT4 (0x00000010) /* < Bit 4 mask of an 32 bit integer */
#define BIT5 (0x00000020) /* < Bit 5 mask of an 32 bit integer */
#define BIT6 (0x00000040) /* < Bit 6 mask of an 32 bit integer */
#define BIT7 (0x00000080) /* < Bit 7 mask of an 32 bit integer */
#define BIT8 (0x00000100) /* < Bit 8 mask of an 32 bit integer */
#define BIT9 (0x00000200) /* < Bit 9 mask of an 32 bit integer */
#define BIT10 (0x00000400) /* < Bit 10 mask of an 32 bit integer */
#define BIT11 (0x00000800) /* < Bit 11 mask of an 32 bit integer */
#define BIT12 (0x00001000) /* < Bit 12 mask of an 32 bit integer */
#define BIT13 (0x00002000) /* < Bit 13 mask of an 32 bit integer */
#define BIT14 (0x00004000) /* < Bit 14 mask of an 32 bit integer */
#define BIT15 (0x00008000) /* < Bit 15 mask of an 32 bit integer */
#define BIT16 (0x00010000) /* < Bit 16 mask of an 32 bit integer */
#define BIT17 (0x00020000) /* < Bit 17 mask of an 32 bit integer */
#define BIT18 (0x00040000) /* < Bit 18 mask of an 32 bit integer */
#define BIT19 (0x00080000) /* < Bit 19 mask of an 32 bit integer */
#define BIT20 (0x00100000) /* < Bit 20 mask of an 32 bit integer */
#define BIT21 (0x00200000) /* < Bit 21 mask of an 32 bit integer */
#define BIT22 (0x00400000) /* < Bit 22 mask of an 32 bit integer */
#define BIT23 (0x00800000) /* < Bit 23 mask of an 32 bit integer */
#define BIT24 (0x01000000) /* < Bit 24 mask of an 32 bit integer */
#define BIT25 (0x02000000) /* < Bit 25 mask of an 32 bit integer */
#define BIT26 (0x04000000) /* < Bit 26 mask of an 32 bit integer */
#define BIT27 (0x08000000) /* < Bit 27 mask of an 32 bit integer */
#define BIT28 (0x10000000) /* < Bit 28 mask of an 32 bit integer */
#define BIT29 (0x20000000) /* < Bit 29 mask of an 32 bit integer */
#define BIT30 (0x40000000) /* < Bit 30 mask of an 32 bit integer */
#define BIT31 (0x80000000) /* < Bit 31 mask of an 32 bit integer */

#define M8(addr)  (*((volatile unsigned char  *) (addr)))
#define M16(addr) (*((volatile unsigned short *) (addr)))
#define M32(addr) (*((volatile unsigned long *) (addr)))

/* Register bit operation macros */
#define MaskBit(bit_nb) ((uint32_t)0x1 << (bit_nb))
#define IsRegBitSet(Reg, BitMsk, BitMskVal)                                                        \
	(((Reg) & (BitMsk)) == (BitMskVal)) /* Is register bit set */
#define RegSetBit(Reg, BitMsk)                                                                     \
	{                                                                                          \
		(Reg) |= (BitMsk);                                                                 \
	} /* Set register bit */
#define RegClrBit(Reg, BitMsk)                                                                     \
	{                                                                                          \
		(Reg) &= ~(BitMsk);                                                                \
	} /* Clear register bit */
#define RegRepBit(Reg, BitMsk, BitMskVal)                                                          \
	{                                                                                          \
		(Reg) = ((Reg) & ~(BitMsk)) | (BitMskVal);                                         \
	} /* Replace register bit */

#endif /* _NUVOTON_NPCM_REG_ACCESS_H */
