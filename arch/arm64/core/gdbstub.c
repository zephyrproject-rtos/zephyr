/*
 * Copyright (c) 2026 KylinSoft Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <kernel_internal.h>
#include <zephyr/arch/arm64/gdbstub.h>
#include <zephyr/arch/arm64/cpu.h>
#include <zephyr/arch/arm64/lib_helpers.h>
#include <zephyr/debug/gdbstub.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/util.h>
#include <string.h>

const struct gdb_mem_region gdb_mem_region_array[] = {
	{
		.start = (uintptr_t)__text_region_start,
		.end = (uintptr_t)__text_region_end,
		.attributes = GDB_MEM_REGION_RO,
		.alignment = 0,
	},
	{
		.start = (uintptr_t)__rodata_region_start,
		.end = (uintptr_t)__rodata_region_end,
		.attributes = GDB_MEM_REGION_RO,
		.alignment = 0,
	},
	{
		.start = (uintptr_t)_image_ram_start,
		.end = (uintptr_t)_image_ram_end,
		.attributes = GDB_MEM_REGION_RW,
		.alignment = 0,
	},
#if defined(CONFIG_NOCACHE_MEMORY)
	{
		.start = (uintptr_t)_nocache_ram_start,
		.end = (uintptr_t)_nocache_ram_end,
		.attributes = GDB_MEM_REGION_RW,
		.alignment = 0,
	},
#endif
};

const size_t gdb_mem_num_regions = ARRAY_SIZE(gdb_mem_region_array);

static struct gdb_ctx ctx;

/* ESR_EL1.EC: BRK instruction in AArch64 state */
#define ESR_EC_BRK_AARCH64 0x3cU

/* Hex length of one 8-byte register in the g-packet */
#define REG64_HEX_LEN 16
#define REG32_HEX_LEN 8
#define VREG_HEX_LEN  32

static size_t hex_offset_for_regno(uint32_t regno)
{
	if (regno <= GDB_AARCH64_PC_REGNO) {
		return (size_t)regno * REG64_HEX_LEN;
	}
	if (regno == GDB_AARCH64_CPSR_REGNO) {
		return (GDB_AARCH64_PC_REGNO + 1) * REG64_HEX_LEN;
	}
	if (regno < GDB_AARCH64_FPSR_REGNO) {
		/* V0..Vn after CPSR (4 bytes / 8 hex) */
		size_t base = (GDB_AARCH64_PC_REGNO + 1) * REG64_HEX_LEN + REG32_HEX_LEN;

		return base + (size_t)(regno - GDB_AARCH64_V0_REGNO) * VREG_HEX_LEN;
	}
	if (regno == GDB_AARCH64_FPSR_REGNO) {
		size_t base = (GDB_AARCH64_PC_REGNO + 1) * REG64_HEX_LEN + REG32_HEX_LEN;

		return base + 32U * VREG_HEX_LEN;
	}
	if (regno == GDB_AARCH64_FPCR_REGNO) {
		size_t base = (GDB_AARCH64_PC_REGNO + 1) * REG64_HEX_LEN + REG32_HEX_LEN;

		return base + 32U * VREG_HEX_LEN + REG32_HEX_LEN;
	}
	return SIZE_MAX;
}

static bool regno_is_core(uint32_t regno)
{
	return regno <= GDB_AARCH64_CPSR_REGNO;
}

static size_t regno_bin_size(uint32_t regno)
{
	if (regno <= GDB_AARCH64_PC_REGNO) {
		return 8;
	}
	if (regno == GDB_AARCH64_CPSR_REGNO || regno == GDB_AARCH64_FPSR_REGNO ||
	    regno == GDB_AARCH64_FPCR_REGNO) {
		return 4;
	}
	if (regno >= GDB_AARCH64_V0_REGNO && regno < GDB_AARCH64_FPSR_REGNO) {
		return 16;
	}
	return 0;
}

#ifdef CONFIG_ARM64_SAFE_EXCEPTION_STACK
/*
 * SP is writable via gdbstub only when esf->sp (SP_EL0) is restored on eret:
 * EL0t with CONFIG_ARM64_SAFE_EXCEPTION_STACK. EL1 contexts expose a
 * reconstructed SP_EL1 for read only; z_arm64_exit_exc always uses add sp, SIZEOF.
 */
static bool gdb_sp_write_supported(uint64_t spsr)
{
	const bool is_el0t = ((spsr & SPSR_MODE_MASK) == SPSR_MODE_EL0T);

	return is_el0t;
}
#endif

static uint64_t gdb_get_sp(const struct arch_esf *esf)
{
	/*
	 * From EL0, report SP_EL0. With CONFIG_ARM64_SAFE_EXCEPTION_STACK the
	 * vector entry saves it in esf->sp before switching to the safe stack;
	 * without that option SP_EL0 is left untouched, so read it live.
	 * From EL1, reconstruct the pre-exception SP as ESF base + frame size.
	 */
	if ((esf->spsr & SPSR_MODE_MASK) == SPSR_MODE_EL0T) {
#ifdef CONFIG_ARM64_SAFE_EXCEPTION_STACK
		return esf->sp;
#else
		return read_sp_el0();
#endif
	}

	return (uint64_t)(uintptr_t)esf + sizeof(struct arch_esf);
}

static uint64_t *esf_reg_slot(struct arch_esf *esf, uint32_t regno)
{
	switch (regno) {
	case 0:
		return &esf->x0;
	case 1:
		return &esf->x1;
	case 2:
		return &esf->x2;
	case 3:
		return &esf->x3;
	case 4:
		return &esf->x4;
	case 5:
		return &esf->x5;
	case 6:
		return &esf->x6;
	case 7:
		return &esf->x7;
	case 8:
		return &esf->x8;
	case 9:
		return &esf->x9;
	case 10:
		return &esf->x10;
	case 11:
		return &esf->x11;
	case 12:
		return &esf->x12;
	case 13:
		return &esf->x13;
	case 14:
		return &esf->x14;
	case 15:
		return &esf->x15;
	case 16:
		return &esf->x16;
	case 17:
		return &esf->x17;
	case 18:
		return &esf->x18;
#ifdef CONFIG_EXTRA_EXCEPTION_INFO
	case 19:
		return &esf->x19;
	case 20:
		return &esf->x20;
	case 21:
		return &esf->x21;
	case 22:
		return &esf->x22;
	case 23:
		return &esf->x23;
	case 24:
		return &esf->x24;
	case 25:
		return &esf->x25;
	case 26:
		return &esf->x26;
	case 27:
		return &esf->x27;
	case 28:
		return &esf->x28;
#endif
#ifdef CONFIG_FRAME_POINTER
	case GDB_AARCH64_FP_REGNO:
		return &esf->fp;
#endif
	case GDB_AARCH64_LR_REGNO:
		return &esf->lr;
	case GDB_AARCH64_PC_REGNO:
		return &esf->elr;
	default:
		return NULL;
	}
}

static bool read_esf_reg(struct arch_esf *esf, uint32_t regno, uint64_t *val)
{
	uint64_t *slot;

	switch (regno) {
	case GDB_AARCH64_SP_REGNO:
		*val = gdb_get_sp(esf);
		return true;
	case GDB_AARCH64_CPSR_REGNO:
		*val = (uint32_t)esf->spsr;
		return true;
	default:
		slot = esf_reg_slot(esf, regno);
		if (slot == NULL) {
			return false;
		}
		*val = *slot;
		return true;
	}
}

static bool write_esf_reg(struct arch_esf *esf, uint32_t regno, uint64_t val)
{
	uint64_t *slot;

	switch (regno) {
	case GDB_AARCH64_SP_REGNO:
#ifdef CONFIG_ARM64_SAFE_EXCEPTION_STACK
		if (gdb_sp_write_supported(esf->spsr)) {
			esf->sp = val;
			return true;
		}
#endif
		return false;
	case GDB_AARCH64_CPSR_REGNO:
		esf->spsr = (uint32_t)val;
		return true;
	default:
		slot = esf_reg_slot(esf, regno);
		if (slot == NULL) {
			return false;
		}
		*slot = val;
		return true;
	}
}

void z_gdb_entry(struct arch_esf *esf, unsigned int cause)
{
	uint64_t entry_elr = esf->elr;
	unsigned int ec = (unsigned int)GET_ESR_EC(read_esr_el1());

	ctx.exception = cause;
	ctx.esf = esf;

	z_gdb_main_loop(&ctx);

	/*
	 * A BRK exception leaves ELR pointing at the BRK. Step past it when
	 * GDB left PC unchanged (covers arch_gdb_init() and compiled-in BRK).
	 * Hardware breakpoints (Z0/Z1) and software step use other ECs and
	 * must re-execute ELR. If patched-BRK software breakpoints are added
	 * later, check on exit that entry_elr still holds a BRK before
	 * advancing so restored instructions are not skipped.
	 */
	if ((ec == ESR_EC_BRK_AARCH64) && (esf->elr == entry_elr)) {
		esf->elr += 4U;
	}

	ctx.esf = NULL;
}

/*
 * Hardware breakpoint helpers. ARM64 text is mapped RX (W^X), so GDB software
 * memory breakpoints cannot patch BRK into code. Implement Z0/Z1 via DBGBVR/CR.
 */
#define GDB_ARM64_MAX_HW_BP 16

/* Return values defined by the arch_gdb_{add,remove}_breakpoint() API */
#define GDB_BP_ERROR         (-1)
#define GDB_BP_NOT_SUPPORTED (-2)

static uint64_t dbg_read_bvr(unsigned int i)
{
	switch (i) {
	case 0:
		return read_sysreg(dbgbvr0_el1);
	case 1:
		return read_sysreg(dbgbvr1_el1);
	case 2:
		return read_sysreg(dbgbvr2_el1);
	case 3:
		return read_sysreg(dbgbvr3_el1);
	case 4:
		return read_sysreg(dbgbvr4_el1);
	case 5:
		return read_sysreg(dbgbvr5_el1);
	case 6:
		return read_sysreg(dbgbvr6_el1);
	case 7:
		return read_sysreg(dbgbvr7_el1);
	case 8:
		return read_sysreg(dbgbvr8_el1);
	case 9:
		return read_sysreg(dbgbvr9_el1);
	case 10:
		return read_sysreg(dbgbvr10_el1);
	case 11:
		return read_sysreg(dbgbvr11_el1);
	case 12:
		return read_sysreg(dbgbvr12_el1);
	case 13:
		return read_sysreg(dbgbvr13_el1);
	case 14:
		return read_sysreg(dbgbvr14_el1);
	case 15:
		return read_sysreg(dbgbvr15_el1);
	default:
		return 0;
	}
}

static uint64_t dbg_read_bcr(unsigned int i)
{
	switch (i) {
	case 0:
		return read_sysreg(dbgbcr0_el1);
	case 1:
		return read_sysreg(dbgbcr1_el1);
	case 2:
		return read_sysreg(dbgbcr2_el1);
	case 3:
		return read_sysreg(dbgbcr3_el1);
	case 4:
		return read_sysreg(dbgbcr4_el1);
	case 5:
		return read_sysreg(dbgbcr5_el1);
	case 6:
		return read_sysreg(dbgbcr6_el1);
	case 7:
		return read_sysreg(dbgbcr7_el1);
	case 8:
		return read_sysreg(dbgbcr8_el1);
	case 9:
		return read_sysreg(dbgbcr9_el1);
	case 10:
		return read_sysreg(dbgbcr10_el1);
	case 11:
		return read_sysreg(dbgbcr11_el1);
	case 12:
		return read_sysreg(dbgbcr12_el1);
	case 13:
		return read_sysreg(dbgbcr13_el1);
	case 14:
		return read_sysreg(dbgbcr14_el1);
	case 15:
		return read_sysreg(dbgbcr15_el1);
	default:
		return 0;
	}
}

static void dbg_write_bvr(unsigned int i, uint64_t val)
{
	switch (i) {
	case 0:
		write_sysreg(val, dbgbvr0_el1);
		break;
	case 1:
		write_sysreg(val, dbgbvr1_el1);
		break;
	case 2:
		write_sysreg(val, dbgbvr2_el1);
		break;
	case 3:
		write_sysreg(val, dbgbvr3_el1);
		break;
	case 4:
		write_sysreg(val, dbgbvr4_el1);
		break;
	case 5:
		write_sysreg(val, dbgbvr5_el1);
		break;
	case 6:
		write_sysreg(val, dbgbvr6_el1);
		break;
	case 7:
		write_sysreg(val, dbgbvr7_el1);
		break;
	case 8:
		write_sysreg(val, dbgbvr8_el1);
		break;
	case 9:
		write_sysreg(val, dbgbvr9_el1);
		break;
	case 10:
		write_sysreg(val, dbgbvr10_el1);
		break;
	case 11:
		write_sysreg(val, dbgbvr11_el1);
		break;
	case 12:
		write_sysreg(val, dbgbvr12_el1);
		break;
	case 13:
		write_sysreg(val, dbgbvr13_el1);
		break;
	case 14:
		write_sysreg(val, dbgbvr14_el1);
		break;
	case 15:
		write_sysreg(val, dbgbvr15_el1);
		break;
	default:
		break;
	}
}

static void dbg_write_bcr(unsigned int i, uint64_t val)
{
	switch (i) {
	case 0:
		write_sysreg(val, dbgbcr0_el1);
		break;
	case 1:
		write_sysreg(val, dbgbcr1_el1);
		break;
	case 2:
		write_sysreg(val, dbgbcr2_el1);
		break;
	case 3:
		write_sysreg(val, dbgbcr3_el1);
		break;
	case 4:
		write_sysreg(val, dbgbcr4_el1);
		break;
	case 5:
		write_sysreg(val, dbgbcr5_el1);
		break;
	case 6:
		write_sysreg(val, dbgbcr6_el1);
		break;
	case 7:
		write_sysreg(val, dbgbcr7_el1);
		break;
	case 8:
		write_sysreg(val, dbgbcr8_el1);
		break;
	case 9:
		write_sysreg(val, dbgbcr9_el1);
		break;
	case 10:
		write_sysreg(val, dbgbcr10_el1);
		break;
	case 11:
		write_sysreg(val, dbgbcr11_el1);
		break;
	case 12:
		write_sysreg(val, dbgbcr12_el1);
		break;
	case 13:
		write_sysreg(val, dbgbcr13_el1);
		break;
	case 14:
		write_sysreg(val, dbgbcr14_el1);
		break;
	case 15:
		write_sysreg(val, dbgbcr15_el1);
		break;
	default:
		break;
	}
}

static unsigned int hw_bp_count(void)
{
	uint64_t dfr0 = read_id_aa64dfr0_el1();
	unsigned int count;

	count = (unsigned int)(((dfr0 >> ID_AA64DFR0_BRPS_SHIFT) & ID_AA64DFR0_BRPS_MASK) + 1U);

	return MIN(count, GDB_ARM64_MAX_HW_BP);
}

/*
 * The stub owns all breakpoint register pairs; a pair whose DBGBCRn_EL1.E bit
 * is clear is free. Return the slot programmed with @addr, or @count if none.
 */
static unsigned int hw_bp_find(uintptr_t addr, unsigned int count, unsigned int *free_slot)
{
	for (unsigned int i = 0; i < count; i++) {
		if ((dbg_read_bcr(i) & DBGBCR_E_BIT) == 0U) {
			if ((free_slot != NULL) && (*free_slot == count)) {
				*free_slot = i;
			}
			continue;
		}

		if (dbg_read_bvr(i) == addr) {
			return i;
		}
	}

	return count;
}

void arch_gdb_init(void)
{
	uint64_t mdscr;
	unsigned int count = hw_bp_count();

	/* Clear OS Lock so monitor debug registers are accessible */
	zero_sysreg(oslar_el1);

	mdscr = read_mdscr_el1();
	mdscr |= MDSCR_EL1_MDE_BIT | MDSCR_EL1_KDE_BIT;
	mdscr &= ~MDSCR_EL1_SS_BIT;
	write_mdscr_el1(mdscr);

	/* Breakpoint registers reset to UNKNOWN values; start with all slots free */
	for (unsigned int i = 0; i < count; i++) {
		dbg_write_bcr(i, 0);
		dbg_write_bvr(i, 0);
	}
	barrier_isync_fence_full();

	enable_debug_exceptions();

	__asm__ volatile("brk #0");
}

void arch_gdb_continue(void)
{
	uint64_t mdscr = read_mdscr_el1();

	mdscr &= ~MDSCR_EL1_SS_BIT;
	write_mdscr_el1(mdscr);

	if (ctx.esf != NULL) {
		ctx.esf->spsr &= ~SPSR_SS_BIT;
	}
}

void arch_gdb_step(void)
{
	uint64_t mdscr = read_mdscr_el1();

	mdscr |= MDSCR_EL1_SS_BIT;
	write_mdscr_el1(mdscr);

	/* PSTATE.SS must be set on ERET for the step to take effect */
	if (ctx.esf != NULL) {
		ctx.esf->spsr |= SPSR_SS_BIT;
	}
}

int arch_gdb_add_breakpoint(struct gdb_ctx *c, uint8_t type, uintptr_t addr, uint32_t kind)
{
	unsigned int count;
	unsigned int free_slot;

	ARG_UNUSED(c);
	ARG_UNUSED(kind);

	/* type 0: software BP request, type 1: hardware BP — both use HW */
	if (type > 1U) {
		return GDB_BP_NOT_SUPPORTED;
	}

	/* DBGBVRn_EL1[1:0] are RES0: A64 instructions are word aligned */
	if ((addr & 3U) != 0U) {
		return GDB_BP_ERROR;
	}

	count = hw_bp_count();
	free_slot = count;

	if (hw_bp_find(addr, count, &free_slot) < count) {
		return 0;
	}

	if (free_slot == count) {
		return GDB_BP_ERROR;
	}

	dbg_write_bvr(free_slot, addr);
	dbg_write_bcr(free_slot, DBGBCR_E_BIT | DBGBCR_PMC_EL1_EL0 | DBGBCR_BAS_A64);

	return 0;
}

int arch_gdb_remove_breakpoint(struct gdb_ctx *c, uint8_t type, uintptr_t addr, uint32_t kind)
{
	unsigned int count;
	unsigned int slot;

	ARG_UNUSED(c);
	ARG_UNUSED(kind);

	if (type > 1U) {
		return GDB_BP_NOT_SUPPORTED;
	}

	count = hw_bp_count();
	slot = hw_bp_find(addr, count, NULL);
	if (slot == count) {
		return GDB_BP_ERROR;
	}

	dbg_write_bcr(slot, 0);
	dbg_write_bvr(slot, 0);

	return 0;
}

size_t arch_gdb_reg_readall(struct gdb_ctx *c, uint8_t *buf, size_t buflen)
{
	size_t pos;
	uint32_t regno;

	if ((c == NULL) || (c->esf == NULL) || (buflen < GDB_AARCH64_G_PACKET_HEXLEN)) {
		return 0;
	}

	/* Unavailable registers (FP/SIMD, or x29 without FRAME_POINTER) reported as 'x' */
	memset(buf, 'x', GDB_AARCH64_G_PACKET_HEXLEN);

	for (regno = 0; regno <= GDB_AARCH64_CPSR_REGNO; regno++) {
		size_t bin_sz = regno_bin_size(regno);
		size_t hex_sz = bin_sz * 2;
		uint64_t val = 0;
		size_t written;

		if (!read_esf_reg(c->esf, regno, &val)) {
			continue;
		}

		pos = hex_offset_for_regno(regno);
		if (pos == SIZE_MAX || (pos + hex_sz) > buflen) {
			return 0;
		}

		if (regno == GDB_AARCH64_CPSR_REGNO) {
			uint32_t cpsr = (uint32_t)val;

			written = gdb_bin2hex((const uint8_t *)&cpsr, bin_sz, buf + pos,
					      buflen - pos);
		} else {
			written =
				gdb_bin2hex((const uint8_t *)&val, bin_sz, buf + pos, buflen - pos);
		}
		if (written != hex_sz) {
			return 0;
		}
	}

	return GDB_AARCH64_G_PACKET_HEXLEN;
}

size_t arch_gdb_reg_writeall(struct gdb_ctx *c, uint8_t *hex, size_t hexlen)
{
	size_t ret = 0;
	uint32_t regno;
	struct arch_esf temp;
	uint64_t cur_sp;
	uint64_t new_sp;
	bool sp_written = false;

	if ((c == NULL) || (c->esf == NULL) || (hexlen < GDB_AARCH64_G_PACKET_HEXLEN)) {
		return 0;
	}

	temp = *c->esf;
	cur_sp = gdb_get_sp(c->esf);
	new_sp = cur_sp;

	for (regno = 0; regno <= GDB_AARCH64_CPSR_REGNO; regno++) {
		size_t bin_sz = regno_bin_size(regno);
		size_t hex_sz = bin_sz * 2;
		size_t pos = hex_offset_for_regno(regno);
		uint64_t val = 0;

		if (pos == SIZE_MAX) {
			return 0;
		}

		if (hex[pos] == 'x') {
			continue;
		}

		if (regno == GDB_AARCH64_CPSR_REGNO) {
			uint32_t cpsr = 0;

			if (hex2bin(hex + pos, hex_sz, (uint8_t *)&cpsr, bin_sz) != bin_sz) {
				return 0;
			}
			val = cpsr;
		} else {
			if (hex2bin(hex + pos, hex_sz, (uint8_t *)&val, bin_sz) != bin_sz) {
				return 0;
			}
		}

		if (regno == GDB_AARCH64_SP_REGNO) {
			new_sp = val;
			sp_written = true;
		} else {
			if (!write_esf_reg(&temp, regno, val)) {
				return 0;
			}
		}

		ret += bin_sz;
	}

	/*
	 * G packets always include SP. Fail the whole packet only when the
	 * value actually changes and exception return will not restore it, so
	 * an unrelated register write is not rejected or partially applied.
	 */
	if (sp_written && (new_sp != cur_sp)) {
		if (!write_esf_reg(&temp, GDB_AARCH64_SP_REGNO, new_sp)) {
			return 0;
		}
	}

	*c->esf = temp;

	return ret;
}

size_t arch_gdb_reg_readone(struct gdb_ctx *c, uint8_t *buf, size_t buflen, uint32_t regno)
{
	size_t bin_sz;
	size_t hex_sz;
	uint64_t val = 0;

	if ((c == NULL) || (c->esf == NULL) || (regno >= GDB_AARCH64_NUM_GDB_REGS)) {
		return 0;
	}

	bin_sz = regno_bin_size(regno);
	hex_sz = bin_sz * 2;
	if (buflen < hex_sz) {
		return 0;
	}

	if (!regno_is_core(regno) || !read_esf_reg(c->esf, regno, &val)) {
		memset(buf, 'x', hex_sz);
		return hex_sz;
	}

	if (regno == GDB_AARCH64_CPSR_REGNO) {
		uint32_t cpsr = (uint32_t)val;

		if (gdb_bin2hex((const uint8_t *)&cpsr, bin_sz, buf, buflen) != hex_sz) {
			return 0;
		}
	} else {
		if (gdb_bin2hex((const uint8_t *)&val, bin_sz, buf, buflen) != hex_sz) {
			return 0;
		}
	}

	return hex_sz;
}

size_t arch_gdb_reg_writeone(struct gdb_ctx *c, uint8_t *hex, size_t hexlen, uint32_t regno)
{
	size_t bin_sz;
	uint64_t val = 0;

	if ((c == NULL) || (c->esf == NULL) || !regno_is_core(regno)) {
		return 0;
	}

	bin_sz = regno_bin_size(regno);
	if (hexlen != bin_sz * 2) {
		return 0;
	}

	if (regno == GDB_AARCH64_CPSR_REGNO) {
		uint32_t cpsr = 0;

		if (hex2bin(hex, hexlen, (uint8_t *)&cpsr, bin_sz) != bin_sz) {
			return 0;
		}
		val = cpsr;
	} else {
		if (hex2bin(hex, hexlen, (uint8_t *)&val, bin_sz) != bin_sz) {
			return 0;
		}
	}

	if (regno == GDB_AARCH64_SP_REGNO) {
		uint64_t cur_sp = gdb_get_sp(c->esf);

		if (val == cur_sp) {
			return bin_sz;
		}
	}

	if (!write_esf_reg(c->esf, regno, val)) {
		return 0;
	}

	return bin_sz;
}
