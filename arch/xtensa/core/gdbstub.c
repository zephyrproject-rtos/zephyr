/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <kernel_internal.h>
#include <zephyr/toolchain.h>
#include <zephyr/debug/gdbstub.h>

#include <xtensa-asm2-context.h>
#include <xtensa/corebits.h>

static bool not_first_break;

extern struct gdb_ctx xtensa_gdb_ctx;

/*
 * Special register number (from specreg.h).
 *
 * These should be the same across different Xtensa SoCs.
 */
enum {
	LBEG = 0,
	LEND = 1,
	LCOUNT = 2,
	SAR = 3,
	SCOMPARE1 = 12,
	WINDOWBASE = 72,
	WINDOWSTART = 73,
	IBREAKENABLE = 96,
	MEMCTL = 97,
	ATOMCTL = 99,
	IBREAKA0 = 128,
	IBREAKA1 = 129,
	CONFIGID0 = 176,
	EPC_1 = 177,
	EPC_2 = 178,
	EPC_3 = 179,
	EPC_4 = 180,
	EPC_5 = 181,
	EPC_6 = 182,
	EPC_7 = 183,
	DEPC = 192,
	EPS_2 = 194,
	EPS_3 = 195,
	EPS_4 = 196,
	EPS_5 = 197,
	EPS_6 = 198,
	EPS_7 = 199,
	CONFIGID1 = 208,
	EXCSAVE_1 = 209,
	EXCSAVE_2 = 210,
	EXCSAVE_3 = 211,
	EXCSAVE_4 = 212,
	EXCSAVE_5 = 213,
	EXCSAVE_6 = 214,
	EXCSAVE_7 = 215,
	CPENABLE = 224,
	INTERRUPT = 226,
	INTENABLE = 228,
	PS = 230,
	THREADPTR = 231,
	EXCCAUSE = 232,
	DEBUGCAUSE = 233,
	CCOUNT = 234,
	PRID = 235,
	ICOUNT = 236,
	ICOUNTLEVEL = 237,
	EXCVADDR = 238,
	CCOMPARE_0 = 240,
	CCOMPARE_1 = 241,
	CCOMPARE_2 = 242,
	MISC_REG_0 = 244,
	MISC_REG_1 = 245,
	MISC_REG_2 = 246,
	MISC_REG_3 = 247,
};

#define get_one_sreg(regnum_p) ({ \
	unsigned int retval; \
	__asm__ volatile( \
		"rsr %[retval], %[regnum]\n\t" \
		: [retval] "=r" (retval) \
		: [regnum] "i" (regnum_p)); \
	retval; \
	})

#define set_one_sreg(regnum_p, regval) { \
	__asm__ volatile( \
		"wsr %[val], %[regnum]\n\t" \
		:: \
		[val] "r" (regval), \
		[regnum] "i" (regnum_p)); \
	}

/**
 * Read one special register.
 *
 * @param ctx GDB context
 * @param reg Register descriptor
 */
static void read_sreg(struct gdb_ctx *ctx, struct xtensa_register *reg)
{
	uint8_t regno;
	uint32_t val;
	bool has_val = true;

	if (!gdb_xtensa_is_special_reg(reg)) {
		return;
	}

	/*
	 * Special registers have 0x300 added to the register number
	 * in the register descriptor. So need to extract the actual
	 * special register number recognized by architecture,
	 * which is 0-255.
	 */
	regno = reg->regno & 0xFF;

	/*
	 * Each special register has to be done separately
	 * as the register number in RSR/WSR needs to be
	 * hard-coded at compile time.
	 */
	switch (regno) {
	case SAR:
		val = get_one_sreg(SAR);
		break;
	case PS:
		val = get_one_sreg(PS);
		break;
	case MEMCTL:
		val = get_one_sreg(MEMCTL);
		break;
	case ATOMCTL:
		val = get_one_sreg(ATOMCTL);
		break;
	case CONFIGID0:
		val = get_one_sreg(CONFIGID0);
		break;
	case CONFIGID1:
		val = get_one_sreg(CONFIGID1);
		break;
	case DEBUGCAUSE:
		val = get_one_sreg(DEBUGCAUSE);
		break;
	case EXCCAUSE:
		val = get_one_sreg(EXCCAUSE);
		break;
	case DEPC:
		val = get_one_sreg(DEPC);
		break;
	case EPC_1:
		val = get_one_sreg(EPC_1);
		break;
	case EXCSAVE_1:
		val = get_one_sreg(EXCSAVE_1);
		break;
	case EXCVADDR:
		val = get_one_sreg(EXCVADDR);
		break;
#if XCHAL_HAVE_LOOPS
	case LBEG:
		val = get_one_sreg(LBEG);
		break;
	case LEND:
		val = get_one_sreg(LEND);
		break;
	case LCOUNT:
		val = get_one_sreg(LCOUNT);
		break;
#endif
#if XCHAL_HAVE_S32C1I
	case SCOMPARE1:
		val = get_one_sreg(SCOMPARE1);
		break;
#endif
#if XCHAL_HAVE_WINDOWED
	case WINDOWBASE:
		val = get_one_sreg(WINDOWBASE);
		break;
	case WINDOWSTART:
		val = get_one_sreg(WINDOWSTART);
		break;
#endif
#if XCHAL_NUM_INTLEVELS > 0
	case EPS_2:
		val = get_one_sreg(EPS_2);
		break;
	case EPC_2:
		val = get_one_sreg(EPC_2);
		break;
	case EXCSAVE_2:
		val = get_one_sreg(EXCSAVE_2);
		break;
#endif
#if XCHAL_NUM_INTLEVELS > 1
	case EPS_3:
		val = get_one_sreg(EPS_3);
		break;
	case EPC_3:
		val = get_one_sreg(EPC_3);
		break;
	case EXCSAVE_3:
		val = get_one_sreg(EXCSAVE_3);
		break;
#endif
#if XCHAL_NUM_INTLEVELS > 2
	case EPC_4:
		val = get_one_sreg(EPC_4);
		break;
	case EPS_4:
		val = get_one_sreg(EPS_4);
		break;
	case EXCSAVE_4:
		val = get_one_sreg(EXCSAVE_4);
		break;
#endif
#if XCHAL_NUM_INTLEVELS > 3
	case EPC_5:
		val = get_one_sreg(EPC_5);
		break;
	case EPS_5:
		val = get_one_sreg(EPS_5);
		break;
	case EXCSAVE_5:
		val = get_one_sreg(EXCSAVE_5);
		break;
#endif
#if XCHAL_NUM_INTLEVELS > 4
	case EPC_6:
		val = get_one_sreg(EPC_6);
		break;
	case EPS_6:
		val = get_one_sreg(EPS_6);
		break;
	case EXCSAVE_6:
		val = get_one_sreg(EXCSAVE_6);
		break;
#endif
#if XCHAL_NUM_INTLEVELS > 5
	case EPC_7:
		val = get_one_sreg(EPC_7);
		break;
	case EPS_7:
		val = get_one_sreg(EPS_7);
		break;
	case EXCSAVE_7:
		val = get_one_sreg(EXCSAVE_7);
		break;
#endif
#if XCHAL_HAVE_CP
	case CPENABLE:
		val = get_one_sreg(CPENABLE);
		break;
#endif
#if XCHAL_HAVE_INTERRUPTS
	case INTERRUPT:
		val = get_one_sreg(INTERRUPT);
		break;
	case INTENABLE:
		val = get_one_sreg(INTENABLE);
		break;
#endif
#if XCHAL_HAVE_THREADPTR
	case THREADPTR:
		val = get_one_sreg(THREADPTR);
		break;
#endif
#if XCHAL_HAVE_CCOUNT
	case CCOUNT:
		val = get_one_sreg(CCOUNT);
		break;
#endif
#if XCHAL_HAVE_PRID
	case PRID:
		val = get_one_sreg(PRID);
		break;
#endif
#if XCHAL_NUM_TIMERS > 0
	case CCOMPARE_0:
		val = get_one_sreg(CCOMPARE_0);
		break;
#endif
#if XCHAL_NUM_TIMERS > 1
	case CCOMPARE_1:
		val = get_one_sreg(CCOMPARE_1);
		break;
#endif
#if XCHAL_NUM_TIMERS > 2
	case CCOMPARE_2:
		val = get_one_sreg(CCOMPARE_2);
		break;
#endif
#if XCHAL_NUM_MISC_REGS > 0
	case MISC_REG_0:
		val = get_one_sreg(MISC_REG_0);
		break;
#endif
#if XCHAL_NUM_MISC_REGS > 1
	case MISC_REG_1:
		val = get_one_sreg(MISC_REG_1);
		break;
#endif
#if XCHAL_NUM_MISC_REGS > 2
	case MISC_REG_2:
		val = get_one_sreg(MISC_REG_2);
		break;
#endif
#if XCHAL_NUM_MISC_REGS > 3
	case MISC_REG_3:
		val = get_one_sreg(MISC_REG_3);
		break;
#endif
	default:
		has_val = false;
		break;
	}

	if (has_val) {
		reg->val = val;
		reg->seqno = ctx->seqno;
	}
}

/**
 * Translate exception into GDB exception reason.
 *
 * @param reason Reason for exception
 */
static unsigned int get_gdb_exception_reason(unsigned int reason)
{
	unsigned int exception;

	switch (reason) {
	case EXCCAUSE_ILLEGAL:
		/* illegal instruction */
		exception = GDB_EXCEPTION_INVALID_INSTRUCTION;
		break;
	case EXCCAUSE_INSTR_ERROR:
		/* instr fetch error */
		exception = GDB_EXCEPTION_MEMORY_FAULT;
		break;
	case EXCCAUSE_LOAD_STORE_ERROR:
		/* load/store error */
		exception = GDB_EXCEPTION_MEMORY_FAULT;
		break;
	case EXCCAUSE_DIVIDE_BY_ZERO:
		/* divide by zero */
		exception = GDB_EXCEPTION_DIVIDE_ERROR;
		break;
	case EXCCAUSE_UNALIGNED:
		/* load/store alignment */
		exception = GDB_EXCEPTION_MEMORY_FAULT;
		break;
	case EXCCAUSE_INSTR_DATA_ERROR:
		/* instr PIF data error */
		exception = GDB_EXCEPTION_MEMORY_FAULT;
		break;
	case EXCCAUSE_LOAD_STORE_DATA_ERROR:
		/* load/store PIF data error */
		exception = GDB_EXCEPTION_MEMORY_FAULT;
		break;
	case EXCCAUSE_INSTR_ADDR_ERROR:
		/* instr PIF addr error */
		exception = GDB_EXCEPTION_MEMORY_FAULT;
		break;
	case EXCCAUSE_LOAD_STORE_ADDR_ERROR:
		/* load/store PIF addr error */
		exception = GDB_EXCEPTION_MEMORY_FAULT;
		break;
	case EXCCAUSE_INSTR_PROHIBITED:
		/* inst fetch prohibited */
		exception = GDB_EXCEPTION_INVALID_MEMORY;
		break;
	case EXCCAUSE_LOAD_STORE_RING:
		/* load/store privilege */
		exception = GDB_EXCEPTION_INVALID_MEMORY;
		break;
	case EXCCAUSE_LOAD_PROHIBITED:
		/* load prohibited */
		exception = GDB_EXCEPTION_INVALID_MEMORY;
		break;
	case EXCCAUSE_STORE_PROHIBITED:
		/* store prohibited */
		exception = GDB_EXCEPTION_INVALID_MEMORY;
		break;
	case EXCCAUSE_CP0_DISABLED:
		__fallthrough;
	case EXCCAUSE_CP1_DISABLED:
		__fallthrough;
	case EXCCAUSE_CP2_DISABLED:
		__fallthrough;
	case EXCCAUSE_CP3_DISABLED:
		__fallthrough;
	case EXCCAUSE_CP4_DISABLED:
		__fallthrough;
	case EXCCAUSE_CP5_DISABLED:
		__fallthrough;
	case EXCCAUSE_CP6_DISABLED:
		__fallthrough;
	case EXCCAUSE_CP7_DISABLED:
		/* coprocessor disabled */
		exception = GDB_EXCEPTION_INVALID_INSTRUCTION;
		break;
	default:
		exception = GDB_EXCEPTION_MEMORY_FAULT;
		break;
	}

	return exception;
}

/**
 * Copy debug information from stack into GDB context.
 *
 * This copies the information stored in the stack into the GDB
 * context for the thread being debugged.
 *
 * @param ctx   GDB context
 * @param stack Pointer to the stack frame
 */
static void copy_to_ctx(struct gdb_ctx *ctx, const z_arch_esf_t *stack)
{
	struct xtensa_register *reg;
	int idx, num_laddr_regs;

	uint32_t *bsa = *(int **)stack;

	if ((int *)bsa - stack > 4) {
		num_laddr_regs = 8;
	} else if ((int *)bsa - stack > 8) {
		num_laddr_regs = 12;
	} else if ((int *)bsa - stack > 12) {
		num_laddr_regs = 16;
	} else {
		num_laddr_regs = 4;
	}

	/* Get logical address registers A0 - A<num_laddr_regs> from stack */
	for (idx = 0; idx < num_laddr_regs; idx++) {
		reg = &xtensa_gdb_ctx.regs[xtensa_gdb_ctx.a0_idx + idx];

		if (reg->regno == SOC_GDB_REGNO_A1) {
			/* A1 is calculated */
			reg->val = POINTER_TO_UINT(
					((char *)bsa) + BASE_SAVE_AREA_SIZE);
			reg->seqno = ctx->seqno;
		} else {
			reg->val = bsa[reg->stack_offset / 4];
			reg->seqno = ctx->seqno;
		}
	}

	/* For registers other than logical address registers */
	for (idx = 0; idx < xtensa_gdb_ctx.num_regs; idx++) {
		reg = &xtensa_gdb_ctx.regs[idx];

		if (gdb_xtensa_is_logical_addr_reg(reg)) {
			/* Logical address registers are handled above */
			continue;
		} else if (reg->stack_offset != 0) {
			/* For those registers stashed in stack */
			reg->val = bsa[reg->stack_offset / 4];
			reg->seqno = ctx->seqno;
		} else if (gdb_xtensa_is_special_reg(reg)) {
			read_sreg(ctx, reg);
		}
	}

#if XCHAL_HAVE_WINDOWED
	uint8_t a0_idx, ar_idx, wb_start;

	wb_start = (uint8_t)xtensa_gdb_ctx.regs[xtensa_gdb_ctx.wb_idx].val;

	/*
	 * Copied the logical registers A0-A15 to physical registers (AR*)
	 * according to WINDOWBASE.
	 */
	for (idx = 0; idx < num_laddr_regs; idx++) {
		/* Index to register description array for A */
		a0_idx = xtensa_gdb_ctx.a0_idx + idx;

		/* Find the start of window (== WINDOWBASE * 4) */
		ar_idx = wb_start * 4;
		/* Which logical register we are working on... */
		ar_idx += idx;
		/* Wrap around A64 (or A32) -> A0 */
		ar_idx %= XCHAL_NUM_AREGS;
		/* Index to register description array for AR */
		ar_idx += xtensa_gdb_ctx.ar_idx;

		xtensa_gdb_ctx.regs[ar_idx].val = xtensa_gdb_ctx.regs[a0_idx].val;
		xtensa_gdb_ctx.regs[ar_idx].seqno = xtensa_gdb_ctx.regs[a0_idx].seqno;
	}
#endif

	/* Disable stepping */
	set_one_sreg(ICOUNT, 0);
	set_one_sreg(ICOUNTLEVEL, 0);
	__asm__ volatile("isync");
}

/**
 * Restore debug information from stack into GDB context.
 *
 * This copies the information stored the GDB context back into
 * the stack. So that the thread being debugged has new values
 * after context switch from GDB stub back to the thread.
 *
 * @param ctx   GDB context
 * @param stack Pointer to the stack frame
 */
static void restore_from_ctx(struct gdb_ctx *ctx, const z_arch_esf_t *stack)
{
	struct xtensa_register *reg;
	int idx, num_laddr_regs;

	uint32_t *bsa = *(int **)stack;

	if ((int *)bsa - stack > 4) {
		num_laddr_regs = 8;
	} else if ((int *)bsa - stack > 8) {
		num_laddr_regs = 12;
	} else if ((int *)bsa - stack > 12) {
		num_laddr_regs = 16;
	} else {
		num_laddr_regs = 4;
	}

	/*
	 * Note that we don't need to copy AR* back to A* for
	 * windowed registers. GDB manipulates A0-A15 directly
	 * without going through AR*.
	 */

	/*
	 * Push values of logical address registers A0 - A<num_laddr_regs>
	 * back to stack.
	 */
	for (idx = 0; idx < num_laddr_regs; idx++) {
		reg = &xtensa_gdb_ctx.regs[xtensa_gdb_ctx.a0_idx + idx];

		if (reg->regno == SOC_GDB_REGNO_A1) {
			/* Shouldn't be changing stack pointer */
			continue;
		} else {
			bsa[reg->stack_offset / 4] = reg->val;
		}
	}

	for (idx = 0; idx < xtensa_gdb_ctx.num_regs; idx++) {
		reg = &xtensa_gdb_ctx.regs[idx];

		if (gdb_xtensa_is_logical_addr_reg(reg)) {
			/* Logical address registers are handled above */
			continue;
		} else if (reg->stack_offset != 0) {
			/* For those registers stashed in stack */
			bsa[reg->stack_offset / 4] = reg->val;
		} else if (gdb_xtensa_is_special_reg(reg)) {
			/*
			 * Currently not writing back any special
			 * registers.
			 */
			continue;
		}
	}

	if (!not_first_break) {
		/*
		 * Need to go past the BREAK.N instruction (16-bit)
		 * in arch_gdb_init(). Or else the SoC will simply
		 * go back to execute the BREAK.N instruction,
		 * which raises debug interrupt, and we will be
		 * stuck in an infinite loop.
		 */
		bsa[BSA_PC_OFF / 4] += 2;
		not_first_break = true;
	}
}

void arch_gdb_continue(void)
{
	/*
	 * No need to do anything. Simply let the GDB stub main
	 * loop to return from debug interrupt for code to
	 * continue running.
	 */
}

void arch_gdb_step(void)
{
	set_one_sreg(ICOUNT, 0xFFFFFFFEU);
	set_one_sreg(ICOUNTLEVEL, XCHAL_DEBUGLEVEL);
	__asm__ volatile("isync");
}

/**
 * Convert a register value into hex string.
 *
 * Note that this assumes the output buffer always has enough
 * space.
 *
 * @param reg Xtensa register
 * @param hex Pointer to output buffer
 * @return Number of bytes written to output buffer
 */
static size_t reg2hex(const struct xtensa_register *reg, char *hex)
{
	uint8_t *bin = (uint8_t *)&reg->val;
	size_t binlen = reg->byte_size;

	for (size_t i = 0; i < binlen; i++) {
		if (hex2char(bin[i] >> 4, &hex[2 * i]) < 0) {
			return 0;
		}
		if (hex2char(bin[i] & 0xf, &hex[2 * i + 1]) < 0) {
			return 0;
		}
	}

	return 2 * binlen;
}

size_t arch_gdb_reg_readall(struct gdb_ctx *ctx, uint8_t *buf, size_t buflen)
{
	struct xtensa_register *reg;
	int idx;
	uint8_t *output;
	size_t ret;

	if (buflen < SOC_GDB_GPKT_HEX_SIZE) {
		ret = 0;
		goto out;
	}

	/*
	 * Fill with 'x' to mark them as available since most registers
	 * are not available in the stack.
	 */
	memset(buf, 'x', SOC_GDB_GPKT_HEX_SIZE);

	ret = 0;
	for (idx = 0; idx < ctx->num_regs; idx++) {
		reg = &ctx->regs[idx];

		if (reg->seqno != ctx->seqno) {
			/*
			 * Register struct has stale value from
			 * previous debug interrupt. Don't
			 * send it out.
			 */
			continue;
		}

		if ((reg->gpkt_offset < 0) ||
		    (reg->gpkt_offset >= SOC_GDB_GPKT_BIN_SIZE)) {
			/*
			 * Register is not in G-packet, or
			 * beyond maximum size of G-packet.
			 *
			 * xtensa-config.c may specify G-packet
			 * offset beyond what GDB expects, so
			 * need to make sure we won't write beyond
			 * the buffer.
			 */
			continue;
		}

		/* Two hex characters per byte */
		output = &buf[reg->gpkt_offset * 2];

		if (reg2hex(reg, output) == 0) {
			goto out;
		}
	}

	ret = SOC_GDB_GPKT_HEX_SIZE;

out:
	return ret;
}

size_t arch_gdb_reg_writeall(struct gdb_ctx *ctx, uint8_t *hex, size_t hexlen)
{
	/*
	 * GDB on Xtensa does not seem to use G-packet to write register
	 * values. So we can skip this function.
	 */

	ARG_UNUSED(ctx);
	ARG_UNUSED(hex);
	ARG_UNUSED(hexlen);

	return 0;
}

size_t arch_gdb_reg_readone(struct gdb_ctx *ctx, uint8_t *buf, size_t buflen,
			    uint32_t regno)
{
	struct xtensa_register *reg;
	int idx;
	size_t ret;

	ret = 0;
	for (idx = 0; idx < ctx->num_regs; idx++) {
		reg = &ctx->regs[idx];

		/*
		 * GDB sends the G-packet index as register number
		 * instead of the actual Xtensa register number.
		 */
		if (reg->idx == regno) {
			if (reg->seqno != ctx->seqno) {
				/*
				 * Register value has stale value from
				 * previous debug interrupt. Report
				 * register value as unavailable.
				 *
				 * Don't report error here, or else GDB
				 * may stop the debug session.
				 */

				if (buflen < 2) {
					/* Output buffer cannot hold 'xx' */
					goto out;
				}

				buf[0] = 'x';
				buf[1] = 'x';
				ret = 2;
				goto out;
			}

			/* Make sure output buffer is large enough */
			if (buflen < (reg->byte_size * 2)) {
				goto out;
			}

			ret = reg2hex(reg, buf);

			break;
		}
	}

out:
	return ret;
}

size_t arch_gdb_reg_writeone(struct gdb_ctx *ctx, uint8_t *hex, size_t hexlen,
			     uint32_t regno)
{
	struct xtensa_register *reg = NULL;
	int idx;
	size_t ret;

	ret = 0;
	for (idx = 0; idx < ctx->num_regs; idx++) {
		reg = &ctx->regs[idx];

		/*
		 * Remember GDB sends index number instead of
		 * actual register number (as defined in Xtensa
		 * architecture).
		 */
		if (reg->idx != regno) {
			continue;
		}

		if (hexlen < (reg->byte_size * 2)) {
			/* Not enough hex digits to fill the register */
			goto out;
		}

		/* Register value is now up-to-date */
		reg->seqno = ctx->seqno;

		/* Convert from hexadecimal into binary */
		ret = hex2bin(hex, hexlen,
			      (uint8_t *)&reg->val, reg->byte_size);
		break;
	}

out:
	return ret;
}

int arch_gdb_add_breakpoint(struct gdb_ctx *ctx, uint8_t type,
			    uintptr_t addr, uint32_t kind)
{
	int ret, idx;
	uint32_t ibreakenable;

	switch (type) {
	case 1:
		/* Hardware breakpoint */
		ibreakenable = get_one_sreg(IBREAKENABLE);
		for (idx = 0; idx < MAX(XCHAL_NUM_IBREAK, 2); idx++) {
			/* Find an empty IBREAK slot */
			if ((ibreakenable & BIT(idx)) == 0) {
				/* Set breakpoint address */

				if (idx == 0) {
					set_one_sreg(IBREAKA0, addr);
				} else if (idx == 1) {
					set_one_sreg(IBREAKA1, addr);
				} else {
					ret = -1;
					goto out;
				}

				/* Enable the breakpoint */
				ibreakenable |= BIT(idx);
				set_one_sreg(IBREAKENABLE, ibreakenable);

				ret = 0;
				goto out;
			}
		}

		/* Cannot find an empty slot, return error */
		ret = -1;
		break;
	case 0:
		/*
		 * Software breakpoint is to replace the instruction at
		 * target address with BREAK or BREAK.N. GDB, by default,
		 * does this by using memory write packets to replace
		 * instructions. So there is no need to implement
		 * software breakpoint here.
		 */
		__fallthrough;
	default:
		/* Breakpoint type not supported */
		ret = -2;
		break;
	}

out:
	return ret;
}

int arch_gdb_remove_breakpoint(struct gdb_ctx *ctx, uint8_t type,
			       uintptr_t addr, uint32_t kind)
{
	int ret, idx;
	uint32_t ibreakenable, ibreak;

	switch (type) {
	case 1:
		/* Hardware breakpoint */
		ibreakenable = get_one_sreg(IBREAKENABLE);
		for (idx = 0; idx < MAX(XCHAL_NUM_IBREAK, 2); idx++) {
			/* Find an active IBREAK slot and compare address */
			if ((ibreakenable & BIT(idx)) == BIT(idx)) {
				if (idx == 0) {
					ibreak = get_one_sreg(IBREAKA0);
				} else if (idx == 1) {
					ibreak = get_one_sreg(IBREAKA1);
				} else {
					ret = -1;
					goto out;
				}

				if (ibreak == addr) {
					/* Clear breakpoint address */
					if (idx == 0) {
						set_one_sreg(IBREAKA0, 0U);
					} else if (idx == 1) {
						set_one_sreg(IBREAKA1, 0U);
					} else {
						ret = -1;
						goto out;
					}

					/* Disable the breakpoint */
					ibreakenable &= ~BIT(idx);
					set_one_sreg(IBREAKENABLE,
						     ibreakenable);

					ret = 0;
					goto out;
				}
			}
		}

		/*
		 * Cannot find matching breakpoint address,
		 * return error.
		 */
		ret = -1;

		break;
	case 0:
		/*
		 * Software breakpoint is to replace the instruction at
		 * target address with BREAK or BREAK.N. GDB, by default,
		 * does this by using memory write packets to restore
		 * instructions. So there is no need to implement
		 * software breakpoint here.
		 */
		__fallthrough;
	default:
		/* Breakpoint type not supported */
		ret = -2;
		break;
	}

out:
	return ret;
}

void z_gdb_isr(z_arch_esf_t *esf)
{
	uint32_t reg;

	reg = get_one_sreg(DEBUGCAUSE);
	if (reg != 0) {
		/* Manual breaking */
		xtensa_gdb_ctx.exception = GDB_EXCEPTION_BREAKPOINT;
	} else {
		/* Actual exception */
		reg = get_one_sreg(EXCCAUSE);
		xtensa_gdb_ctx.exception = get_gdb_exception_reason(reg);
	}

	xtensa_gdb_ctx.seqno++;

	/* Copy registers into GDB context */
	copy_to_ctx(&xtensa_gdb_ctx, esf);

	z_gdb_main_loop(&xtensa_gdb_ctx);

	/* Restore registers from GDB context */
	restore_from_ctx(&xtensa_gdb_ctx, esf);
}

void arch_gdb_init(void)
{
	int idx;

	/*
	 * Find out the starting index in the register
	 * description array of certain registers.
	 */
	for (idx = 0; idx < xtensa_gdb_ctx.num_regs; idx++) {
		switch (xtensa_gdb_ctx.regs[idx].regno) {
		case 0x0000:
			/* A0: 0x0000 */
			xtensa_gdb_ctx.a0_idx = idx;
			break;
		case XTREG_GRP_ADDR:
			/* AR0: 0x0100 */
			xtensa_gdb_ctx.ar_idx = idx;
			break;
		case (XTREG_GRP_SPECIAL + WINDOWBASE):
			/* WINDOWBASE (Special Register) */
			xtensa_gdb_ctx.wb_idx = idx;
			break;
		default:
			break;
		};
	}

	/*
	 * The interrupt enable bits for higher level interrupts
	 * (level 2+) sit just after the level-1 interrupts.
	 * The need to do a minus 2 is simply that the first bit
	 * after level-1 interrupts is for level-2 interrupt.
	 * So need to do an offset by subtraction.
	 */
	z_xtensa_irq_enable(XCHAL_NUM_EXTINTERRUPTS +
			    XCHAL_DEBUGLEVEL - 2);

	/*
	 * Break and go into the GDB stub.
	 * The underscore in front is to avoid toolchain
	 * converting BREAK.N into BREAK which is bigger.
	 * This is needed as the GDB stub will need to change
	 * the program counter past this instruction to
	 * continue working. Or else SoC would repeatedly
	 * raise debug exception on this instruction and
	 * won't go forward.
	 */
	__asm__ volatile ("_break.n 0");
}
