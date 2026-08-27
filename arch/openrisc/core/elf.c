/*
 * Copyright (c) 2025 NVIDIA Corporation <jholdsworth@nvidia.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OpenRISC (or1k) ELF relocation support for LLEXT.
 *
 * Implements arch_elf_relocate() for the OpenRISC architecture,
 * handling the relocation types needed for statically-linked
 * loadable extensions (ELF objects / relocatable ELFs).
 */

#include <zephyr/llext/elf.h>
#include <zephyr/llext/llext.h>
#include <zephyr/llext/llext_internal.h>
#include <zephyr/llext/loader.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <zephyr/arch/openrisc/elf.h>

LOG_MODULE_REGISTER(elf, CONFIG_LLEXT_LOG_LEVEL);

/**
 * @brief Apply an OR1K ELF relocation.
 *
 * Nomenclature (following ELF specification conventions):
 *   S = symbol value (sym_base_addr)
 *   A = addend (rel->r_addend)
 *   P = place / location of the relocation (loc)
 */
int arch_elf_relocate(struct llext_loader *ldr, struct llext *ext, elf_rela_t *rel,
		      const elf_shdr_t *shdr)
{
	const elf_word reloc_type = ELF32_R_TYPE(rel->r_info);
	const uintptr_t loc = llext_get_reloc_instruction_location(ldr, ext, shdr->sh_info, rel);
	elf_sym_t sym;
	uintptr_t sym_base_addr;
	uint32_t insn;
	intptr_t value;
	int ret;

	ret = llext_read_symbol(ldr, ext, rel, &sym);
	if (ret != 0) {
		return ret;
	}

	const char *const sym_name = llext_symbol_name(ldr, ext, &sym);

	ret = llext_lookup_symbol(ldr, ext, &sym_base_addr, rel, &sym, sym_name, shdr);
	if (ret != 0) {
		LOG_ERR("Could not find symbol %s!", sym_name);
		return ret;
	}

	LOG_DBG("Relocating symbol %s at %p with base addr %p type %" PRIu32,
		sym_name, (void *)loc, (void *)sym_base_addr, reloc_type);

	switch (reloc_type) {
	case R_OR1K_NONE:
		break;

	/* --- Absolute data relocations --- */

	case R_OR1K_32:
	case R_OR1K_TLS_TPOFF:
		/* S + A (32-bit absolute) */
		*(uint32_t *)loc += sym_base_addr + rel->r_addend;
		break;

	case R_OR1K_32_PCREL:
		/* S + A - P (32-bit PC-relative) */
		*(uint32_t *)loc += sym_base_addr + rel->r_addend - loc;
		break;

	/* --- Instruction-immediate relocations --- */

	case R_OR1K_INSN_REL_26:
		/* (S + A - P) >> 2, placed in insn[25:0] */
		value = (intptr_t)(sym_base_addr + rel->r_addend) - (intptr_t)loc;
		if (value & 3) {
			LOG_ERR("Misaligned branch target for %s: 0x%lx",
				sym_name, (unsigned long)value);
			return -ENOEXEC;
		}
		insn = *(uint32_t *)loc;
		insn = (insn & ~R_OR1K_INSN_REL_26_MASK) |
		       (((uint32_t)(value >> 2)) & R_OR1K_INSN_REL_26_MASK);
		*(uint32_t *)loc = insn;
		break;

	case R_OR1K_HI_16_IN_INSN:
	case R_OR1K_TLS_LE_HI16:
		/* (S + A) >> 16, placed in insn[15:0] */
		value = (intptr_t)(sym_base_addr + rel->r_addend);
		insn = *(uint32_t *)loc;
		insn = (insn & ~R_OR1K_IMM16_MASK) |
		       (((uint32_t)(value >> 16)) & R_OR1K_IMM16_MASK);
		*(uint32_t *)loc = insn;
		break;

	case R_OR1K_LO_16_IN_INSN:
	case R_OR1K_TLS_LE_LO16:
		/* (S + A) & 0xFFFF, placed in insn[15:0] */
		value = (intptr_t)(sym_base_addr + rel->r_addend);
		insn = *(uint32_t *)loc;
		insn = (insn & ~R_OR1K_IMM16_MASK) |
		       ((uint32_t)value & R_OR1K_IMM16_MASK);
		*(uint32_t *)loc = insn;
		break;

	case R_OR1K_AHI16:
	case R_OR1K_TLS_LE_AHI16:
		/* ((S + A) + 0x8000) >> 16, placed in insn[15:0]
		 * "Adjusted high" compensates for sign-extension of paired LO16.
		 */
		value = (intptr_t)(sym_base_addr + rel->r_addend) + 0x8000;
		insn = *(uint32_t *)loc;
		insn = (insn & ~R_OR1K_IMM16_MASK) |
		       (((uint32_t)(value >> 16)) & R_OR1K_IMM16_MASK);
		*(uint32_t *)loc = insn;
		break;

	case R_OR1K_SLO16:
	case R_OR1K_TLS_LE_SLO16:
		/* (S + A) & 0xFFFF, in split-immediate store format:
		 *   insn[25:21] = imm[15:11]
		 *   insn[10:0]  = imm[10:0]
		 */
		value = (intptr_t)(sym_base_addr + rel->r_addend);
		insn = *(uint32_t *)loc;
		insn = R_OR1K_SLO_PACK(insn, (uint32_t)value & R_OR1K_IMM16_MASK);
		*(uint32_t *)loc = insn;
		break;

	/* --- Page-relative relocations (ADRP-style, 8KB pages) --- */

	case R_OR1K_PCREL_PG21:
		/* ((S + A) & PAGE_MASK) - (P & PAGE_MASK)) >> 13, placed in insn[20:0] */
		value = (intptr_t)(((sym_base_addr + rel->r_addend) & R_OR1K_PAGE_MASK) -
				   (loc & R_OR1K_PAGE_MASK));
		insn = *(uint32_t *)loc;
		insn = (insn & ~R_OR1K_PG21_MASK) |
		       (((uint32_t)(value >> 13)) & R_OR1K_PG21_MASK);
		*(uint32_t *)loc = insn;
		break;

	case R_OR1K_LO13:
		/* (S + A) & 0x1FFF, placed in insn[15:0] (only 13 bits meaningful) */
		value = (intptr_t)(sym_base_addr + rel->r_addend);
		insn = *(uint32_t *)loc;
		insn = (insn & ~R_OR1K_IMM16_MASK) |
		       ((uint32_t)value & (R_OR1K_PAGE_SIZE - 1U));
		*(uint32_t *)loc = insn;
		break;

	case R_OR1K_SLO13:
		/* (S + A) & 0x1FFF, in split-immediate store format */
		value = (intptr_t)(sym_base_addr + rel->r_addend);
		insn = *(uint32_t *)loc;
		insn = R_OR1K_SLO_PACK(insn, (uint32_t)value & (R_OR1K_PAGE_SIZE - 1U));
		*(uint32_t *)loc = insn;
		break;

	default:
		LOG_ERR("Unsupported relocation type %" PRIu32 " for symbol %s at %p",
			reloc_type, sym_name, (void *)loc);
		return -ENOEXEC;
	}

	return 0;
}
