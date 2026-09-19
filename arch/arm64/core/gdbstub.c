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
#include <zephyr/cache.h>
#include <zephyr/debug/gdbstub.h>
#include <zephyr/sys/util.h>
#include <string.h>

/* Required by exception asm path: x19–x29 not in arch_esf */
struct arm64_gdb_esf_extra_regs gdb_esf_extra_regs;

static struct gdb_ctx ctx;
static bool stub_entry_brk;

/* Hex length of one 8-byte register in the g-packet */
#define REG64_HEX_LEN	16
#define REG32_HEX_LEN	8
#define VREG_HEX_LEN	32

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
	if (regno == GDB_AARCH64_CPSR_REGNO ||
	    regno == GDB_AARCH64_FPSR_REGNO ||
	    regno == GDB_AARCH64_FPCR_REGNO) {
		return 4;
	}
	if (regno >= GDB_AARCH64_V0_REGNO && regno < GDB_AARCH64_FPSR_REGNO) {
		return 16;
	}
	return 0;
}

static void fill_ctx_from_esf(struct arch_esf *esf)
{
	ctx.registers[GDB_X0] = esf->x0;
	ctx.registers[GDB_X1] = esf->x1;
	ctx.registers[GDB_X2] = esf->x2;
	ctx.registers[GDB_X3] = esf->x3;
	ctx.registers[GDB_X4] = esf->x4;
	ctx.registers[GDB_X5] = esf->x5;
	ctx.registers[GDB_X6] = esf->x6;
	ctx.registers[GDB_X7] = esf->x7;
	ctx.registers[GDB_X8] = esf->x8;
	ctx.registers[GDB_X9] = esf->x9;
	ctx.registers[GDB_X10] = esf->x10;
	ctx.registers[GDB_X11] = esf->x11;
	ctx.registers[GDB_X12] = esf->x12;
	ctx.registers[GDB_X13] = esf->x13;
	ctx.registers[GDB_X14] = esf->x14;
	ctx.registers[GDB_X15] = esf->x15;
	ctx.registers[GDB_X16] = esf->x16;
	ctx.registers[GDB_X17] = esf->x17;
	ctx.registers[GDB_X18] = esf->x18;
	ctx.registers[GDB_X19] = gdb_esf_extra_regs.x19;
	ctx.registers[GDB_X20] = gdb_esf_extra_regs.x20;
	ctx.registers[GDB_X21] = gdb_esf_extra_regs.x21;
	ctx.registers[GDB_X22] = gdb_esf_extra_regs.x22;
	ctx.registers[GDB_X23] = gdb_esf_extra_regs.x23;
	ctx.registers[GDB_X24] = gdb_esf_extra_regs.x24;
	ctx.registers[GDB_X25] = gdb_esf_extra_regs.x25;
	ctx.registers[GDB_X26] = gdb_esf_extra_regs.x26;
	ctx.registers[GDB_X27] = gdb_esf_extra_regs.x27;
	ctx.registers[GDB_X28] = gdb_esf_extra_regs.x28;
#ifdef CONFIG_FRAME_POINTER
	ctx.registers[GDB_X29] = esf->fp;
#else
	ctx.registers[GDB_X29] = gdb_esf_extra_regs.x29;
#endif
	ctx.registers[GDB_X30] = esf->lr;
	ctx.registers[GDB_PC] = esf->elr;
	ctx.registers[GDB_CPSR] = esf->spsr;

#ifdef CONFIG_ARM64_SAFE_EXCEPTION_STACK
	/*
	 * From EL0, SP_EL0 is saved in the ESF. From EL1, reconstruct the
	 * pre-exception SP as the ESF base + frame size.
	 */
	if ((esf->spsr & SPSR_MODE_MASK) == SPSR_MODE_EL0T) {
		ctx.registers[GDB_SP] = esf->sp;
	} else {
		ctx.registers[GDB_SP] = (uint64_t)esf + sizeof(struct arch_esf);
	}
#else
	ctx.registers[GDB_SP] = (uint64_t)esf + sizeof(struct arch_esf);
#endif
}

static void write_ctx_to_esf(struct arch_esf *esf)
{
	esf->x0 = ctx.registers[GDB_X0];
	esf->x1 = ctx.registers[GDB_X1];
	esf->x2 = ctx.registers[GDB_X2];
	esf->x3 = ctx.registers[GDB_X3];
	esf->x4 = ctx.registers[GDB_X4];
	esf->x5 = ctx.registers[GDB_X5];
	esf->x6 = ctx.registers[GDB_X6];
	esf->x7 = ctx.registers[GDB_X7];
	esf->x8 = ctx.registers[GDB_X8];
	esf->x9 = ctx.registers[GDB_X9];
	esf->x10 = ctx.registers[GDB_X10];
	esf->x11 = ctx.registers[GDB_X11];
	esf->x12 = ctx.registers[GDB_X12];
	esf->x13 = ctx.registers[GDB_X13];
	esf->x14 = ctx.registers[GDB_X14];
	esf->x15 = ctx.registers[GDB_X15];
	esf->x16 = ctx.registers[GDB_X16];
	esf->x17 = ctx.registers[GDB_X17];
	esf->x18 = ctx.registers[GDB_X18];
	esf->lr = ctx.registers[GDB_X30];
	esf->elr = ctx.registers[GDB_PC];
	esf->spsr = ctx.registers[GDB_CPSR];

#ifdef CONFIG_FRAME_POINTER
	esf->fp = ctx.registers[GDB_X29];
#endif
#ifdef CONFIG_ARM64_SAFE_EXCEPTION_STACK
	if ((esf->spsr & SPSR_MODE_MASK) == SPSR_MODE_EL0T) {
		esf->sp = ctx.registers[GDB_SP];
	}
#endif

	gdb_esf_extra_regs.x19 = ctx.registers[GDB_X19];
	gdb_esf_extra_regs.x20 = ctx.registers[GDB_X20];
	gdb_esf_extra_regs.x21 = ctx.registers[GDB_X21];
	gdb_esf_extra_regs.x22 = ctx.registers[GDB_X22];
	gdb_esf_extra_regs.x23 = ctx.registers[GDB_X23];
	gdb_esf_extra_regs.x24 = ctx.registers[GDB_X24];
	gdb_esf_extra_regs.x25 = ctx.registers[GDB_X25];
	gdb_esf_extra_regs.x26 = ctx.registers[GDB_X26];
	gdb_esf_extra_regs.x27 = ctx.registers[GDB_X27];
	gdb_esf_extra_regs.x28 = ctx.registers[GDB_X28];
	gdb_esf_extra_regs.x29 = ctx.registers[GDB_X29];
}

void z_gdb_entry(struct arch_esf *esf, unsigned int cause)
{
	bool entry_brk = stub_entry_brk;

	stub_entry_brk = false;
	ctx.exception = cause;
	fill_ctx_from_esf(esf);

	z_gdb_main_loop(&ctx);

	/*
	 * Only advance past the BRK used by arch_gdb_init(). GDB-managed
	 * software breakpoints must leave PC on the BRK so GDB can restore
	 * the original instruction.
	 */
	if (entry_brk) {
		ctx.registers[GDB_PC] += 4;
	}
	write_ctx_to_esf(esf);
}

/*
 * Hardware breakpoint helpers. ARM64 text is mapped RX (W^X), so GDB software
 * memory breakpoints cannot patch BRK into code. Implement Z0/Z1 via DBGBVR/CR.
 */
#define GDB_ARM64_MAX_HW_BP	16

static uint8_t hw_bp_used[GDB_ARM64_MAX_HW_BP];
static uintptr_t hw_bp_addr[GDB_ARM64_MAX_HW_BP];
static uint8_t hw_bp_count;

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

static void hw_bp_init_count(void)
{
	uint64_t dfr0 = read_id_aa64dfr0_el1();

	hw_bp_count = ((dfr0 >> ID_AA64DFR0_BRPS_SHIFT) & ID_AA64DFR0_BRPS_MASK) + 1U;
	if (hw_bp_count > GDB_ARM64_MAX_HW_BP) {
		hw_bp_count = GDB_ARM64_MAX_HW_BP;
	}
}

void arch_gdb_init(void)
{
	uint64_t mdscr;

	/* Clear OS Lock so monitor debug registers are accessible */
	__asm__ volatile("msr oslar_el1, xzr" ::: "memory");

	mdscr = read_mdscr_el1();
	mdscr |= MDSCR_EL1_MDE_BIT | MDSCR_EL1_KDE_BIT;
	mdscr &= ~MDSCR_EL1_SS_BIT;
	write_mdscr_el1(mdscr);

	enable_debug_exceptions();

	hw_bp_init_count();

	stub_entry_brk = true;
	__asm__ volatile("brk #0");
}

void arch_gdb_continue(void)
{
	uint64_t mdscr = read_mdscr_el1();

	mdscr &= ~MDSCR_EL1_SS_BIT;
	write_mdscr_el1(mdscr);

	ctx.registers[GDB_CPSR] &= ~SPSR_SS_BIT;
}

void arch_gdb_step(void)
{
	uint64_t mdscr = read_mdscr_el1();

	mdscr |= MDSCR_EL1_SS_BIT;
	write_mdscr_el1(mdscr);

	/* PSTATE.SS must be set on ERET for the step to take effect */
	ctx.registers[GDB_CPSR] |= SPSR_SS_BIT;
}

int arch_gdb_add_breakpoint(struct gdb_ctx *c, uint8_t type,
			    uintptr_t addr, uint32_t kind)
{
	ARG_UNUSED(c);
	ARG_UNUSED(kind);

	/* type 0: software BP request, type 1: hardware BP — both use HW */
	if (type > 1U) {
		return -2;
	}

	if (hw_bp_count == 0U) {
		hw_bp_init_count();
	}

	for (unsigned int i = 0; i < hw_bp_count; i++) {
		if (hw_bp_used[i] && hw_bp_addr[i] == addr) {
			return 0;
		}
	}

	for (unsigned int i = 0; i < hw_bp_count; i++) {
		if (!hw_bp_used[i]) {
			uint64_t bcr = DBGBCR_E_BIT | DBGBCR_PMC_EL1_EL0 | DBGBCR_BAS_A64;

			dbg_write_bvr(i, addr);
			dbg_write_bcr(i, bcr);
			hw_bp_addr[i] = addr;
			hw_bp_used[i] = 1U;
			return 0;
		}
	}

	return -1;
}

int arch_gdb_remove_breakpoint(struct gdb_ctx *c, uint8_t type,
			       uintptr_t addr, uint32_t kind)
{
	ARG_UNUSED(c);
	ARG_UNUSED(kind);

	if (type > 1U) {
		return -2;
	}

	if (hw_bp_count == 0U) {
		hw_bp_init_count();
	}

	for (unsigned int i = 0; i < hw_bp_count; i++) {
		if (hw_bp_used[i] && hw_bp_addr[i] == addr) {
			dbg_write_bcr(i, 0);
			dbg_write_bvr(i, 0);
			hw_bp_used[i] = 0U;
			hw_bp_addr[i] = 0;
			return 0;
		}
	}

	return -1;
}

size_t arch_gdb_reg_readall(struct gdb_ctx *c, uint8_t *buf, size_t buflen)
{
	size_t pos;
	uint32_t regno;

	if (buflen < GDB_AARCH64_G_PACKET_HEXLEN) {
		return 0;
	}

	/* Unavailable FP/SIMD (and any gap) reported as 'x' */
	memset(buf, 'x', GDB_AARCH64_G_PACKET_HEXLEN);

	for (regno = 0; regno <= GDB_AARCH64_CPSR_REGNO; regno++) {
		size_t bin_sz = regno_bin_size(regno);
		size_t hex_sz = bin_sz * 2;
		uint32_t idx = regno;
		size_t written;

		pos = hex_offset_for_regno(regno);
		if (pos == SIZE_MAX || (pos + hex_sz) > buflen) {
			return 0;
		}

		if (regno == GDB_AARCH64_CPSR_REGNO) {
			uint32_t cpsr = (uint32_t)c->registers[GDB_CPSR];

			written = gdb_bin2hex((const uint8_t *)&cpsr, bin_sz, buf + pos,
					      buflen - pos);
		} else {
			written = gdb_bin2hex((const uint8_t *)&c->registers[idx], bin_sz,
					      buf + pos, buflen - pos);
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

	if (hexlen < GDB_AARCH64_G_PACKET_HEXLEN) {
		return 0;
	}

	for (regno = 0; regno <= GDB_AARCH64_CPSR_REGNO; regno++) {
		size_t bin_sz = regno_bin_size(regno);
		size_t hex_sz = bin_sz * 2;
		size_t pos = hex_offset_for_regno(regno);

		if (hex[pos] == 'x') {
			continue;
		}

		if (regno == GDB_AARCH64_CPSR_REGNO) {
			uint32_t cpsr = 0;

			if (hex2bin(hex + pos, hex_sz, (uint8_t *)&cpsr, bin_sz) != bin_sz) {
				return 0;
			}
			c->registers[GDB_CPSR] = cpsr;
			ret += bin_sz;
		} else {
			if (hex2bin(hex + pos, hex_sz,
				    (uint8_t *)&c->registers[regno], bin_sz) != bin_sz) {
				return 0;
			}
			ret += bin_sz;
		}
	}

	return ret;
}

size_t arch_gdb_reg_readone(struct gdb_ctx *c, uint8_t *buf, size_t buflen, uint32_t regno)
{
	size_t bin_sz;
	size_t hex_sz;

	if (regno >= GDB_AARCH64_NUM_GDB_REGS) {
		return 0;
	}

	bin_sz = regno_bin_size(regno);
	hex_sz = bin_sz * 2;
	if (buflen < hex_sz) {
		return 0;
	}

	if (!regno_is_core(regno)) {
		memset(buf, 'x', hex_sz);
		return hex_sz;
	}

	if (regno == GDB_AARCH64_CPSR_REGNO) {
		uint32_t cpsr = (uint32_t)c->registers[GDB_CPSR];

		if (gdb_bin2hex((const uint8_t *)&cpsr, bin_sz, buf, buflen) != hex_sz) {
			return 0;
		}
	} else if (gdb_bin2hex((const uint8_t *)&c->registers[regno], bin_sz, buf,
			       buflen) != hex_sz) {
		return 0;
	}

	return hex_sz;
}

size_t arch_gdb_reg_writeone(struct gdb_ctx *c, uint8_t *hex, size_t hexlen, uint32_t regno)
{
	size_t bin_sz;

	if (!regno_is_core(regno)) {
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
		c->registers[GDB_CPSR] = cpsr;
		return bin_sz;
	}

	if (hex2bin(hex, hexlen, (uint8_t *)&c->registers[regno], bin_sz) != bin_sz) {
		return 0;
	}

	return bin_sz;
}

void arch_gdb_post_memory_write(uintptr_t addr, size_t len, uint8_t align)
{
	ARG_UNUSED(align);

	/*
	 * GDB may patch BRK into text. Ensure D-cache is written back and
	 * I-cache sees the new instructions.
	 */
	sys_cache_data_flush_range((void *)addr, len);
	sys_cache_instr_invd_range((void *)addr, len);
	__asm__ volatile("isb" ::: "memory");
}
