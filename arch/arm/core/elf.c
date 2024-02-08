/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/llext/elf.h>
#include <zephyr/llext/llext.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util_macro.h>

LOG_MODULE_REGISTER(elf, CONFIG_LLEXT_LOG_LEVEL);

int32_t read_thm_b_addend(uint8_t *addr)
{
	uint32_t S = (*(uint16_t *)addr >> 10) & 1;
	uint32_t J1 = (*(uint16_t *)(addr + 2) >> 13) & 1;
	uint32_t J2 = (*(uint16_t *)(addr + 2) >> 11) & 1;
	uint32_t I1 = !(J1 ^ S);
	uint32_t I2 = !(J2 ^ S);
	uint32_t imm10 = READ_BITS(*(uint16_t *)addr, 9, 0);
	uint32_t imm11 = READ_BITS(*(uint16_t *)(addr + 2), 10, 0);
	uint32_t val = (S << 24) | (I1 << 23) | (I2 << 22) | (imm10 << 12) | (imm11 << 1);

	return SIGN_EXTEND32(val, 24);
}

void rewrite_thm_b_imm(uint8_t *addr, uint32_t imm)
{
	uint32_t sign = (imm >> 24) & 1;
	uint32_t I1 = (imm >> 23) & 1;
	uint32_t I2 = (imm >> 22) & 1;
	uint32_t J1 = !I1 ^ sign;
	uint32_t J2 = !I2 ^ sign;
	uint32_t imm10 = READ_BITS(imm, 21, 12);
	uint32_t imm11 = READ_BITS(imm, 11, 1);

	uint16_t *buf = (uint16_t *)addr;

	buf[0] = (buf[0] & 0b1111100000000000) | (sign << 10) | imm10;
	buf[1] = (buf[1] & 0b1101000000000000) | (J1 << 13) | (J2 << 11) | imm11;
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
void arch_elf_relocate(elf_rela_t *rel, uintptr_t opaddr, uintptr_t opval)
{
	elf_word reloc_type = ELF32_R_TYPE(rel->r_info);

	switch (reloc_type) {
	case R_ARM_ABS32:
		/* Update the absolute address of a load/store instruction */
		*((uint32_t *)opaddr) = (uint32_t)opval;
		break;
	case R_ARM_THM_CALL:
		/* FIXME: If weak undefined symbol, jump to next instruction */

		uint32_t sym_loc = opval - *((uintptr_t *)opaddr);
		bool is_thumb = sym_loc & 1;
		int32_t addend = read_thm_b_addend((uint8_t *)opaddr);

		/* offset = S + A - P*/
		int32_t offset = sym_loc + addend - opaddr;

		/* FIXME: for jumps greater than 16 MiB, BL/X should be rewritten
		 * to jump to linker-synthesized code that constructs 32-bit address
		 */
		if (is_thumb) {
			rewrite_thm_b_imm((uint8_t *)opaddr, offset);
			*(uint16_t *)(opaddr + 2) |= 0x1000; /* rewrite to BL */
		} else {
			rewrite_thm_b_imm((uint8_t *)opaddr, ALIGN_TO(offset, 4));
			*(uint16_t *)(opaddr + 2) &= ~0x1000; /* rewrite to BLX */
		}

		break;
	default:
		LOG_DBG("Unsupported ARM elf relocation type %d at address %lx",
			reloc_type, opaddr);
		break;
	}
}
