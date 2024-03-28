/*
 * Copyright (c) 2023 Intel Corporation
 * Copyright (c) 2024 Schneider Electric
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/llext/elf.h>
#include <zephyr/llext/llext.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(elf, CONFIG_LLEXT_LOG_LEVEL);

#define OPCODE2ARMMEM(x) opcode_identity32(x)
#define OPCODE2THM16MEM(x) opcode_identity16(x)
#define MEM2ARMOPCODE(x) OPCODE2ARMMEM(x)
#define MEM2THM16OPCODE(x) OPCODE2THM16MEM(x)
#define JUMP_UPPER_BOUNDARY ((int32_t)0xfe000000)
#define JUMP_LOWER_BOUNDARY ((int32_t)0x2000000)
#define PREL31_UPPER_BOUNDARY ((int32_t)0x40000000)
#define PREL31_LOWER_BOUNDARY ((int32_t)-0x40000000)
#define THM_JUMP_UPPER_BOUNDARY ((int32_t)0xff000000)
#define THM_JUMP_LOWER_BOUNDARY ((int32_t)0x01000000)
#define MASK_BRANCH_COND GENMASK(31, 28)
#define MASK_BRANCH_101 GENMASK(27, 25)
#define MASK_BRANCH_L BIT(24)
#define MASK_BRANCH_OffSET GENMASK(23, 0)
#define MASK_MOV_COND GENMASK(31, 28)
#define MASK_MOV_00 GENMASK(27, 26)
#define MASK_MOV_I BIT(25)
#define MASK_MOV_OPCODE GENMASK(24, 21)
#define MASK_MOV_S BIT(20)
#define MASK_MOV_RN GENMASK(19, 16)
#define MASK_MOV_RD GENMASK(15, 12)
#define MASK_MOV_OPERAND2 GENMASK(11, 0)
#define BIT_THM_BW_S 10
#define MASK_THM_BW_11110 GENMASK(15, 11)
#define MASK_THM_BW_S BIT(10)
#define MASK_THM_BW_IMM10 GENMASK(9, 0)
#define BIT_THM_BL_J1 13
#define BIT_THM_BL_J2 11
#define MASK_THM_BL_10 GENMASK(15, 14)
#define MASK_THM_BL_J1 BIT(13)
#define MASK_THM_BL_1 BIT(12)
#define MASK_THM_BL_J2 BIT(11)
#define MASK_THM_BL_IMM11 GENMASK(10, 0)
#define MASK_THM_MOV_11110 GENMASK(15, 11)
#define MASK_THM_MOV_I BIT(10)
#define MASK_THM_MOV_100100 GENMASK(9, 4)
#define MASK_THM_MOV_IMM4 GENMASK(3, 0)
#define MASK_THM_MOV_0 BIT(15)
#define MASK_THM_MOV_IMM3 GENMASK(14, 12)
#define MASK_THM_MOV_RD GENMASK(11, 8)
#define MASK_THM_MOV_IMM8 GENMASK(7, 0)

static inline uint32_t opcode_identity16(uintptr_t x)
{
	return *(uint16_t *)x;
}

static inline uint32_t opcode_identity32(uintptr_t x)
{
	return *(uint32_t *)x;
}

static int decode_prel31(uint32_t rel_index, elf_word reloc_type, uint32_t loc,
				uint32_t sym_base_addr, const char *symname, int32_t *offset)
{
	int ret;

	*offset = sign_extend(*(int32_t *)loc, 30);
	*offset += sym_base_addr - loc;
	if (*offset >= PREL31_UPPER_BOUNDARY || *offset < PREL31_LOWER_BOUNDARY) {
		LOG_ERR("sym '%s': relocation %u out of range (%#x -> %#x)\n",
			symname, rel_index, loc, sym_base_addr);
		ret = -ENOEXEC;
	} else {
		*(uint32_t *)loc &= BIT(31);
		*(uint32_t *)loc |= *offset & GENMASK(30, 0);

		ret = 0;
	}

	return ret;
}

static int decode_jumps(uint32_t rel_index, elf_word reloc_type, uint32_t loc,
				uint32_t sym_base_addr, const char *symname, int32_t *offset)
{
	int ret;

	*offset = MEM2ARMOPCODE(*(uint32_t *)loc);
	*offset = (*offset & MASK_BRANCH_OffSET) << 2;
	*offset = sign_extend(*offset, 25);
	*offset += sym_base_addr - loc;
	if (*offset <= JUMP_UPPER_BOUNDARY || *offset >= JUMP_LOWER_BOUNDARY) {
		LOG_ERR("sym '%s': relocation %u out of range (%#x -> %#x)\n",
			symname, rel_index, loc, sym_base_addr);
		ret = -ENOEXEC;
	} else {
		*offset >>= 2;
		*offset &= MASK_BRANCH_OffSET;

		*(uint32_t *)loc &= OPCODE2ARMMEM(MASK_BRANCH_COND|MASK_BRANCH_101|MASK_BRANCH_L);
		*(uint32_t *)loc |= OPCODE2ARMMEM(*offset);

		ret = 0;
	}

	return ret;
}

static void decode_movs(uint32_t rel_index, elf_word reloc_type, uint32_t loc,
			uint32_t sym_base_addr, const char *symname, int32_t *offset)
{
	uint32_t tmp;

	*offset = tmp = MEM2ARMOPCODE(*(uint32_t *)loc);
	*offset = ((*offset & MASK_MOV_RN) >> 4) | (*offset & MASK_MOV_OPERAND2);
	*offset = sign_extend(*offset, 15);

	*offset += sym_base_addr;
	if (reloc_type == R_ARM_MOVT_PREL || reloc_type == R_ARM_MOVW_PREL_NC) {
		*offset -= loc;
	}
	if (reloc_type == R_ARM_MOVT_ABS || reloc_type == R_ARM_MOVT_PREL) {
		*offset >>= 16;
	}

	tmp &= (MASK_MOV_COND | MASK_MOV_00 | MASK_MOV_I | MASK_MOV_OPCODE | MASK_MOV_RD);
	tmp |= ((*offset & MASK_MOV_RD) << 4) |
	(*offset & (MASK_MOV_RD|MASK_MOV_OPERAND2));

	*(uint32_t *)loc = OPCODE2ARMMEM(tmp);
}

static int decode_thm_jumps(uint32_t rel_index, elf_word reloc_type, uint32_t loc,
			uint32_t sym_base_addr, const char *symname, int32_t *offset)
{
	int ret;
	uint32_t j1, j2, upper, lower, sign;

	/**
	 * For function symbols, only Thumb addresses are
	 * allowed (no interworking).
	 *
	 * For non-function symbols, the destination
	 * has no specific ARM/Thumb disposition, so
	 * the branch is resolved under the assumption
	 * that interworking is not required.
	 */
	upper = MEM2THM16OPCODE(*(uint16_t *)loc);
	lower = MEM2THM16OPCODE(*(uint16_t *)(loc + 2));

	/* sign is bit10 */
	sign = (upper >> BIT_THM_BW_S) & 1;
	j1 = (lower >> BIT_THM_BL_J1) & 1;
	j2 = (lower >> BIT_THM_BL_J2) & 1;
	*offset = (sign << 24) | ((~(j1 ^ sign) & 1) << 23) |
		((~(j2 ^ sign) & 1) << 22) |
		((upper & MASK_THM_BW_IMM10) << 12) |
		((lower & MASK_THM_BL_IMM11) << 1);
	*offset = sign_extend(*offset, 24);
	*offset += sym_base_addr - loc;

	if (*offset <= THM_JUMP_UPPER_BOUNDARY || *offset >= THM_JUMP_LOWER_BOUNDARY) {
		LOG_ERR("sym '%s': relocation %u out of range (%#x -> %#x)\n",
			symname, rel_index, loc, sym_base_addr);
		ret = -ENOEXEC;
	} else {
		sign = (*offset >> 24) & 1;
		j1 = sign ^ (~(*offset >> 23) & 1);
		j2 = sign ^ (~(*offset >> 22) & 1);
		upper = (uint16_t)((upper & MASK_THM_BW_11110) | (sign << BIT_THM_BW_S) |
					((*offset >> 12) & MASK_THM_BW_IMM10));
		lower = (uint16_t)((lower & (MASK_THM_BL_10|MASK_THM_BL_1)) |
				(j1 << BIT_THM_BL_J1) | (j2 << BIT_THM_BL_J2) |
				((*offset >> 1) & MASK_THM_BL_IMM11));

		*(uint16_t *)loc = OPCODE2THM16MEM(upper);
		*(uint16_t *)(loc + 2) = OPCODE2THM16MEM(lower);
		ret = 0;
	}

	return ret;
}

static void decode_thm_movs(uint32_t rel_index, elf_word reloc_type, uint32_t loc,
			uint32_t sym_base_addr, const char *symname, int32_t *offset)
{
	uint32_t upper, lower;

	upper = MEM2THM16OPCODE(*(uint16_t *)loc);
	lower = MEM2THM16OPCODE(*(uint16_t *)(loc + 2));

	/**
	 * MOVT/MOVW instructions encoding in Thumb-2
	 */
	*offset = ((upper & MASK_THM_MOV_IMM4) << 12) |
		((upper & MASK_THM_MOV_I) << 1) |
		((lower & MASK_THM_MOV_IMM3) >> 4) | (lower & MASK_THM_MOV_IMM8);
	*offset = sign_extend(*offset, 15);
	*offset += sym_base_addr;

	if (reloc_type == R_ARM_THM_MOVT_PREL || reloc_type == R_ARM_THM_MOVW_PREL_NC) {
		*offset -= loc;
	}
	if (reloc_type == R_ARM_THM_MOVT_ABS || reloc_type == R_ARM_THM_MOVT_PREL) {
		*offset >>= 16;
	}

	upper = (uint16_t)((upper & (MASK_THM_MOV_11110|MASK_THM_MOV_100100)) |
			((*offset & (MASK_THM_MOV_IMM4<<12)) >> 12) |
			((*offset & (MASK_THM_MOV_I<<1)) >> 1));
	lower = (uint16_t)((lower & (MASK_THM_MOV_0|MASK_THM_MOV_RD)) |
			((*offset & MASK_THM_MOV_IMM3) << 4) |
			(*offset & MASK_THM_MOV_IMM8));
	*(uint16_t *)loc = OPCODE2THM16MEM(upper);
	*(uint16_t *)(loc + 2) = OPCODE2THM16MEM(lower);
}

/**
 * loc = dstsec->sh_addr + rel->r_offset;
 *
 * loc : addr of opcode
 * sym_base_addr : addr of symbol if undef else addr of symbol section + sym value
 * symname: symbol name
 */
static int apply_relocate(uint32_t rel_index, elf_word reloc_type, uint32_t loc,
			 uint32_t sym_base_addr, const char *symname,
			 uintptr_t load_bias)
{
	int ret = 0;
	int32_t offset;

	LOG_DBG("%s:%u %d %x %x %s", __func__, rel_index, reloc_type, loc, sym_base_addr, symname);

	switch (reloc_type) {
	case R_ARM_NONE:
		/* no-op */
		break;

	case R_ARM_ABS32:
		/* fall-through */
	case R_ARM_TARGET1:
		*(uint32_t *)loc += sym_base_addr;
		break;

	case R_ARM_PC24:
		/* fall-through */
	case R_ARM_CALL:
		/* fall-through */
	case R_ARM_JUMP24:
		ret = decode_jumps(rel_index, reloc_type, loc, sym_base_addr, symname, &offset);
		break;

	case R_ARM_V4BX:
#ifdef CONFIG_LLEXT_ARM_V4BX
		/**
		 * Preserve Rm and the condition code. Alter
		 * other bits to re-code instruction as
		 * MOV PC,Rm.
		 */
		*(uint32_t *)loc &= OPCODE2ARMMEM(0xf000000f);
		*(uint32_t *)loc |= OPCODE2ARMMEM(0x01a0f000);
#endif
		break;

	case R_ARM_PREL31:
		ret = decode_prel31(rel_index, reloc_type, loc, sym_base_addr,
							symname, &offset);
		break;

	case R_ARM_REL32:
		*(uint32_t *)loc += sym_base_addr - loc;
		break;

	case R_ARM_MOVW_ABS_NC:
		/* fall-through */
	case R_ARM_MOVT_ABS:
		/* fall-through */
	case R_ARM_MOVW_PREL_NC:
		/* fall-through */
	case R_ARM_MOVT_PREL:
		decode_movs(rel_index, reloc_type, loc, sym_base_addr, symname, &offset);
		break;

	case R_ARM_THM_CALL:
		/* fall-through */
	case R_ARM_THM_JUMP24:
		ret = decode_thm_jumps(rel_index, reloc_type, loc, sym_base_addr, symname, &offset);
		break;

	case R_ARM_THM_MOVW_ABS_NC:
		/* fall-through */
	case R_ARM_THM_MOVT_ABS:
		/* fall-through */
	case R_ARM_THM_MOVW_PREL_NC:
		/* fall-through */
	case R_ARM_THM_MOVT_PREL:
		decode_thm_movs(rel_index, reloc_type, loc, sym_base_addr, symname, &offset);
		break;

	case R_ARM_RELATIVE:
		*(uint32_t *)loc += load_bias;
		break;

	case R_ARM_GLOB_DAT:
		*(uint32_t *)loc = sym_base_addr;
		break;

	case R_ARM_JUMP_SLOT:
		*(uint32_t *)loc = sym_base_addr;
		break;

	default:
		LOG_ERR("unknown relocation: %u\n", reloc_type);
		ret = -ENOEXEC;
	}

	return ret;
}

/**
 * @brief Architecture specific function for relocating partially linked (static) elf
 *
 * Elf files contain a series of relocations described in a section. These relocation
 * instructions are architecture specific and each architecture supporting extensions
 * must implement this.
 *
 * The relocation codes for arm are well documented
 * https://github.com/ARM-software/abi-aa/blob/main/aaelf32/aaelf32.rst#relocation
 */
int32_t arch_elf_relocate(elf_rela_t *rel, uint32_t rel_index, uintptr_t loc,
		 uintptr_t sym_base_addr, const char *symname, uintptr_t load_bias)
{
	elf_word reloc_type = ELF32_R_TYPE(rel->r_info);

	return apply_relocate(rel_index, reloc_type, (uint32_t)loc,
			 (uint32_t)sym_base_addr, symname, load_bias);
}
