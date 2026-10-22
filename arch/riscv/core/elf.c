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
#include <zephyr/llext/llext_internal.h>
#include <zephyr/llext/loader.h>

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
	/*
	 * two's complement encoding
	 * e.g., [-128=0b10000000, 127=0b01111111] encodable with 8 bits
	 */
	if (jump_target < 0) {
		max_distance++;
	}
	if (llabs(jump_target) > max_distance) {
		LOG_ERR("%lld byte relocation is not possible for type %" PRIu64 " (max %lld)!",
			jump_target, (uint64_t)reloc_type, max_distance);
		return -ENOEXEC; /* jump too far */
	}

	return 0;
}

static size_t riscv_last_rel_idx;

/**
 * @brief On RISC-V, PC-relative relocations (PCREL_LO12_I, PCREL_LO12_S) do not refer to
 * the actual symbol. Instead, they refer to the location of a different instruction in the
 * same section, which has a PCREL_HI20 relocation. The relocation offset is then computed based
 * on the location and symbol from the HI20 relocation. 20 bits from the offset go into the
 * instruction that has the HI20 relocation, and 12 bits go into the PCREL_LO12 instruction.
 *
 * @param[in] ldr llext loader
 * @param[in] ext current extension
 * @param[in] pcrel_lo12 the elf relocation structure for the PCREL_LO12I/S relocation.
 * @param[in] shdr ELF section header for the relocation
 * @param[in] sym ELF symbol for PCREL_LO12I
 * @param[out] link_addr_out computed link address
 *
 */
static int llext_riscv_find_sym_pcrel(struct llext_loader *ldr, struct llext *ext,
				      const elf_rela_t *pcrel_lo12, const elf_shdr_t *shdr,
				      const elf_sym_t *sym, intptr_t *link_addr_out)
{
	int ret;
	elf_rela_t candidate;
	uintptr_t candidate_loc;
	elf_word reloc_type;
	elf_sym_t candidate_sym;
	uintptr_t link_addr;
	const char *symbol_name;
	int iteration_start = riscv_last_rel_idx;
	bool is_first = true;
	const elf_word rel_cnt = shdr->sh_size / shdr->sh_entsize;
	const uintptr_t sect_base = (uintptr_t)llext_loaded_sect_ptr(ldr, ext, shdr->sh_info);
	bool found_candidate = false;

	if (iteration_start >= rel_cnt) {
		/* value left over from a different section */
		iteration_start = 0;
	}

	reloc_type = ELF32_R_TYPE(pcrel_lo12->r_info);

	if (reloc_type != R_RISCV_PCREL_LO12_I && reloc_type != R_RISCV_PCREL_LO12_S) {
		/* this function does not apply - the symbol is already correct */
		return 0;
	}

	for (int i = iteration_start; i != iteration_start || is_first; i++) {

		is_first = false;

		/* get each relocation entry */
		ret = llext_seek(ldr, shdr->sh_offset + i * shdr->sh_entsize);
		if (ret != 0) {
			return ret;
		}

		ret = llext_read(ldr, &candidate, shdr->sh_entsize);
		if (ret != 0) {
			return ret;
		}

		/* FIXME currently, RISC-V relocations all fit in ELF_32_R_TYPE */
		reloc_type = ELF32_R_TYPE(candidate.r_info);

		candidate_loc = sect_base + candidate.r_offset;

		/*
		 * RISC-V ELF specification: "value" of the symbol for the PCREL_LO12 relocation
		 * is actually the offset of the PCREL_HI20 relocation instruction from section
		 * start
		 */
		if (candidate.r_offset == sym->st_value && reloc_type == R_RISCV_PCREL_HI20) {
			found_candidate = true;

			/*
			 * start here in next iteration
			 * it is fairly likely (albeit not guaranteed) that we require PCREL_HI20
			 * relocations in order
			 * we can safely write this even if an error occurs after the loop -
			 * in that case,we can safely abort the execution anyway
			 */
			riscv_last_rel_idx = i;

			break;
		}

		if (i + 1 >= rel_cnt) {
			/* wrap around and search in previously processed indices as well */
			i = -1;
		}
	}

	if (!found_candidate) {
		LOG_ERR("Could not find R_RISCV_PCREL_HI20 relocation for "
			"R_RISCV_PCREL_LO12 relocation!");
		return -ENOEXEC;
	}

	/* we found a match - need to compute the relocation for this instruction */
	/* lower 12 bits go to the PCREL_LO12 relocation */

	/* get corresponding / "actual" symbol */
	ret = llext_seek(ldr, ldr->sects[LLEXT_MEM_SYMTAB].sh_offset +
			 ELF_R_SYM(candidate.r_info) * sizeof(elf_sym_t));
	if (ret != 0) {
		return ret;
	}

	ret = llext_read(ldr, &candidate_sym, sizeof(elf_sym_t));
	if (ret != 0) {
		return ret;
	}

	symbol_name = llext_symbol_name(ldr, ext, &candidate_sym);

	ret = llext_lookup_symbol(ldr, ext, &link_addr, &candidate, &candidate_sym,
				  symbol_name, shdr);

	if (ret != 0) {
		return ret;
	}

	*link_addr_out = (intptr_t)(link_addr + candidate.r_addend - candidate_loc); /* S + A - P */

	/* found the matching entry */
	return 0;
}

/**
 * @brief RISC-V specific function for relocating partially linked ELF binaries
 *
 * This implementation follows the official RISC-V specification:
 * https://github.com/riscv-non-isa/riscv-elf-psabi-doc/blob/master/riscv-elf.adoc
 *
 */
int arch_elf_relocate(struct llext_loader *ldr, struct llext *ext, elf_rela_t *rel,
		      const elf_shdr_t *shdr)
{
	/* FIXME currently, RISC-V relocations all fit in ELF_32_R_TYPE */
	elf_word reloc_type = ELF32_R_TYPE(rel->r_info);
	const uintptr_t load_bias = (uintptr_t)ext->mem[LLEXT_MEM_TEXT];
	const uintptr_t loc_unsigned = llext_get_reloc_instruction_location(ldr, ext,
									    shdr->sh_info, rel);
	elf_sym_t sym;
	uintptr_t sym_base_addr_unsigned;
	const char *sym_name;
	int ret;

	ret = llext_read_symbol(ldr, ext, rel, &sym);
	if (ret != 0) {
		LOG_ERR("Could not read symbol from binary!");
		return ret;
	}

	sym_name = llext_symbol_name(ldr, ext, &sym);
	ret = llext_lookup_symbol(ldr, ext, &sym_base_addr_unsigned, rel, &sym, sym_name, shdr);

	if (ret != 0) {
		LOG_ERR("Could not find symbol %s!", sym_name);
		return ret;
	}
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
	intptr_t sym_base_addr = (intptr_t)sym_base_addr_unsigned;

	/*
	 * For HI20/LO12 ("PCREL") relocation pairs, we need a helper function to
	 * determine the address for the LO12 relocation, as it depends on the
	 * value in the HI20 relocation.
	 */
	ret = llext_riscv_find_sym_pcrel(ldr, ext, rel, shdr, &sym, &sym_base_addr);

	if (ret != 0) {
		LOG_ERR("Failed to resolve RISC-V PCREL relocation for symbol %s at %p "
			"with base address %p load address %p type %" PRIu64,
			sym_name, (void *)loc, (void *)sym_base_addr, (void *)load_bias,
			(uint64_t)reloc_type);
		return ret;
	}

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

		return riscv_relocation_fits(jump_target, RISCV_MAX_JUMP_DISTANCE_U_PLUS_I_TYPE,
					     reloc_type);
	case R_RISCV_PCREL_LO12_I:
		/*
		 * Jump target is resolved in llext_riscv_find_sym_pcrel in llext_link.c
		 * as it depends on other relocations.
		 */
		modified_operand = UNALIGNED_GET(loc32);
		imm8 = (int32_t)sym_base_addr; /* already computed */
		modified_operand = R_RISCV_CLEAR_ITYPE_IMM8(modified_operand);
		modified_operand = R_RISCV_SET_ITYPE_IMM8(modified_operand, imm8);
		UNALIGNED_PUT(modified_operand, loc32);
		/* we have checked that this fits with the associated relocation */
		break;
	case R_RISCV_PCREL_LO12_S:
		/*
		 * Jump target is resolved in llext_riscv_find_sym_pcrel in llext_link.c
		 * as it depends on other relocations.
		 */
		modified_operand = UNALIGNED_GET(loc32);
		imm8 = (int32_t)sym_base_addr; /* already computed */
		modified_operand = R_RISCV_CLEAR_STYPE_IMM8(modified_operand);
		modified_operand = R_RISCV_SET_STYPE_IMM8(modified_operand, imm8);
		UNALIGNED_PUT(modified_operand, loc32);
		/* we have checked that this fits with the associated relocation */
		break;
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
