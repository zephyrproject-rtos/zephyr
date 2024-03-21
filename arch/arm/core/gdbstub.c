/*
 * Copyright (c) 2023 Marek Vedral <vedrama5@fel.cvut.cz>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <kernel_internal.h>
#include <zephyr/arch/arm/gdbstub.h>
#include <zephyr/debug/gdbstub.h>

/* Position of each register in the packet - n-th register in the ctx.registers array needs to be
 * the packet_pos[n]-th byte of the g (read all registers) packet. See struct arm_register_names in
 * GDB file gdb/arm-tdep.c, which defines these positions.
 */
static const int packet_pos[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 41};

/* Required struct */
static struct gdb_ctx ctx;

/* Return true if BKPT instruction caused the current entry */
static int is_bkpt(unsigned int exc_cause)
{
	int ret = 0;

	if (exc_cause == GDB_EXCEPTION_BREAKPOINT) {
		/* Get the instruction */
		unsigned int instr = sys_read32(ctx.registers[PC]);
		/* Try to check the instruction encoding */
		int ist = ((ctx.registers[SPSR] & BIT(SPSR_J)) >> (SPSR_J - 1)) |
			  ((ctx.registers[SPSR] & BIT(SPSR_T)) >> SPSR_T);

		if (ist == SPSR_ISETSTATE_ARM) {
			/* ARM instruction set state */
			ret = ((instr & 0xFF00000) == 0x1200000) && ((instr & 0xF0) == 0x70);
		} else if (ist != SPSR_ISETSTATE_JAZELLE) {
			/* Thumb or ThumbEE encoding */
			ret = ((instr & 0xFF00) == 0xBE00);
		}
	}
	return ret;
}

/* Wrapper function to save and restore execution c */
void z_gdb_entry(z_arch_esf_t *esf, unsigned int exc_cause)
{
	/* Disable the hardware breakpoint in case it was set */
	__asm__ volatile("mcr p14, 0, %0, c0, c0, 5" ::"r"(0x0) :);

	ctx.exception = exc_cause;
	/* save the registers */
	ctx.registers[R0] = esf->basic.r0;
	ctx.registers[R1] = esf->basic.r1;
	ctx.registers[R2] = esf->basic.r2;
	ctx.registers[R3] = esf->basic.r3;
	/* The EXTRA_EXCEPTION_INFO kernel option ensures these regs are set */
	ctx.registers[R4] = esf->extra_info.callee->v1;
	ctx.registers[R5] = esf->extra_info.callee->v2;
	ctx.registers[R6] = esf->extra_info.callee->v3;
	ctx.registers[R7] = esf->extra_info.callee->v4;
	ctx.registers[R8] = esf->extra_info.callee->v5;
	ctx.registers[R9] = esf->extra_info.callee->v6;
	ctx.registers[R10] = esf->extra_info.callee->v7;
	ctx.registers[R11] = esf->extra_info.callee->v8;
	ctx.registers[R13] = esf->extra_info.callee->psp;

	ctx.registers[R12] = esf->basic.r12;
	ctx.registers[LR] = esf->basic.lr;
	ctx.registers[PC] = esf->basic.pc;
	ctx.registers[SPSR] = esf->basic.xpsr;

	/* True if entering after a BKPT instruction */
	const int bkpt_entry = is_bkpt(exc_cause);

	z_gdb_main_loop(&ctx);

	/* The registers part of EXTRA_EXCEPTION_INFO are read-only - the excpetion return code
	 * does not restore them, thus we don't need to do so here
	 */
	esf->basic.r0 = ctx.registers[R0];
	esf->basic.r1 = ctx.registers[R1];
	esf->basic.r2 = ctx.registers[R2];
	esf->basic.r3 = ctx.registers[R3];
	esf->basic.r12 = ctx.registers[R12];
	esf->basic.lr = ctx.registers[LR];
	esf->basic.pc = ctx.registers[PC];
	esf->basic.xpsr = ctx.registers[SPSR];
	/* TODO: restore regs from extra exc. info */

	if (bkpt_entry) {
		/* Apply this offset, so that the process won't be affected by the
		 * BKPT instruction
		 */
		esf->basic.pc += 0x4;
	}
	esf->basic.xpsr = ctx.registers[SPSR];
}

void arch_gdb_init(void)
{
	uint32_t reg_val;
	/* Enable the monitor debug mode */
	__asm__ volatile("mrc p14, 0, %0, c0, c2, 2" : "=r"(reg_val)::);
	reg_val |= DBGDSCR_MONITOR_MODE_EN;
	__asm__ volatile("mcr p14, 0, %0, c0, c2, 2" ::"r"(reg_val) :);

	/* Generate the Prefetch abort exception */
	__asm__ volatile("BKPT");
}

void arch_gdb_continue(void)
{
	/* No need to do anything, return to the code. */
}

void arch_gdb_step(void)
{
	/* Set the hardware breakpoint */
	uint32_t reg_val = ctx.registers[PC];
	/* set BVR (Breakpoint value register) to PC, make sure it is word aligned */
	reg_val &= ~(0x3);
	__asm__ volatile("mcr p14, 0, %0, c0, c0, 4" ::"r"(reg_val) :);

	reg_val = 0;
	/* Address mismatch */
	reg_val |= (DBGDBCR_MEANING_ADDR_MISMATCH & DBGDBCR_MEANING_MASK) << DBGDBCR_MEANING_SHIFT;
	/* Match any other instruction */
	reg_val |= (0xF & DBGDBCR_BYTE_ADDR_MASK) << DBGDBCR_BYTE_ADDR_SHIFT;
	/* Breakpoint enable */
	reg_val |= DBGDBCR_BRK_EN_MASK;
	__asm__ volatile("mcr p14, 0, %0, c0, c0, 5" ::"r"(reg_val) :);
}

size_t arch_gdb_reg_readall(struct gdb_ctx *c, uint8_t *buf, size_t buflen)
{
	int ret = 0;
	/* All other registers are not supported */
	memset(buf, 'x', buflen);
	for (int i = 0; i < GDB_NUM_REGS; i++) {
		/* offset inside the packet */
		int pos = packet_pos[i] * 8;
		int r = bin2hex((const uint8_t *)(c->registers + i), 4, buf + pos, buflen - pos);
		/* remove the newline character placed by the bin2hex function */
		buf[pos + 8] = 'x';
		if (r == 0) {
			ret = 0;
			break;
		}
		ret += r;
	}

	if (ret) {
		/* Since we don't support some floating point registers, set the packet size
		 * manually
		 */
		ret = GDB_READALL_PACKET_SIZE;
	}
	return ret;
}

size_t arch_gdb_reg_writeall(struct gdb_ctx *c, uint8_t *hex, size_t hexlen)
{
	int ret = 0;

	for (unsigned int i = 0; i < hexlen; i += 8) {
		if (hex[i] != 'x') {
			/* check if the stub supports this register */
			for (unsigned int j = 0; j < GDB_NUM_REGS; j++) {
				if (packet_pos[j] != i) {
					continue;
				}
				int r = hex2bin(hex + i * 8, 8, (uint8_t *)(c->registers + j), 4);

				if (r == 0) {
					return 0;
				}
				ret += r;
			}
		}
	}
	return ret;
}

size_t arch_gdb_reg_readone(struct gdb_ctx *c, uint8_t *buf, size_t buflen, uint32_t regno)
{
	/* Reading four bytes (could be any return value except 0, which would indicate an error) */
	int ret = 4;
	/* Fill the buffer with 'x' in case the stub does not support the required register */
	memset(buf, 'x', 8);
	if (regno == SPSR_REG_IDX) {
		/* The SPSR register is at the end, we have to check separately */
		ret = bin2hex((uint8_t *)(c->registers + GDB_NUM_REGS - 1), 4, buf, buflen);
	} else {
		/* Check which of our registers corresponds to regnum */
		for (int i = 0; i < GDB_NUM_REGS; i++) {
			if (packet_pos[i] == regno) {
				ret = bin2hex((uint8_t *)(c->registers + i), 4, buf, buflen);
				break;
			}
		}
	}
	return ret;
}

size_t arch_gdb_reg_writeone(struct gdb_ctx *c, uint8_t *hex, size_t hexlen, uint32_t regno)
{
	int ret = 0;
	/* Set the value of a register */
	if (hexlen != 8) {
		return ret;
	}

	if (regno < (GDB_NUM_REGS - 1)) {
		/* Again, check the corresponding register index */
		for (int i = 0; i < GDB_NUM_REGS; i++) {
			if (packet_pos[i] == regno) {
				ret = hex2bin(hex, hexlen, (uint8_t *)(c->registers + i), 4);
				break;
			}
		}
	} else if (regno == SPSR_REG_IDX) {
		ret = hex2bin(hex, hexlen, (uint8_t *)(c->registers + GDB_NUM_REGS - 1), 4);
	}
	return ret;
}
