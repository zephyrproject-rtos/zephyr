/** @file
 * @brief Architecture-specific relocations for RISC-V instruction sets.
 */
/*
 * Copyright (c) 2024 CISPA Helmholtz Center for Information Security gGmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/llext/elf.h>
#include <zephyr/llext/llext.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <zephyr/arch/riscv/elf.h>

#include <stdlib.h>

LOG_MODULE_REGISTER(elf, CONFIG_LLEXT_LOG_LEVEL);

/*
 * RISC-V relocations commonly use pairs of U-type and I-type instructions.
 * U-type instructions have 20-bit immediates, I-type instructions have 12-bit immediates.
 * Immediates in RISC-V are always sign-extended.
 * Thereby, this type of relocation can reach any address within a 2^31-1 byte range.
 */
#define RISCV_MAX_JUMP_DISTANCE_U_PLUS_I_TYPE INT32_MAX

/* S-type has 12-bit signed immediate */
#define RISCV_MAX_JUMP_DISTANCE_S_TYPE ((1 << 11) - 1)

/* I-type has 12-bit signed immediate also */
#define RISCV_MAX_JUMP_DISTANCE_I_TYPE ((1 << 11) - 1)

/* B-type has 13-bit signed immediate */
#define RISCV_MAX_JUMP_DISTANCE_B_TYPE ((1 << 12) - 1)

/* CB-type has 9-bit signed immediate */
#define RISCV_MAX_JUMP_DISTANCE_CB_TYPE ((1 << 8) - 1)

/* CJ-type has 12-bit signed immediate (last bit implicit 0) */
#define RISCV_MAX_JUMP_DISTANCE_CJ_TYPE ((1 << 11) - 1)

static inline int riscv_relocation_fits(long long jump_target, long long max_distance,
					elf_word reloc_type)
{
	if (llabs(jump_target) > max_distance) {
		LOG_ERR("%lld byte relocation is not possible for type %" PRIu64 " (max %lld)!",
			jump_target, (uint64_t)reloc_type, max_distance);
		return -ENOEXEC; /* jump too far */
	}

	return 0;
}

static long long last_u_type_jump_target;

/**
 * @brief RISC-V specific function for relocating partially linked ELF binaries
 *
 * This implementation follows the official RISC-V specification:
 * https://github.com/riscv-non-isa/riscv-elf-psabi-doc/blob/master/riscv-elf.adoc
 *
 */
int arch_elf_relocate(elf_rela_t *rel, uintptr_t loc_unsigned, uintptr_t sym_base_addr_unsigned,
		      const char *sym_name, uintptr_t load_bias)
{
	/* FIXME currently, RISC-V relocations all fit in ELF_32_R_TYPE */
	elf_word reloc_type = ELF32_R_TYPE(rel->r_info);
	/*
	 * The RISC-V specification uses the following symbolic names for the relocations:
	 *
	 * A - addend (rel->r_addend)
	 * B - base address (load_bias)
	 * G - global offset table (not supported yet)
	 * P - position of the relocation (loc)
	 * S - symbol value (sym_base_addr)
	 * V - value at the relocation position (*loc)
	 * GP - value of __global_pointer$ (not supported yet)
	 * TLSMODULE - TLS module for the object (not supported yet)
	 * TLSOFFSET - TLS static block for the object (not supported yet)
	 */
	intptr_t loc = (intptr_t)loc_unsigned;
	uint8_t *loc8 = (uint8_t *)loc, tmp8;
	uint16_t *loc16 = (uint16_t *)loc, tmp16;
	uint32_t *loc32 = (uint32_t *)loc, tmp32;
	uint64_t *loc64 = (uint64_t *)loc, tmp64;
	/* uint32_t or uint64_t */
	r_riscv_wordclass_t *loc_word = (r_riscv_wordclass_t *)loc;
	uint32_t modified_operand;
	uint16_t modified_compressed_operand;
	int32_t imm8;
	long long original_imm8, jump_target;
	int16_t compressed_imm8;
	__typeof__(rel->r_addend) target_alignment = 1;
	const intptr_t sym_base_addr = (intptr_t)sym_base_addr_unsigned;

	LOG_DBG("Relocating symbol %s at %p with base address %p load address %p type %" PRIu64,
		sym_name, (void *)loc, (void *)sym_base_addr, (void *)load_bias,
		(uint64_t)reloc_type);

	/* FIXME not all types of relocations currently supported, especially TLS */

	switch (reloc_type) {
	case R_RISCV_NONE:
		break;
	case R_RISCV_32:
		jump_target = sym_base_addr + rel->r_addend; /* S + A */
		UNALIGNED_PUT((uint32_t)jump_target, loc32);
		return riscv_relocation_fits(jump_target, INT32_MAX, reloc_type);
	case R_RISCV_64:
		/* full 64-bit range, need no range check */
		UNALIGNED_PUT(sym_base_addr + rel->r_addend, loc64); /* S + A */
		break;
	case R_RISCV_RELATIVE:
		/* either full 32-bit or 64-bit range, need no range check */
		UNALIGNED_PUT(load_bias + rel->r_addend, loc_word); /* B + A */
		break;
	case R_RISCV_JUMP_SLOT:
		/* either full 32-bit or 64-bit range, need no range check */
		UNALIGNED_PUT(sym_base_addr, loc_word); /* S */
		break;
	case R_RISCV_BRANCH:
		jump_target = sym_base_addr + rel->r_addend - loc; /* S + A - P */
		modified_operand = UNALIGNED_GET(loc32);
		imm8 = jump_target;
		modified_operand = R_RISCV_CLEAR_BTYPE_IMM8(modified_operand);
		modified_operand = R_RISCV_SET_BTYPE_IMM8(modified_operand, imm8);
		UNALIGNED_PUT(modified_operand, loc32);
		return riscv_relocation_fits(jump_target, RISCV_MAX_JUMP_DISTANCE_B_TYPE,
					     reloc_type);
	case R_RISCV_JAL:
		jump_target = sym_base_addr + rel->r_addend - loc; /* S + A - P */
		modified_operand = UNALIGNED_GET(loc32);
		imm8 = jump_target;
		modified_operand = R_RISCV_CLEAR_JTYPE_IMM8(modified_operand);
		modified_operand = R_RISCV_SET_JTYPE_IMM8(modified_operand, imm8);
		UNALIGNED_PUT(modified_operand, loc32);
		return riscv_relocation_fits(jump_target, RISCV_MAX_JUMP_DISTANCE_U_PLUS_I_TYPE,
					     reloc_type);
	case R_RISCV_CALL:
	case R_RISCV_CALL_PLT:
	case R_RISCV_PCREL_HI20:
		modified_operand = UNALIGNED_GET(loc32);
		jump_target = sym_base_addr + rel->r_addend - loc; /* S + A - P */
		imm8 = jump_target;
		/* bit 12 of the immediate goes to I-type instruction and might
		 * change the sign of the number
		 */
		/* in order to avoid that, we add 1 to the upper immediate if bit 12 is one */
		/* see RISC-V la pseudo instruction */
		imm8 += imm8 & 0x800;

		original_imm8 = imm8;

		modified_operand = R_RISCV_CLEAR_UTYPE_IMM8(modified_operand);
		modified_operand = R_RISCV_SET_UTYPE_IMM8(modified_operand, imm8);
		UNALIGNED_PUT(modified_operand, loc32);

		if (reloc_type != R_RISCV_PCREL_HI20) {
			/* PCREL_HI20 is only U-type, not truly U+I-type */
			/* for the others, need to also modify following I-type */
			loc32++;

			imm8 = jump_target;

			modified_operand = UNALIGNED_GET(loc32);
			modified_operand = R_RISCV_CLEAR_ITYPE_IMM8(modified_operand);
			modified_operand = R_RISCV_SET_ITYPE_IMM8(modified_operand, imm8);
			UNALIGNED_PUT(modified_operand, loc32);
		}

		last_u_type_jump_target = jump_target;

		return riscv_relocation_fits(jump_target, RISCV_MAX_JUMP_DISTANCE_U_PLUS_I_TYPE,
					     reloc_type);
	case R_RISCV_PCREL_LO12_I:
		/* need the same jump target as preceding U-type relocation */
		if (last_u_type_jump_target == 0) {
			LOG_ERR("R_RISCV_PCREL_LO12_I relocation without preceding U-type "
				"relocation!");
			return -ENOEXEC;
		}
		modified_operand = UNALIGNED_GET(loc32);
		jump_target = last_u_type_jump_target; /* S - P */
		last_u_type_jump_target = 0;
		imm8 = jump_target;
		modified_operand = R_RISCV_CLEAR_ITYPE_IMM8(modified_operand);
		modified_operand = R_RISCV_SET_ITYPE_IMM8(modified_operand, imm8);
		UNALIGNED_PUT(modified_operand, loc32);
		return riscv_relocation_fits(jump_target, RISCV_MAX_JUMP_DISTANCE_U_PLUS_I_TYPE,
					     reloc_type);
		break;
	case R_RISCV_PCREL_LO12_S:
		/* need the same jump target as preceding U-type relocation */
		if (last_u_type_jump_target == 0) {
			LOG_ERR("R_RISCV_PCREL_LO12_I relocation without preceding U-type "
				"relocation!");
			return -ENOEXEC;
		}
		modified_operand = UNALIGNED_GET(loc32);
		jump_target = last_u_type_jump_target; /* S - P */
		last_u_type_jump_target = 0;
		imm8 = jump_target;
		modified_operand = R_RISCV_CLEAR_STYPE_IMM8(modified_operand);
		modified_operand = R_RISCV_SET_STYPE_IMM8(modified_operand, imm8);
		UNALIGNED_PUT(modified_operand, loc32);
		return riscv_relocation_fits(jump_target, RISCV_MAX_JUMP_DISTANCE_U_PLUS_I_TYPE,
					     reloc_type);
	case R_RISCV_HI20:
		jump_target = sym_base_addr + rel->r_addend; /* S + A */
		modified_operand = UNALIGNED_GET(loc32);
		imm8 = jump_target;
		/* bit 12 of the immediate goes to I-type instruction and might
		 * change the sign of the number
		 */
		/* in order to avoid that, we add 1 to the upper immediate if bit 12 is one*/
		/* see RISC-V la pseudo instruction */
		original_imm8 = imm8;
		imm8 += imm8 & 0x800;
		modified_operand = R_RISCV_CLEAR_UTYPE_IMM8(modified_operand);
		modified_operand = R_RISCV_SET_UTYPE_IMM8(modified_operand, imm8);
		UNALIGNED_PUT(modified_operand, loc32);
		return riscv_relocation_fits(jump_target, RISCV_MAX_JUMP_DISTANCE_U_PLUS_I_TYPE,
					     reloc_type);
	case R_RISCV_LO12_I:
		modified_operand = UNALIGNED_GET(loc32);
		jump_target = sym_base_addr + rel->r_addend; /* S + A */
		imm8 = jump_target;
		/* this is always used with R_RISCV_HI20 */
		modified_operand = R_RISCV_CLEAR_ITYPE_IMM8(modified_operand);
		modified_operand = R_RISCV_SET_ITYPE_IMM8(modified_operand, imm8);
		UNALIGNED_PUT(modified_operand, loc32);
		return riscv_relocation_fits(jump_target, RISCV_MAX_JUMP_DISTANCE_U_PLUS_I_TYPE,
					     reloc_type);
	case R_RISCV_LO12_S:
		modified_operand = UNALIGNED_GET(loc32);
		imm8 = sym_base_addr + rel->r_addend; /* S + A */
		/*
		 * S-type is used for stores/loads etc.
		 * size check is done at compile time, as it depends on the size of
		 * the structure we are trying to load/store
		 */
		modified_operand = R_RISCV_CLEAR_STYPE_IMM8(modified_operand);
		modified_operand = R_RISCV_SET_STYPE_IMM8(modified_operand, imm8);
		UNALIGNED_PUT(modified_operand, loc32);
		break;
	/* for add/sub/set, compiler needs to ensure that the ELF sections are close enough */
	case R_RISCV_ADD8:
		tmp8 = UNALIGNED_GET(loc8);
		tmp8 += sym_base_addr + rel->r_addend; /* V + S + A */
		UNALIGNED_PUT(tmp8, loc8);
		break;
	case R_RISCV_ADD16:
		tmp16 = UNALIGNED_GET(loc16);
		tmp16 += sym_base_addr + rel->r_addend; /* V + S + A */
		UNALIGNED_PUT(tmp16, loc16);
		break;
	case R_RISCV_ADD32:
		tmp32 = UNALIGNED_GET(loc32);
		tmp32 += sym_base_addr + rel->r_addend; /* V + S + A */
		UNALIGNED_PUT(tmp32, loc32);
		break;
	case R_RISCV_ADD64:
		tmp64 = UNALIGNED_GET(loc64);
		tmp64 += sym_base_addr + rel->r_addend; /* V + S + A */
		UNALIGNED_PUT(tmp64, loc64);
		break;
	case R_RISCV_SUB8:
		tmp8 = UNALIGNED_GET(loc8);
		tmp8 -= sym_base_addr + rel->r_addend; /* V - S - A */
		UNALIGNED_PUT(tmp8, loc8);
		break;
	case R_RISCV_SUB16:
		tmp16 = UNALIGNED_GET(loc16);
		tmp16 -= sym_base_addr + rel->r_addend; /* V - S - A */
		UNALIGNED_PUT(tmp16, loc16);
		break;
	case R_RISCV_SUB32:
		tmp32 = UNALIGNED_GET(loc32);
		tmp32 -= sym_base_addr + rel->r_addend; /* V - S - A */
		UNALIGNED_PUT(tmp32, loc32);
		break;
	case R_RISCV_SUB64:
		tmp64 = UNALIGNED_GET(loc64);
		tmp64 -= sym_base_addr + rel->r_addend; /* V - S - A */
		UNALIGNED_PUT(tmp64, loc64);
		break;
	case R_RISCV_SUB6:
		tmp8 = UNALIGNED_GET(loc8) & (0x1F);
		UNALIGNED_PUT(tmp8, loc8);
		tmp8 = tmp8 - sym_base_addr - rel->r_addend; /* V - S - A */
		tmp8 = tmp8 & (0x1F);
		tmp8 = tmp8 | UNALIGNED_GET(loc8);
		UNALIGNED_PUT(tmp8, loc8);
		break;
	case R_RISCV_SET6:
		tmp8 = UNALIGNED_GET(loc8) & (0x1F);
		UNALIGNED_PUT(tmp8, loc8);
		tmp8 = sym_base_addr + rel->r_addend; /* S + A */
		tmp8 = tmp8 | UNALIGNED_GET(loc8);
		UNALIGNED_PUT(tmp8, loc8);
		break;
	case R_RISCV_SET8:
		tmp8 = sym_base_addr + rel->r_addend; /* S + A */
		UNALIGNED_PUT(tmp8, loc8);
		break;
	case R_RISCV_SET16:
		tmp16 = sym_base_addr + rel->r_addend; /* S + A */
		UNALIGNED_PUT(tmp16, loc16);
		break;
	case R_RISCV_SET32:
		tmp32 = sym_base_addr + rel->r_addend; /* S + A */
		UNALIGNED_PUT(tmp32, loc32);
		break;
	case R_RISCV_32_PCREL:
		jump_target = sym_base_addr + rel->r_addend - loc; /* S + A - P */
		tmp32 = jump_target;
		UNALIGNED_PUT(tmp32, loc32);
		return riscv_relocation_fits(jump_target, RISCV_MAX_JUMP_DISTANCE_U_PLUS_I_TYPE,
					     reloc_type);
	case R_RISCV_PLT32:
		jump_target = sym_base_addr + rel->r_addend - loc; /* S + A - P */
		tmp32 = jump_target;
		UNALIGNED_PUT(tmp32, loc32);
		return riscv_relocation_fits(jump_target, RISCV_MAX_JUMP_DISTANCE_U_PLUS_I_TYPE,
					     reloc_type);
	case R_RISCV_RVC_BRANCH:
		jump_target = sym_base_addr + rel->r_addend - loc; /* S + A - P */
		modified_compressed_operand = UNALIGNED_GET(loc16);
		compressed_imm8 = jump_target;
		modified_compressed_operand =
			R_RISCV_CLEAR_CBTYPE_IMM8(modified_compressed_operand);
		modified_compressed_operand =
			R_RISCV_SET_CBTYPE_IMM8(modified_compressed_operand, compressed_imm8);
		UNALIGNED_PUT(modified_compressed_operand, loc16);
		return riscv_relocation_fits(jump_target, RISCV_MAX_JUMP_DISTANCE_CB_TYPE,
					     reloc_type);
	case R_RISCV_RVC_JUMP:
		jump_target = sym_base_addr + rel->r_addend - loc; /* S + A - P */
		modified_compressed_operand = UNALIGNED_GET(loc16);
		compressed_imm8 = jump_target;
		modified_compressed_operand =
			R_RISCV_CLEAR_CJTYPE_IMM8(modified_compressed_operand);
		modified_compressed_operand =
			R_RISCV_SET_CJTYPE_IMM8(modified_compressed_operand, compressed_imm8);
		UNALIGNED_PUT(modified_compressed_operand, loc16);
		return riscv_relocation_fits(jump_target, RISCV_MAX_JUMP_DISTANCE_CJ_TYPE,
					     reloc_type);
	case R_RISCV_ALIGN:
		/* we are supposed to move the symbol such that it is aligned to the next power of
		 * two >= addend
		 */
		/* this involves moving the symbol */
		while (target_alignment < rel->r_addend) {
			target_alignment *= 2;
		}
		LOG_ERR("Symbol %s with location %p requires alignment to %" PRIu64 " bytes!",
			sym_name, (void *)loc, (uint64_t)target_alignment);
		LOG_ERR("Alignment relocation is currently not supported!");
		return -ENOEXEC;
	/* ignored, this is primarily intended for removing instructions during link-time
	 * optimization
	 */
	case R_RISCV_RELAX:
		break;
	default:
		LOG_ERR("Unsupported relocation type: %" PRIu64 " for symbol: %s",
			(uint64_t)reloc_type, sym_name);
		return -ENOEXEC;
	}

	return 0;
}
