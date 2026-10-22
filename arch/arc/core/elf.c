/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/llext/elf.h>
#include <zephyr/llext/llext.h>
#include <zephyr/llext/llext_internal.h>
#include <zephyr/llext/loader.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(elf, CONFIG_LLEXT_LOG_LEVEL);

#define R_ARC_32            4
#define R_ARC_B26           5 /* AKA R_ARC_64 */
#define R_ARC_S25H_PCREL    16
#define R_ARC_S25W_PCREL    17
#define R_ARC_32_ME         27

/* ARCompact insns packed in memory have Middle Endian encoding */
#define ME(x) (((x & 0xffff0000) >> 16) | ((x & 0xffff) << 16))

/**
 * @brief Architecture specific function for relocating shared elf
 *
 * Elf files contain a series of relocations described in multiple sections.
 * These relocation instructions are architecture specific and each architecture
 * supporting modules must implement this.
 *
 * The relocation codes are well documented:
 * https://github.com/foss-for-synopsys-dwc-arc-processors/arc-ABI-manual/blob/master/ARCv2_ABI.pdf
 * https://github.com/zephyrproject-rtos/binutils-gdb
 */
int arch_elf_relocate(struct llext_loader *ldr, struct llext *ext, elf_rela_t *rel,
		      const elf_shdr_t *shdr)
{
	int ret = 0;
	uint32_t value;
	const uintptr_t loc = llext_get_reloc_instruction_location(ldr, ext, shdr->sh_info, rel);
	uint32_t insn = UNALIGNED_GET((uint32_t *)loc);
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

	sym_base_addr += rel->r_addend;

	int reloc_type = ELF32_R_TYPE(rel->r_info);

	switch (reloc_type) {
	case R_ARC_32:
	case R_ARC_B26:
		UNALIGNED_PUT(sym_base_addr, (uint32_t *)loc);
		break;
	case R_ARC_S25H_PCREL:
		/* ((S + A) - P) >> 1
		 * S = symbol address
		 * A = addend
		 * P = relative offset to PCL
		 */
		value = (sym_base_addr + rel->r_addend - (loc & ~0x3)) >> 1;

		insn = ME(insn);

		/* disp25h */
		insn = insn & ~0x7feffcf;
		insn |= ((value >> 0) & 0x03ff) << 17;
		insn |= ((value >> 10) & 0x03ff) << 6;
		insn |= ((value >> 20) & 0x000f) << 0;

		insn = ME(insn);

		UNALIGNED_PUT(insn, (uint32_t *)loc);
		break;
	case R_ARC_S25W_PCREL:
		/* ((S + A) - P) >> 2 */
		value = (sym_base_addr + rel->r_addend - (loc & ~0x3)) >> 2;

		insn = ME(insn);

		/* disp25w */
		insn = insn & ~0x7fcffcf;
		insn |= ((value >> 0) & 0x01ff) << 18;
		insn |= ((value >> 9) & 0x03ff) << 6;
		insn |= ((value >> 19) & 0x000f) << 0;

		insn = ME(insn);

		UNALIGNED_PUT(insn, (uint32_t *)loc);
		break;
	case R_ARC_32_ME:
		UNALIGNED_PUT(ME(sym_base_addr), (uint32_t *)loc);
		break;
	default:
		LOG_ERR("unknown relocation: %u\n", reloc_type);
		ret = -ENOEXEC;
		break;
	}

	return ret;
}
