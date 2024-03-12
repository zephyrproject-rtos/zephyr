/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/llext/elf.h>
#include <zephyr/llext/llext.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(elf, CONFIG_LLEXT_LOG_LEVEL);

#define ARM_BL_BLX_UPPER_S_BIT BIT(10)
#define ARM_BL_BLX_ADDEND_OFFSET 0
#define ARM_BL_BLX_ADDEND_SIZE 11
#define ARM_BL_BLX_ADDEND_MASK 0x7FF
#define ARM_BL_BLX_HDR_MASK 0xF800
#define ARM_BL_BLX_LOWER_T1T2_BIT BIT(12)

static int32_t arm_bl_blx_decode_addend(uintptr_t opaddr)
{
	uint16_t upper = *((uint16_t *)opaddr);
	uint16_t lower = *(((uint16_t *)opaddr) + 1);

	int32_t addend = upper & ARM_BL_BLX_UPPER_S_BIT ? UINT32_MAX : 0;

	addend <<= ARM_BL_BLX_ADDEND_SIZE;
	addend |= upper & ARM_BL_BLX_ADDEND_MASK;
	addend <<= ARM_BL_BLX_ADDEND_SIZE;
	addend |= lower & ARM_BL_BLX_ADDEND_MASK;

	return lower & ARM_BL_BLX_LOWER_T1T2_BIT ? addend << 1 : addend << 2;
}

static void arm_bl_blx_encode_addend(uintptr_t opaddr, int32_t addend)
{
	uint16_t upper = *((uint16_t *)opaddr);
	uint16_t lower = *(((uint16_t *)opaddr) + 1);

	addend = upper & ARM_BL_BLX_UPPER_S_BIT ? addend >> 1 : addend >> 2;

	upper &= ARM_BL_BLX_HDR_MASK;
	lower &= ARM_BL_BLX_HDR_MASK;
	upper |= (addend >> ARM_BL_BLX_ADDEND_SIZE) & ARM_BL_BLX_ADDEND_MASK;
	lower |= addend & ARM_BL_BLX_ADDEND_MASK;

	*((uint16_t *)opaddr) = upper;
	*(((uint16_t *)opaddr) + 1) = lower;
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
		/* Add the addend stored at opaddr to opval */
		opval += *((uint32_t *)opaddr);

		/* Update the absolute address of a load/store instruction */
		*((uint32_t *)opaddr) = (uint32_t)opval;
		break;
	case R_ARM_THM_CALL:
		/* Decode the initial addend */
		int32_t addend = arm_bl_blx_decode_addend(opaddr);

		/* Calculate and add the branch offset (addend) */
		addend += ((int32_t)opval) - ((int32_t)opaddr);

		/* Encode the addend */
		arm_bl_blx_encode_addend(opaddr, addend);
		break;
	default:
		LOG_DBG("Unsupported ARM elf relocation type %d at address %lx",
			reloc_type, opaddr);
		break;
	}
}
