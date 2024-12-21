/*
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/llext/elf.h>
#include <zephyr/llext/llext.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(elf, CONFIG_LLEXT_LOG_LEVEL);

#define R_ARM_NONE     0
#define R_AARCH64_NONE 256

/* Static data relocations */
#define R_AARCH64_ABS64  257
#define R_AARCH64_ABS32  258
#define R_AARCH64_ABS16  259
#define R_AARCH64_PREL64 260
#define R_AARCH64_PREL32 261
#define R_AARCH64_PREL16 262

/* Static relocations */
#define R_AARCH64_MOVW_UABS_G0    263
#define R_AARCH64_MOVW_UABS_G0_NC 264
#define R_AARCH64_MOVW_UABS_G1    265
#define R_AARCH64_MOVW_UABS_G1_NC 266
#define R_AARCH64_MOVW_UABS_G2    267
#define R_AARCH64_MOVW_UABS_G2_NC 268
#define R_AARCH64_MOVW_UABS_G3    269
#define R_AARCH64_MOVW_SABS_G0    270
#define R_AARCH64_MOVW_SABS_G1    271
#define R_AARCH64_MOVW_SABS_G2    272
#define R_AARCH64_MOVW_PREL_G0    287
#define R_AARCH64_MOVW_PREL_G0_NC 288
#define R_AARCH64_MOVW_PREL_G1    289
#define R_AARCH64_MOVW_PREL_G1_NC 290
#define R_AARCH64_MOVW_PREL_G2    291
#define R_AARCH64_MOVW_PREL_G2_NC 292
#define R_AARCH64_MOVW_PREL_G3    293

#define R_AARCH64_LD_PREL_LO19        273
#define R_AARCH64_ADR_PREL_LO21       274
#define R_AARCH64_ADR_PREL_PG_HI21    275
#define R_AARCH64_ADR_PREL_PG_HI21_NC 276
#define R_AARCH64_ADD_ABS_LO12_NC     277
#define R_AARCH64_LDST8_ABS_LO12_NC   278
#define R_AARCH64_TSTBR14             279
#define R_AARCH64_CONDBR19            280
#define R_AARCH64_JUMP26              282
#define R_AARCH64_CALL26              283
#define R_AARCH64_LDST16_ABS_LO12_NC  284
#define R_AARCH64_LDST32_ABS_LO12_NC  285
#define R_AARCH64_LDST64_ABS_LO12_NC  286
#define R_AARCH64_LDST128_ABS_LO12_NC 299

/* Masks for immediate values */
#define AARCH64_MASK_IMM12 BIT_MASK(12)
#define AARCH64_MASK_IMM14 BIT_MASK(14)
#define AARCH64_MASK_IMM16 BIT_MASK(16)
#define AARCH64_MASK_IMM19 BIT_MASK(19)
#define AARCH64_MASK_IMM26 BIT_MASK(26)

/* MOV instruction helper symbols */
#define AARCH64_MASK_MOV_OPCODE  BIT_MASK(8)
#define AARCH64_SHIFT_MOV_OPCODE (23)
#define AARCH64_SHIFT_MOV_IMM16  (5)
#define AARCH64_OPCODE_MOVN      (0b00100101)
#define AARCH64_OPCODE_MOVZ      (0b10100101)

/* ADR instruction helper symbols */
#define AARCH64_MASK_ADR_IMMLO  BIT_MASK(2)
#define AARCH64_MASK_ADR_IMMHI  BIT_MASK(19)
#define AARCH64_SHIFT_ADR_IMMLO (29)
#define AARCH64_SHIFT_ADR_IMMHI (5)
#define AARCH64_ADR_IMMLO_BITS  (2)

#define AARCH64_PAGE(expr) ((expr) & ~0xFFF)

enum aarch64_reloc_type {
	AARCH64_RELOC_TYPE_NONE,
	AARCH64_RELOC_TYPE_ABS,
	AARCH64_RELOC_TYPE_PREL,
	AARCH64_RELOC_TYPE_PAGE,
};

/**
 * @brief Function computing a relocation (X in AArch64 ELF).
 *
 * @param[in] reloc_type Type of relocation operation.
 * @param[in] loc Address of an opcode to rewrite (P in AArch64 ELF).
 * @param[in] sym_base_addr Address of the symbol referenced by relocation (S in AArch64 ELF).
 * @param[in] addend Addend from RELA relocation.
 *
 * @return Result of the relocation operation (X in AArch64 ELF)
 */
static uint64_t reloc(enum aarch64_reloc_type reloc_type, uintptr_t loc, uintptr_t sym_base_addr,
		      int64_t addend)
{
	switch (reloc_type) {
	case AARCH64_RELOC_TYPE_ABS:
		return sym_base_addr + addend;
	case AARCH64_RELOC_TYPE_PREL:
		return sym_base_addr + addend - loc;
	case AARCH64_RELOC_TYPE_PAGE:
		return AARCH64_PAGE(sym_base_addr + addend) - AARCH64_PAGE(loc);
	case AARCH64_RELOC_TYPE_NONE:
		return 0;
	}

	CODE_UNREACHABLE;
}

/**
 * @brief Handler for static data relocations.
 *
 * @param[in] rel Relocation data provided by ELF
 * @param[in] reloc_type Type of relocation operation.
 * @param[in] loc Address of an opcode to rewrite (P in AArch64 ELF).
 * @param[in] sym_base_addr Address of the symbol referenced by relocation (S in AArch64 ELF).
 *
 * @retval -ERANGE Relocation value overflow
 * @retval 0 Successful relocation
 */
static int data_reloc_handler(elf_rela_t *rel, elf_word reloc_type, uintptr_t loc,
			      uintptr_t sym_base_addr)
{
	int64_t x;

	switch (reloc_type) {
	case R_AARCH64_ABS64:
		*(int64_t *)loc = reloc(AARCH64_RELOC_TYPE_ABS, loc, sym_base_addr, rel->r_addend);
		break;

	case R_AARCH64_ABS32:
		x = reloc(AARCH64_RELOC_TYPE_ABS, loc, sym_base_addr, rel->r_addend);
		if (x < 0 || x > UINT32_MAX) {
			return -ERANGE;
		}
		*(uint32_t *)loc = (uint32_t)x;
		break;

	case R_AARCH64_ABS16:
		x = reloc(AARCH64_RELOC_TYPE_ABS, loc, sym_base_addr, rel->r_addend);
		if (x < 0 || x > UINT16_MAX) {
			return -ERANGE;
		}
		*(uint16_t *)loc = (uint16_t)x;
		break;

	case R_AARCH64_PREL64:
		*(int64_t *)loc = reloc(AARCH64_RELOC_TYPE_PREL, loc, sym_base_addr, rel->r_addend);
		break;

	case R_AARCH64_PREL32:
		x = reloc(AARCH64_RELOC_TYPE_PREL, loc, sym_base_addr, rel->r_addend);
		if (x < INT32_MIN || x > INT32_MAX) {
			return -ERANGE;
		}
		*(int32_t *)loc = (int32_t)x;
		break;

	case R_AARCH64_PREL16:
		x = reloc(AARCH64_RELOC_TYPE_PREL, loc, sym_base_addr, rel->r_addend);
		if (x < INT16_MIN || x > INT16_MAX) {
			return -ERANGE;
		}
		*(int16_t *)loc = (int16_t)x;
		break;

	default:
		CODE_UNREACHABLE;
	}

	return 0;
}

/**
 * @brief Handler for relocations using MOV* instructions.
 *
 * @param[in] rel Relocation data provided by ELF
 * @param[in] reloc_type Type of relocation operation.
 * @param[in] loc Address of an opcode to rewrite (P in AArch64 ELF).
 * @param[in] sym_base_addr Address of the symbol referenced by relocation (S in AArch64 ELF).
 *
 * @retval -ERANGE Relocation value overflow
 * @retval 0 Successful relocation
 */
static int movw_reloc_handler(elf_rela_t *rel, elf_word reloc_type, uintptr_t loc,
			      uintptr_t sym_base_addr)
{
	int64_t x;
	uint32_t imm;
	int lsb = 0; /* LSB of X to be used */
	bool is_movnz = false;
	enum aarch64_reloc_type type = AARCH64_RELOC_TYPE_ABS;
	uint32_t opcode = sys_le32_to_cpu(*(uint32_t *)loc);

	switch (reloc_type) {
	case R_AARCH64_MOVW_SABS_G0:
		is_movnz = true;
	case R_AARCH64_MOVW_UABS_G0_NC:
	case R_AARCH64_MOVW_UABS_G0:
		break;

	case R_AARCH64_MOVW_SABS_G1:
		is_movnz = true;
	case R_AARCH64_MOVW_UABS_G1_NC:
	case R_AARCH64_MOVW_UABS_G1:
		lsb = 16;
		break;

	case R_AARCH64_MOVW_SABS_G2:
		is_movnz = true;
	case R_AARCH64_MOVW_UABS_G2_NC:
	case R_AARCH64_MOVW_UABS_G2:
		lsb = 32;
		break;

	case R_AARCH64_MOVW_UABS_G3:
		lsb = 48;
		break;

	case R_AARCH64_MOVW_PREL_G0:
		is_movnz = true;
	case R_AARCH64_MOVW_PREL_G0_NC:
		type = AARCH64_RELOC_TYPE_PREL;
		break;

	case R_AARCH64_MOVW_PREL_G1:
		is_movnz = true;
	case R_AARCH64_MOVW_PREL_G1_NC:
		type = AARCH64_RELOC_TYPE_PREL;
		lsb = 16;
		break;

	case R_AARCH64_MOVW_PREL_G2:
		is_movnz = true;
	case R_AARCH64_MOVW_PREL_G2_NC:
		type = AARCH64_RELOC_TYPE_PREL;
		lsb = 32;
		break;

	case R_AARCH64_MOVW_PREL_G3:
		is_movnz = true;
		type = AARCH64_RELOC_TYPE_PREL;
		lsb = 48;
		break;

	default:
		CODE_UNREACHABLE;
	}

	x = reloc(type, loc, sym_base_addr, rel->r_addend);
	imm = x >> lsb;

	/* Manipulate opcode for signed relocations. Result depends on sign of immediate value. */
	if (is_movnz) {
		opcode &= ~(AARCH64_MASK_MOV_OPCODE << AARCH64_SHIFT_MOV_OPCODE);

		if (x >= 0) {
			opcode |= (AARCH64_OPCODE_MOVN << AARCH64_SHIFT_MOV_OPCODE);
		} else {
			opcode |= (AARCH64_OPCODE_MOVZ << AARCH64_SHIFT_MOV_OPCODE);
			/* Need to invert immediate value for MOVZ. */
			imm = ~imm;
		}
	}

	opcode &= ~(AARCH64_MASK_IMM16 << AARCH64_SHIFT_MOV_IMM16);
	opcode |= (imm & AARCH64_MASK_IMM16) << AARCH64_SHIFT_MOV_IMM16;

	*(uint32_t *)loc = sys_cpu_to_le32(opcode);

	if (imm > UINT16_MAX) {
		return -ERANGE;
	}

	return 0;
}

/**
 * @brief Handler for static relocations except these related to MOV* instructions.
 *
 * @param[in] rel Relocation data provided by ELF
 * @param[in] reloc_type Type of relocation operation.
 * @param[in] loc Address of an opcode to rewrite (P in AArch64 ELF).
 * @param[in] sym_base_addr Address of the symbol referenced by relocation (S in AArch64 ELF).
 *
 * @retval -ERANGE Relocation value overflow
 * @retval 0 Successful relocation
 */
static int imm_reloc_handler(elf_rela_t *rel, elf_word reloc_type, uintptr_t loc,
			     uintptr_t sym_base_addr)
{
	int lsb = 2;    /* LSB of X to be used */
	int len;        /* bit length of immediate value */
	int shift = 10; /* shift of the immediate in instruction encoding */
	uint64_t imm;
	uint32_t bitmask = AARCH64_MASK_IMM12;
	int64_t x;
	bool is_adr = false;
	enum aarch64_reloc_type type = AARCH64_RELOC_TYPE_ABS;
	uint32_t opcode = sys_le32_to_cpu(*(uint32_t *)loc);

	switch (reloc_type) {
	case R_AARCH64_ADD_ABS_LO12_NC:
	case R_AARCH64_LDST8_ABS_LO12_NC:
		lsb = 0;
		len = 12;
		break;

	case R_AARCH64_LDST16_ABS_LO12_NC:
		lsb = 1;
		len = 11;
		break;

	case R_AARCH64_LDST32_ABS_LO12_NC:
		len = 10;
		break;

	case R_AARCH64_LDST64_ABS_LO12_NC:
		lsb = 3;
		len = 9;
		break;

	case R_AARCH64_LDST128_ABS_LO12_NC:
		lsb = 4;
		len = 8;
		break;

	case R_AARCH64_LD_PREL_LO19:
	case R_AARCH64_CONDBR19:
		type = AARCH64_RELOC_TYPE_PREL;
		bitmask = AARCH64_MASK_IMM19;
		shift = 5;
		len = 19;
		break;

	case R_AARCH64_ADR_PREL_LO21:
		type = AARCH64_RELOC_TYPE_PREL;
		is_adr = true;
		lsb = 0;
		len = 21;
		break;

	case R_AARCH64_TSTBR14:
		type = AARCH64_RELOC_TYPE_PREL;
		bitmask = AARCH64_MASK_IMM14;
		shift = 5;
		len = 14;
		break;

	case R_AARCH64_ADR_PREL_PG_HI21_NC:
	case R_AARCH64_ADR_PREL_PG_HI21:
		type = AARCH64_RELOC_TYPE_PAGE;
		is_adr = true;
		lsb = 12;
		len = 21;
		break;

	case R_AARCH64_CALL26:
	case R_AARCH64_JUMP26:
		type = AARCH64_RELOC_TYPE_PREL;
		bitmask = AARCH64_MASK_IMM26;
		shift = 0;
		len = 26;
		break;

	default:
		CODE_UNREACHABLE;
	}

	x = reloc(type, loc, sym_base_addr, rel->r_addend);
	x >>= lsb;

	imm = x & BIT_MASK(len);

	/* ADR instruction has immediate value split into two fields. */
	if (is_adr) {
		uint32_t immlo, immhi;

		immlo = (imm & AARCH64_MASK_ADR_IMMLO) << AARCH64_SHIFT_ADR_IMMLO;
		imm >>= AARCH64_ADR_IMMLO_BITS;
		immhi = (imm & AARCH64_MASK_ADR_IMMHI) << AARCH64_SHIFT_ADR_IMMHI;
		imm = immlo | immhi;

		shift = 0;
		bitmask = ((AARCH64_MASK_ADR_IMMLO << AARCH64_SHIFT_ADR_IMMLO) |
			   (AARCH64_MASK_ADR_IMMHI << AARCH64_SHIFT_ADR_IMMHI));
	}

	opcode &= ~(bitmask << shift);
	opcode |= (imm & bitmask) << shift;

	*(uint32_t *)loc = sys_cpu_to_le32(opcode);

	/* Mask X sign bit and upper bits. */
	x = (int64_t)(x & ~BIT_MASK(len - 1)) >> (len - 1);

	/* Incrementing X will either overflow and set it to 0 or
	 * set it 1. Any other case indicates that there was an overflow in relocation.
	 */
	if ((int64_t)x++ > 1) {
		return -ERANGE;
	}

	return 0;
}

/**
 * @brief Architecture specific function for relocating partially linked (static) elf
 *
 * Elf files contain a series of relocations described in a section. These relocation
 * instructions are architecture specific and each architecture supporting extensions
 * must implement this.
 *
 * The relocation codes for arm64 are well documented
 * https://github.com/ARM-software/abi-aa/blob/main/aaelf64/aaelf64.rst#relocation
 *
 * @param[in] rel Relocation data provided by ELF
 * @param[in] loc Address of an opcode to rewrite (P in AArch64 ELF)
 * @param[in] sym_base_addr Address of the symbol referenced by relocation (S in AArch64 ELF)
 * @param[in] sym_name Name of symbol referenced by relocation
 * @param[in] load_bias `.text` load address
 * @retval 0 Success
 * @retval -ENOTSUP Unsupported relocation
 * @retval -ENOEXEC Invalid relocation
 */
int arch_elf_relocate(elf_rela_t *rel, uintptr_t loc, uintptr_t sym_base_addr, const char *sym_name,
		      uintptr_t load_bias)
{
	int ret = 0;
	bool overflow_check = true;
	elf_word reloc_type = ELF_R_TYPE(rel->r_info);

	switch (reloc_type) {
	case R_ARM_NONE:
	case R_AARCH64_NONE:
		overflow_check = false;
		break;

	case R_AARCH64_ABS64:
	case R_AARCH64_PREL64:
		overflow_check = false;
	case R_AARCH64_ABS16:
	case R_AARCH64_ABS32:
	case R_AARCH64_PREL16:
	case R_AARCH64_PREL32:
		ret = data_reloc_handler(rel, reloc_type, loc, sym_base_addr);
		break;

	case R_AARCH64_MOVW_UABS_G0_NC:
	case R_AARCH64_MOVW_UABS_G1_NC:
	case R_AARCH64_MOVW_UABS_G2_NC:
	case R_AARCH64_MOVW_UABS_G3:
	case R_AARCH64_MOVW_PREL_G0_NC:
	case R_AARCH64_MOVW_PREL_G1_NC:
	case R_AARCH64_MOVW_PREL_G2_NC:
	case R_AARCH64_MOVW_PREL_G3:
		overflow_check = false;
	case R_AARCH64_MOVW_UABS_G0:
	case R_AARCH64_MOVW_UABS_G1:
	case R_AARCH64_MOVW_UABS_G2:
	case R_AARCH64_MOVW_SABS_G0:
	case R_AARCH64_MOVW_SABS_G1:
	case R_AARCH64_MOVW_SABS_G2:
	case R_AARCH64_MOVW_PREL_G0:
	case R_AARCH64_MOVW_PREL_G1:
	case R_AARCH64_MOVW_PREL_G2:
		ret = movw_reloc_handler(rel, reloc_type, loc, sym_base_addr);
		break;

	case R_AARCH64_ADD_ABS_LO12_NC:
	case R_AARCH64_LDST8_ABS_LO12_NC:
	case R_AARCH64_LDST16_ABS_LO12_NC:
	case R_AARCH64_LDST32_ABS_LO12_NC:
	case R_AARCH64_LDST64_ABS_LO12_NC:
	case R_AARCH64_LDST128_ABS_LO12_NC:
		overflow_check = false;
	case R_AARCH64_LD_PREL_LO19:
	case R_AARCH64_ADR_PREL_LO21:
	case R_AARCH64_TSTBR14:
	case R_AARCH64_CONDBR19:
		ret = imm_reloc_handler(rel, reloc_type, loc, sym_base_addr);
		break;

	case R_AARCH64_ADR_PREL_PG_HI21_NC:
		overflow_check = false;
	case R_AARCH64_ADR_PREL_PG_HI21:
		ret = imm_reloc_handler(rel, reloc_type, loc, sym_base_addr);
		break;

	case R_AARCH64_CALL26:
	case R_AARCH64_JUMP26:
		ret = imm_reloc_handler(rel, reloc_type, loc, sym_base_addr);
		/* TODO Handle case when address exceeds +/- 128MB */
		break;

	default:
		LOG_ERR("unknown relocation: %llu\n", reloc_type);
		return -ENOEXEC;
	}

	if (overflow_check && ret == -ERANGE) {
		LOG_ERR("sym '%s': relocation out of range (%#lx -> %#lx)\n", sym_name, loc,
			sym_base_addr);
		return -ENOEXEC;
	}

	return 0;
}
