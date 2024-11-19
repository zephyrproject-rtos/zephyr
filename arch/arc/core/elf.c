/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/llext/elf.h>
#include <zephyr/llext/llext.h>
#include <zephyr/llext/loader.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(elf, CONFIG_LLEXT_LOG_LEVEL);

#define R_ARC_32            4
#define R_ARC_B26           5 /* AKA R_ARC_64 */
#define R_ARC_S25W_PCREL    17
#define R_ARC_32_ME         27

/* ARCompact insns packed in memory have Middle Endian encoding */
#define ME(x) ((x & 0xffff0000) >> 16) | ((x & 0xffff) << 16);

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
int arch_elf_relocate(elf_rela_t *rel, uintptr_t loc, uintptr_t sym_base_addr, const char *sym_name,
		      uintptr_t load_bias)
{
	int ret = 0;
	uint32_t insn = *(uint32_t *)loc;
	uint32_t value;

	sym_base_addr += rel->r_addend;

	int reloc_type = ELF32_R_TYPE(rel->r_info);

	switch (reloc_type) {
	case R_ARC_32:
	case R_ARC_B26:
		*(uint32_t *)loc = sym_base_addr;
		break;
	case R_ARC_S25W_PCREL:
		/* ((S + A) - P) >> 2
		 * S = symbol address
		 * A = addend
		 * P = relative offset to PCL
		 */
		value = (sym_base_addr + rel->r_addend - (loc & ~0x3)) >> 2;

		insn = ME(insn);

		/* disp25w */
		insn = insn & ~0x7fcffcf;
		insn |= ((value >> 0) & 0x01ff) << 18;
		insn |= ((value >> 9) & 0x03ff) << 6;
		insn |= ((value >> 19) & 0x000f) << 0;

		insn = ME(insn);

		*(uint32_t *)loc = insn;
		break;
	case R_ARC_32_ME:
		*(uint32_t *)loc = ME(sym_base_addr);
		break;
	default:
		LOG_ERR("unknown relocation: %u\n", reloc_type);
		ret = -ENOEXEC;
		break;
	}

	return ret;
}
