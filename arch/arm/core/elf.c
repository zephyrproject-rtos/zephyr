/*
 * Copyright (c) 2023 Intel Corporation
 * Copyright (c) 2024 Schneider Electric
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/llext/elf.h>
#include <zephyr/llext/llext.h>
#include <zephyr/llext/llext_internal.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(elf, CONFIG_LLEXT_LOG_LEVEL);

#define R_ARM_NONE         0
#define R_ARM_PC24         1
#define R_ARM_ABS32        2
#define R_ARM_REL32        3
#define R_ARM_COPY         20
#define R_ARM_GLOB_DAT     21
#define R_ARM_JUMP_SLOT    22
#define R_ARM_RELATIVE     23
#define R_ARM_CALL         28
#define R_ARM_JUMP24       29
#define R_ARM_TARGET1      38
#define R_ARM_V4BX         40
#define R_ARM_PREL31       42
#define R_ARM_MOVW_ABS_NC  43
#define R_ARM_MOVT_ABS     44
#define R_ARM_MOVW_PREL_NC 45
#define R_ARM_MOVT_PREL    46
#define R_ARM_ALU_PC_G0_NC 57
#define R_ARM_ALU_PC_G1_NC 59
#define R_ARM_LDR_PC_G2    63

#define R_ARM_THM_CALL         10
#define R_ARM_THM_JUMP24       30
#define R_ARM_THM_MOVW_ABS_NC  47
#define R_ARM_THM_MOVT_ABS     48
#define R_ARM_THM_MOVW_PREL_NC 49
#define R_ARM_THM_MOVT_PREL    50

#define OPCODE2ARMMEM(x) ((uint32_t)(x))
#define OPCODE2THM16MEM(x) ((uint16_t)(x))
#define MEM2ARMOPCODE(x) OPCODE2ARMMEM(x)
#define MEM2THM16OPCODE(x) OPCODE2THM16MEM(x)
#define JUMP_UPPER_BOUNDARY ((int32_t)0xfe000000)
#define JUMP_LOWER_BOUNDARY ((int32_t)0x2000000)
#define PREL31_UPPER_BOUNDARY ((int32_t)0x40000000)
#define PREL31_LOWER_BOUNDARY ((int32_t)-0x40000000)
#define THM_JUMP_UPPER_BOUNDARY ((int32_t)0xff000000)
#define THM_JUMP_LOWER_BOUNDARY ((int32_t)0x01000000)
#define MASK_V4BX_RM_COND 0xf000000f
#define MASK_V4BX_NOT_RM_COND 0x01a0f000
#define MASK_BRANCH_COND GENMASK(31, 28)
#define MASK_BRANCH_101 GENMASK(27, 25)
#define MASK_BRANCH_L BIT(24)
#define MASK_BRANCH_OFFSET GENMASK(23, 0)
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
#define SHIFT_PREL31_SIGN 30
#define SHIFT_BRANCH_OFFSET 2
#define SHIFT_JUMPS_SIGN 25
#define SHIFT_MOV_RD 4
#define SHIFT_MOV_RN 4
#define SHIFT_MOVS_SIGN 15
#define SHIFT_THM_JUMPS_SIGN 24
#define SHIFT_THM_BW_IMM10 12
#define SHIFT_THM_BL_J2 22
#define SHIFT_THM_BL_J1 23
#define SHIFT_THM_MOVS_SIGN 15
#define SHIFT_THM_MOV_I 1
#define SHIFT_THM_MOV_IMM3 4
#define SHIFT_THM_MOV_IMM4 12

static inline int prel31_decode(elf_word reloc_type, uint32_t loc,
				uint32_t sym_base_addr, const char *sym_name, int32_t *offset)
{
	int ret;

	*offset = sign_extend(*(int32_t *)loc, SHIFT_PREL31_SIGN);
	*offset += sym_base_addr - loc;
	if (*offset >= PREL31_UPPER_BOUNDARY || *offset < PREL31_LOWER_BOUNDARY) {
		LOG_ERR("sym '%s': relocation out of range (%#x -> %#x)\n",
			sym_name, loc, sym_base_addr);
		ret = -ENOEXEC;
	} else {
		ret = 0;
	}

	return ret;
}

static inline void prel31_reloc(uint32_t loc, int32_t *offset)
{
	*(uint32_t *)loc &= BIT(31);
	*(uint32_t *)loc |= *offset & GENMASK(30, 0);
}

static int prel31_handler(elf_word reloc_type, uint32_t loc,
				uint32_t sym_base_addr, const char *sym_name)
{
	int ret;
	int32_t offset;

	ret = prel31_decode(reloc_type, loc, sym_base_addr, sym_name, &offset);
	if (!ret) {
		prel31_reloc(loc, &offset);
	}

	return ret;
}

static inline int jumps_decode(elf_word reloc_type, uint32_t loc,
				uint32_t sym_base_addr, const char *sym_name, int32_t *offset)
{
	int ret;

	*offset = MEM2ARMOPCODE(*(uint32_t *)loc);
	*offset = (*offset & MASK_BRANCH_OFFSET) << SHIFT_BRANCH_OFFSET;
	*offset = sign_extend(*offset, SHIFT_JUMPS_SIGN);
	*offset += sym_base_addr - loc;
	if (*offset >= JUMP_LOWER_BOUNDARY || *offset <= JUMP_UPPER_BOUNDARY) {
		LOG_ERR("sym '%s': relocation out of range (%#x -> %#x)\n",
			sym_name, loc, sym_base_addr);
		ret = -ENOEXEC;
	} else {
		ret = 0;
	}

	return ret;
}

static inline void jumps_reloc(uint32_t loc, int32_t *offset)
{
	*offset >>= SHIFT_BRANCH_OFFSET;
	*offset &= MASK_BRANCH_OFFSET;

	*(uint32_t *)loc &= OPCODE2ARMMEM(MASK_BRANCH_COND|MASK_BRANCH_101|MASK_BRANCH_L);
	*(uint32_t *)loc |= OPCODE2ARMMEM(*offset);
}

static int jumps_handler(elf_word reloc_type, uint32_t loc,
				uint32_t sym_base_addr, const char *sym_name)
{
	int ret;
	int32_t offset;

	ret = jumps_decode(reloc_type, loc, sym_base_addr, sym_name, &offset);
	if (!ret) {
		jumps_reloc(loc, &offset);
	}

	return ret;
}

static void movs_handler(elf_word reloc_type, uint32_t loc,
			uint32_t sym_base_addr, const char *sym_name)
{
	int32_t offset;
	uint32_t tmp;

	offset = tmp = MEM2ARMOPCODE(*(uint32_t *)loc);
	offset = ((offset & MASK_MOV_RN) >> SHIFT_MOV_RN) | (offset & MASK_MOV_OPERAND2);
	offset = sign_extend(offset, SHIFT_MOVS_SIGN);

	offset += sym_base_addr;
	if (reloc_type == R_ARM_MOVT_PREL || reloc_type == R_ARM_MOVW_PREL_NC) {
		offset -= loc;
	}
	if (reloc_type == R_ARM_MOVT_ABS || reloc_type == R_ARM_MOVT_PREL) {
		offset >>= 16;
	}

	tmp &= (MASK_MOV_COND | MASK_MOV_00 | MASK_MOV_I | MASK_MOV_OPCODE | MASK_MOV_RD);
	tmp |= ((offset & MASK_MOV_RD) << SHIFT_MOV_RD) | (offset & MASK_MOV_OPERAND2);

	*(uint32_t *)loc = OPCODE2ARMMEM(tmp);
}

static inline int thm_jumps_decode(elf_word reloc_type, uint32_t loc,
			uint32_t sym_base_addr, const char *sym_name, int32_t *offset,
			uint32_t *upper, uint32_t *lower)
{
	int ret;
	uint32_t j_one, j_two, sign;

	*upper = MEM2THM16OPCODE(*(uint16_t *)loc);
	*lower = MEM2THM16OPCODE(*(uint16_t *)(loc + 2));

	/* sign is bit10 */
	sign = (*upper >> BIT_THM_BW_S) & 1;
	j_one = (*lower >> BIT_THM_BL_J1) & 1;
	j_two = (*lower >> BIT_THM_BL_J2) & 1;
	*offset = (sign << SHIFT_THM_JUMPS_SIGN) |
				((~(j_one ^ sign) & 1) << SHIFT_THM_BL_J1) |
				((~(j_two ^ sign) & 1) << SHIFT_THM_BL_J2) |
				((*upper & MASK_THM_BW_IMM10) << SHIFT_THM_BW_IMM10) |
				((*lower & MASK_THM_BL_IMM11) << 1);
	*offset = sign_extend(*offset, SHIFT_THM_JUMPS_SIGN);
	*offset += sym_base_addr - loc;

	if (*offset >= THM_JUMP_LOWER_BOUNDARY || *offset <= THM_JUMP_UPPER_BOUNDARY) {
		LOG_ERR("sym '%s': relocation out of range (%#x -> %#x)\n",
			sym_name, loc, sym_base_addr);
		ret = -ENOEXEC;
	} else {
		ret = 0;
	}

	return ret;
}

static inline void thm_jumps_reloc(uint32_t loc, int32_t *offset,
			uint32_t *upper, uint32_t *lower)
{
	uint32_t j_one, j_two, sign;

	sign = (*offset >> SHIFT_THM_JUMPS_SIGN) & 1;
	j_one = sign ^ (~(*offset >> SHIFT_THM_BL_J1) & 1);
	j_two = sign ^ (~(*offset >> SHIFT_THM_BL_J2) & 1);
	*upper = (uint16_t)((*upper & MASK_THM_BW_11110) | (sign << BIT_THM_BW_S) |
				((*offset >> SHIFT_THM_BW_IMM10) & MASK_THM_BW_IMM10));
	*lower = (uint16_t)((*lower & (MASK_THM_BL_10|MASK_THM_BL_1)) |
				(j_one << BIT_THM_BL_J1) | (j_two << BIT_THM_BL_J2) |
				((*offset >> 1) & MASK_THM_BL_IMM11));

	*(uint16_t *)loc = OPCODE2THM16MEM(*upper);
	*(uint16_t *)(loc + 2) = OPCODE2THM16MEM(*lower);
}

static int thm_jumps_handler(elf_word reloc_type, uint32_t loc,
			uint32_t sym_base_addr, const char *sym_name)
{
	int ret;
	int32_t offset;
	uint32_t upper, lower;

	ret = thm_jumps_decode(reloc_type, loc, sym_base_addr, sym_name, &offset, &upper, &lower);
	if (!ret) {
		thm_jumps_reloc(loc, &offset, &upper, &lower);
	}

	return ret;
}

static void thm_movs_handler(elf_word reloc_type, uint32_t loc,
			uint32_t sym_base_addr, const char *sym_name)
{
	int32_t offset;
	uint32_t upper, lower;

	upper = MEM2THM16OPCODE(*(uint16_t *)loc);
	lower = MEM2THM16OPCODE(*(uint16_t *)(loc + 2));

	/* MOVT/MOVW instructions encoding in Thumb-2 */
	offset = ((upper & MASK_THM_MOV_IMM4) << SHIFT_THM_MOV_IMM4) |
		((upper & MASK_THM_MOV_I) << SHIFT_THM_MOV_I) |
		((lower & MASK_THM_MOV_IMM3) >> SHIFT_THM_MOV_IMM3) | (lower & MASK_THM_MOV_IMM8);
	offset = sign_extend(offset, SHIFT_THM_MOVS_SIGN);
	offset += sym_base_addr;

	if (reloc_type == R_ARM_THM_MOVT_PREL || reloc_type == R_ARM_THM_MOVW_PREL_NC) {
		offset -= loc;
	}
	if (reloc_type == R_ARM_THM_MOVT_ABS || reloc_type == R_ARM_THM_MOVT_PREL) {
		offset >>= 16;
	}

	upper = (uint16_t)((upper & (MASK_THM_MOV_11110|MASK_THM_MOV_100100)) |
		((offset & (MASK_THM_MOV_IMM4<<SHIFT_THM_MOV_IMM4)) >> SHIFT_THM_MOV_IMM4) |
		((offset & (MASK_THM_MOV_I<<SHIFT_THM_MOV_I)) >> SHIFT_THM_MOV_I));
	lower = (uint16_t)((lower & (MASK_THM_MOV_0|MASK_THM_MOV_RD)) |
		((offset & (MASK_THM_MOV_IMM3>>SHIFT_THM_MOV_IMM3)) << SHIFT_THM_MOV_IMM3) |
		(offset & MASK_THM_MOV_IMM8));
	*(uint16_t *)loc = OPCODE2THM16MEM(upper);
	*(uint16_t *)(loc + 2) = OPCODE2THM16MEM(lower);
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
 *
 * Handler functions prefixed by '_thm_' means that they are Thumb instructions specific.
 * Do NOT mix them with not 'Thumb instructions' in the below switch/case: they are not
 * intended to work together.
 */
int arch_elf_relocate(struct llext_loader *ldr, struct llext *ext, elf_rela_t *rel,
		      const elf_shdr_t *shdr)
{
	int ret = 0;
	elf_word reloc_type = ELF32_R_TYPE(rel->r_info);
	const uintptr_t load_bias = (uintptr_t)ext->mem[LLEXT_MEM_TEXT];
	const uintptr_t loc = llext_get_reloc_instruction_location(ldr, ext, shdr->sh_info, rel);
	elf_sym_t sym;
	uintptr_t sym_base_addr;
	const char *sym_name;

	ret = llext_read_symbol(ldr, ext, rel, &sym);

	if (ret != 0) {
		LOG_ERR("Could not read symbol from binary!");
		return ret;
	}

	sym_name = llext_symbol_name(ldr, ext, &sym);

	ret = llext_lookup_symbol(ldr, ext, &sym_base_addr, rel, &sym, sym_name, shdr);

	if (ret != 0) {
		LOG_ERR("Could not find symbol %s!", sym_name);
		return ret;
	}

	LOG_DBG("%d %lx %lx %s", reloc_type, loc, sym_base_addr, sym_name);

	switch (reloc_type) {
	case R_ARM_NONE:
		break;

	case R_ARM_ABS32:
	case R_ARM_TARGET1:
		*(uint32_t *)loc += sym_base_addr;
		break;

	case R_ARM_PC24:
	case R_ARM_CALL:
	case R_ARM_JUMP24:
		ret = jumps_handler(reloc_type, loc, sym_base_addr, sym_name);
		break;

	case R_ARM_V4BX:
		/* keep Rm and condition bits */
		*(uint32_t *)loc &= OPCODE2ARMMEM(MASK_V4BX_RM_COND);
		/* remove the rest */
		*(uint32_t *)loc |= OPCODE2ARMMEM(MASK_V4BX_NOT_RM_COND);
		break;

	case R_ARM_PREL31:
		ret = prel31_handler(reloc_type, loc, sym_base_addr, sym_name);
		break;

	case R_ARM_REL32:
		*(uint32_t *)loc += sym_base_addr - loc;
		break;

	case R_ARM_MOVW_ABS_NC:
	case R_ARM_MOVT_ABS:
	case R_ARM_MOVW_PREL_NC:
	case R_ARM_MOVT_PREL:
		movs_handler(reloc_type, loc, sym_base_addr, sym_name);
		break;

	case R_ARM_THM_CALL:
	case R_ARM_THM_JUMP24:
		ret = thm_jumps_handler(reloc_type, loc, sym_base_addr, sym_name);
		break;

	case R_ARM_THM_MOVW_ABS_NC:
	case R_ARM_THM_MOVT_ABS:
	case R_ARM_THM_MOVW_PREL_NC:
	case R_ARM_THM_MOVT_PREL:
		thm_movs_handler(reloc_type, loc, sym_base_addr, sym_name);
		break;

	case R_ARM_RELATIVE:
		*(uint32_t *)loc += load_bias;
		break;

	case R_ARM_GLOB_DAT:
	case R_ARM_JUMP_SLOT:
		*(uint32_t *)loc = sym_base_addr;
		break;

	default:
		LOG_ERR("unknown relocation: %u\n", reloc_type);
		ret = -ENOEXEC;
	}

	return ret;
}
