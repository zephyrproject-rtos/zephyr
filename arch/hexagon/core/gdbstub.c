/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Hexagon GDB stub
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/debug/gdbstub.h>
#include <zephyr/arch/hexagon/gdbstub.h>
#include <zephyr/arch/hexagon/arch.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/uart.h>
#include <hexagon_vm.h>
#include <event_context.h>
#include <hexagon_fatal.h>

#ifdef CONFIG_GDBSTUB

/* Provided by subsys/debug/gdbstub/gdbstub.c */
extern int z_gdb_main_loop(struct gdb_ctx *ctx);

/* GDB stub context */
static struct gdb_ctx hexagon_gdb_ctx;

/*
 * Memory regions accessible by GDB.
 *
 * Single RW region covering the full 16MB SRAM window where code and data
 * reside, plus the PL011 UART I/O region.
 *
 * TODO: These addresses are specific to the QEMU virtual board layout
 * (0xa0000000 = SRAM base, 0x10000000 = PL011 UART).  When a real SoC
 * board is added, these regions should be defined in the board file or
 * derived from DTS rather than hardcoded here.
 */
const struct gdb_mem_region gdb_mem_region_array[] = {
	{
		.start = (uintptr_t)__text_region_start,
		.end = (uintptr_t)__text_region_end,
		.attributes = GDB_MEM_REGION_RO,
		.alignment = 4,
	},
	{
		.start = (uintptr_t)__rodata_region_start,
		.end = (uintptr_t)__rodata_region_end,
		.attributes = GDB_MEM_REGION_RO,
		.alignment = 4,
	},
	{
		.start = (uintptr_t)_image_ram_start,
		.end = (uintptr_t)_image_ram_end,
		.attributes = GDB_MEM_REGION_RW,
		.alignment = 4,
	},
	{
		.start = DT_REG_ADDR(DT_CHOSEN(zephyr_console)),
		.end = DT_REG_ADDR(DT_CHOSEN(zephyr_console)) +
			DT_REG_SIZE(DT_CHOSEN(zephyr_console)),
		.attributes = GDB_MEM_REGION_RW,
		.alignment = 4,
	},
};

const size_t gdb_mem_num_regions = ARRAY_SIZE(gdb_mem_region_array);

bool z_hexagon_is_gdb_breakpoint(uintptr_t addr)
{
	uint8_t alignment;

	if ((addr & 3U) != 0U || !gdb_mem_can_read(addr, sizeof(uint32_t), &alignment)) {
		return false;
	}

	return *(const uint32_t *)addr == HEXAGON_BREAK_INSN;
}

/**
 * Core entry point from the exception/trap0 handlers (irq_manage.c).
 *
 * ctx already holds the full register file -- EVENT_ENTRY
 * (event_handlers.S) saves the callee-saved GPRs and GP/UGP into it
 * whenever CONFIG_GDBSTUB is set, on top of the volatile registers and
 * special registers it always saves. Copies that into the GDB context,
 * calls the GDB main loop, then copies any modifications back so the
 * assembly restore path (EVENT_EXIT) picks them up.
 */
void z_hexagon_gdb_entry(struct event_context *ctx)
{
	uint32_t *regs = hexagon_gdb_ctx.regs;

	/* --- Copy volatile registers from event_context (r0-r15) --- */
	regs[GDB_HEXAGON_R0]  = ctx->r0_r1[0];
	regs[GDB_HEXAGON_R1]  = ctx->r0_r1[1];
	regs[GDB_HEXAGON_R2]  = ctx->r2_r3[0];
	regs[GDB_HEXAGON_R3]  = ctx->r2_r3[1];
	regs[GDB_HEXAGON_R4]  = ctx->r4_r5[0];
	regs[GDB_HEXAGON_R5]  = ctx->r4_r5[1];
	regs[GDB_HEXAGON_R6]  = ctx->r6_r7[0];
	regs[GDB_HEXAGON_R7]  = ctx->r6_r7[1];
	regs[GDB_HEXAGON_R8]  = ctx->r8_r9[0];
	regs[GDB_HEXAGON_R9]  = ctx->r8_r9[1];
	regs[GDB_HEXAGON_R10] = ctx->r10_r11[0];
	regs[GDB_HEXAGON_R11] = ctx->r10_r11[1];
	regs[GDB_HEXAGON_R12] = ctx->r12_r13[0];
	regs[GDB_HEXAGON_R13] = ctx->r12_r13[1];
	regs[GDB_HEXAGON_R14] = ctx->r14_r15[0];
	regs[GDB_HEXAGON_R15] = ctx->r14_r15[1];

	/* --- Copy callee-saved registers, saved by EVENT_ENTRY --- */
	regs[GDB_HEXAGON_R16] = ctx->r16_r17[0];
	regs[GDB_HEXAGON_R17] = ctx->r16_r17[1];
	regs[GDB_HEXAGON_R18] = ctx->r18_r19[0];
	regs[GDB_HEXAGON_R19] = ctx->r18_r19[1];
	regs[GDB_HEXAGON_R20] = ctx->r20_r21[0];
	regs[GDB_HEXAGON_R21] = ctx->r20_r21[1];
	regs[GDB_HEXAGON_R22] = ctx->r22_r23[0];
	regs[GDB_HEXAGON_R23] = ctx->r22_r23[1];
	regs[GDB_HEXAGON_R24] = ctx->r24_r25[0];
	regs[GDB_HEXAGON_R25] = ctx->r24_r25[1];
	regs[GDB_HEXAGON_R26] = ctx->r26_r27[0];
	regs[GDB_HEXAGON_R27] = ctx->r26_r27[1];
	regs[GDB_HEXAGON_R28] = ctx->r28;

	/*
	 * SP (r29): The event_context sits on the interrupted code's stack.
	 * allocframe pushed FP:LR and then the context, so the pre-exception
	 * SP is at: (address of ctx) + EVENT_CTX_SIZE + EVENT_ENTRY_ALLOCFRAME_OVERHEAD.
	 */
	regs[GDB_HEXAGON_R29] = (uint32_t)ctx + EVENT_CTX_SIZE + EVENT_ENTRY_ALLOCFRAME_OVERHEAD;

	/* FP (r30): allocframe saved the old FP above the event context. */
	if (!z_hexagon_event_frame_fp_get(ctx, &regs[GDB_HEXAGON_R30])) {
		regs[GDB_HEXAGON_R30] = 0U;
	}

	/* LR (r31) */
	regs[GDB_HEXAGON_R31] = ctx->link_reg;

	/* PC = GELR (return address from the event) */
	regs[GDB_HEXAGON_PC] = ctx->gelr;

	/* Special registers */
	regs[GDB_HEXAGON_USR] = ctx->usr;
	regs[GDB_HEXAGON_GP]  = ctx->gp;
	regs[GDB_HEXAGON_UGP] = ctx->ugp;
	regs[GDB_HEXAGON_LC0] = ctx->lc0;
	regs[GDB_HEXAGON_LC1] = ctx->lc1;
	regs[GDB_HEXAGON_SA0] = ctx->sa0;
	regs[GDB_HEXAGON_SA1] = ctx->sa1;

	/* Split predicate word into P0-P3 */
	regs[GDB_HEXAGON_P0] = (ctx->pred_regs >> 0)  & 0xFF;
	regs[GDB_HEXAGON_P1] = (ctx->pred_regs >> 8)  & 0xFF;
	regs[GDB_HEXAGON_P2] = (ctx->pred_regs >> 16) & 0xFF;
	regs[GDB_HEXAGON_P3] = (ctx->pred_regs >> 24) & 0xFF;

	/* Set exception reason */
	hexagon_gdb_ctx.exception = GDB_EXCEPTION_BREAKPOINT;

	/* Enter GDB main loop (blocks until GDB says continue/step) */
	z_gdb_main_loop(&hexagon_gdb_ctx);

	/* --- Copy modified registers back --- */

	/* Volatile regs back to event_context */
	ctx->r0_r1[0]    = regs[GDB_HEXAGON_R0];
	ctx->r0_r1[1]    = regs[GDB_HEXAGON_R1];
	ctx->r2_r3[0]    = regs[GDB_HEXAGON_R2];
	ctx->r2_r3[1]    = regs[GDB_HEXAGON_R3];
	ctx->r4_r5[0]    = regs[GDB_HEXAGON_R4];
	ctx->r4_r5[1]    = regs[GDB_HEXAGON_R5];
	ctx->r6_r7[0]    = regs[GDB_HEXAGON_R6];
	ctx->r6_r7[1]    = regs[GDB_HEXAGON_R7];
	ctx->r8_r9[0]    = regs[GDB_HEXAGON_R8];
	ctx->r8_r9[1]    = regs[GDB_HEXAGON_R9];
	ctx->r10_r11[0]  = regs[GDB_HEXAGON_R10];
	ctx->r10_r11[1]  = regs[GDB_HEXAGON_R11];
	ctx->r12_r13[0]  = regs[GDB_HEXAGON_R12];
	ctx->r12_r13[1]  = regs[GDB_HEXAGON_R13];
	ctx->r14_r15[0]  = regs[GDB_HEXAGON_R14];
	ctx->r14_r15[1]  = regs[GDB_HEXAGON_R15];

	/* Callee-saved registers back, restored by EVENT_EXIT */
	ctx->r16_r17[0]  = regs[GDB_HEXAGON_R16];
	ctx->r16_r17[1]  = regs[GDB_HEXAGON_R17];
	ctx->r18_r19[0]  = regs[GDB_HEXAGON_R18];
	ctx->r18_r19[1]  = regs[GDB_HEXAGON_R19];
	ctx->r20_r21[0]  = regs[GDB_HEXAGON_R20];
	ctx->r20_r21[1]  = regs[GDB_HEXAGON_R21];
	ctx->r22_r23[0]  = regs[GDB_HEXAGON_R22];
	ctx->r22_r23[1]  = regs[GDB_HEXAGON_R23];
	ctx->r24_r25[0]  = regs[GDB_HEXAGON_R24];
	ctx->r24_r25[1]  = regs[GDB_HEXAGON_R25];
	ctx->r26_r27[0]  = regs[GDB_HEXAGON_R26];
	ctx->r26_r27[1]  = regs[GDB_HEXAGON_R27];
	ctx->r28         = regs[GDB_HEXAGON_R28];

	/* PC back to GELR (GDB may have changed it) */
	ctx->gelr = regs[GDB_HEXAGON_PC];

	/* LR */
	ctx->link_reg = regs[GDB_HEXAGON_R31];

	/* Special registers */
	ctx->usr = regs[GDB_HEXAGON_USR];
	ctx->gp  = regs[GDB_HEXAGON_GP];
	ctx->ugp = regs[GDB_HEXAGON_UGP];
	ctx->lc0 = regs[GDB_HEXAGON_LC0];
	ctx->lc1 = regs[GDB_HEXAGON_LC1];
	ctx->sa0 = regs[GDB_HEXAGON_SA0];
	ctx->sa1 = regs[GDB_HEXAGON_SA1];

	/* Predicates: reassemble from P0-P3 */
	ctx->pred_regs = (regs[GDB_HEXAGON_P0] & 0xFF)
		       | ((regs[GDB_HEXAGON_P1] & 0xFF) << 8)
		       | ((regs[GDB_HEXAGON_P2] & 0xFF) << 16)
		       | ((regs[GDB_HEXAGON_P3] & 0xFF) << 24);
}

/* Initialize GDB stub */
void arch_gdb_init(void)
{
	memset(&hexagon_gdb_ctx, 0, sizeof(hexagon_gdb_ctx));

	/*
	 * Issue a GDB breakpoint trap to signal the initial stop.  Zephyr
	 * will halt here until a GDB session connects and issues 'continue'.
	 * If no GDB client is attached, z_gdb_getchar() spins forever and
	 * the system will appear hung at boot.  This is intentional: the
	 * GDB stub is meant to be used with an active debugger.  The board's
	 * testcase.yaml should use build_only: true if the test runner does
	 * not attach GDB.
	 */
	__asm__ volatile("trap0(#0xdb)");
}

/* Continue execution - context restore is handled by the assembly return path */
void arch_gdb_continue(void)
{
	/* Nothing to do: z_gdb_main_loop returns, z_hexagon_gdb_entry
	 * copies regs back, and the assembly restores context + vmrte.
	 */
}

/* Single step - not implemented (requires instruction-level branch parsing) */
void arch_gdb_step(void)
{
	/* Software single-step not yet available on Hexagon.
	 * Fall through to continue; GDB can use manual breakpoints.
	 */
	arch_gdb_continue();
}

/* Get all registers */
size_t arch_gdb_reg_readall(struct gdb_ctx *ctx, uint8_t *buf, size_t buf_size)
{
	if (buf_size < (sizeof(ctx->regs) * 2)) {
		return 0;
	}

	return bin2hex((const uint8_t *)&(ctx->regs),
		       sizeof(ctx->regs), buf, buf_size);
}

/* Set all registers */
size_t arch_gdb_reg_writeall(struct gdb_ctx *ctx, uint8_t *buf, size_t buf_size)
{
	if (buf_size < (sizeof(ctx->regs) * 2)) {
		return 0;
	}

	return hex2bin(buf, buf_size, (uint8_t *)&(ctx->regs), sizeof(ctx->regs));
}

/* Read single register */
size_t arch_gdb_reg_readone(struct gdb_ctx *ctx, uint8_t *buf, size_t buf_size,
			    uint32_t regno)
{
	if (regno >= GDB_HEXAGON_NUM_REGS || buf_size < 8) {
		return 0;
	}

	return bin2hex((const uint8_t *)&(ctx->regs[regno]), 4, buf, buf_size);
}

/* Write single register */
size_t arch_gdb_reg_writeone(struct gdb_ctx *ctx, uint8_t *buf, size_t buf_size,
			     uint32_t regno)
{
	if (regno >= GDB_HEXAGON_NUM_REGS || buf_size < 8) {
		return 0;
	}

	return hex2bin(buf, 8, (uint8_t *)&(ctx->regs[regno]), 4);
}

/* Add software breakpoint */
int arch_gdb_add_breakpoint(struct gdb_ctx *ctx, uint8_t type,
			    uintptr_t addr, uint32_t kind)
{
	static const uint32_t brkpt = HEXAGON_BREAK_INSN;

	ARG_UNUSED(kind);

	if (type != 0) {
		return -1;
	}

	for (int i = 0; i < GDB_MAX_BREAKPOINTS; i++) {
		if (!ctx->breakpoints[i].active) {
			ctx->breakpoints[i].addr = addr;
			ctx->breakpoints[i].saved_insn = *(uint32_t *)addr;

			memcpy((void *)addr, &brkpt, sizeof(brkpt));

			hexagon_vm_cache_idsync((uint32_t)addr, sizeof(brkpt));

			ctx->breakpoints[i].active = true;
			return 0;
		}
	}

	return -1;
}

/* Remove software breakpoint */
int arch_gdb_remove_breakpoint(struct gdb_ctx *ctx, uint8_t type,
			       uintptr_t addr, uint32_t kind)
{
	ARG_UNUSED(kind);

	if (type != 0) {
		return -1;
	}

	for (int i = 0; i < GDB_MAX_BREAKPOINTS; i++) {
		if (ctx->breakpoints[i].active && ctx->breakpoints[i].addr == addr) {
			*(uint32_t *)addr = ctx->breakpoints[i].saved_insn;

			hexagon_vm_cache_idsync((uint32_t)addr, 4);

			ctx->breakpoints[i].active = false;
			return 0;
		}
	}

	return -1;
}

/* Cache sync after GDB memory writes (breakpoint patching) */
void arch_gdb_post_memory_write(uintptr_t addr, size_t len, uint8_t align)
{
	ARG_UNUSED(align);
	hexagon_vm_cache_idsync((uint32_t)addr, (uint32_t)len);
}

/**
 * Provide register descriptions for LLDB's qRegisterInfo query.
 *
 * LLDB uses this to discover register layout, sizes, and roles
 * (e.g., which register is the PC, SP, FP, return address).
 */
size_t arch_gdb_register_info(uint32_t reg_num, uint8_t *buf, size_t buflen)
{
	static const struct {
		const char *name;
		const char *generic; /* NULL if no generic role */
		uint8_t dwarf;       /* DWARF register number */
	} reg_info[] = {
		[GDB_HEXAGON_R0]  = { "r0",  "arg1", 0 },
		[GDB_HEXAGON_R1]  = { "r1",  "arg2", 1 },
		[GDB_HEXAGON_R2]  = { "r2",  "arg3", 2 },
		[GDB_HEXAGON_R3]  = { "r3",  "arg4", 3 },
		[GDB_HEXAGON_R4]  = { "r4",  "arg5", 4 },
		[GDB_HEXAGON_R5]  = { "r5",  "arg6", 5 },
		[GDB_HEXAGON_R6]  = { "r6",  NULL, 6 },
		[GDB_HEXAGON_R7]  = { "r7",  NULL, 7 },
		[GDB_HEXAGON_R8]  = { "r8",  NULL, 8 },
		[GDB_HEXAGON_R9]  = { "r9",  NULL, 9 },
		[GDB_HEXAGON_R10] = { "r10", NULL, 10 },
		[GDB_HEXAGON_R11] = { "r11", NULL, 11 },
		[GDB_HEXAGON_R12] = { "r12", NULL, 12 },
		[GDB_HEXAGON_R13] = { "r13", NULL, 13 },
		[GDB_HEXAGON_R14] = { "r14", NULL, 14 },
		[GDB_HEXAGON_R15] = { "r15", NULL, 15 },
		[GDB_HEXAGON_R16] = { "r16", NULL, 16 },
		[GDB_HEXAGON_R17] = { "r17", NULL, 17 },
		[GDB_HEXAGON_R18] = { "r18", NULL, 18 },
		[GDB_HEXAGON_R19] = { "r19", NULL, 19 },
		[GDB_HEXAGON_R20] = { "r20", NULL, 20 },
		[GDB_HEXAGON_R21] = { "r21", NULL, 21 },
		[GDB_HEXAGON_R22] = { "r22", NULL, 22 },
		[GDB_HEXAGON_R23] = { "r23", NULL, 23 },
		[GDB_HEXAGON_R24] = { "r24", NULL, 24 },
		[GDB_HEXAGON_R25] = { "r25", NULL, 25 },
		[GDB_HEXAGON_R26] = { "r26", NULL, 26 },
		[GDB_HEXAGON_R27] = { "r27", NULL, 27 },
		[GDB_HEXAGON_R28] = { "r28", NULL, 28 },
		[GDB_HEXAGON_R29] = { "sp",  "sp",  29 },
		[GDB_HEXAGON_R30] = { "fp",  "fp",  30 },
		[GDB_HEXAGON_R31] = { "lr",  "ra",  31 },
		[GDB_HEXAGON_PC]  = { "pc",  "pc",  32 },
		[GDB_HEXAGON_USR] = { "usr", NULL,   33 },
		[GDB_HEXAGON_GP]  = { "gp",  NULL,   34 },
		[GDB_HEXAGON_UGP] = { "ugp", NULL,   35 },
		[GDB_HEXAGON_LC0] = { "lc0", NULL,   36 },
		[GDB_HEXAGON_LC1] = { "lc1", NULL,   37 },
		[GDB_HEXAGON_SA0] = { "sa0", NULL,   38 },
		[GDB_HEXAGON_SA1] = { "sa1", NULL,   39 },
		[GDB_HEXAGON_P0]  = { "p3:0", "flags", 40 },
		[GDB_HEXAGON_P1]  = { "p1",  NULL,   41 },
		[GDB_HEXAGON_P2]  = { "p2",  NULL,   42 },
		[GDB_HEXAGON_P3]  = { "p3",  NULL,   43 },
	};
	const char *set_name;
	int n;

	if (reg_num >= GDB_HEXAGON_NUM_REGS) {
		return 0;
	}

	/* Determine register set name */
	if (reg_num <= GDB_HEXAGON_R31) {
		set_name = "General Purpose Registers";
	} else if (reg_num == GDB_HEXAGON_PC) {
		set_name = "General Purpose Registers";
	} else {
		set_name = "Special Registers";
	}

	if (reg_info[reg_num].generic != NULL) {
		n = snprintf((char *)buf, buflen,
			     "name:%s;bitsize:32;offset:%u;"
			     "encoding:uint;format:hex;"
			     "set:%s;gcc:%u;dwarf:%u;generic:%s;",
			     reg_info[reg_num].name,
			     reg_num * 4,
			     set_name,
			     reg_info[reg_num].dwarf,
			     reg_info[reg_num].dwarf,
			     reg_info[reg_num].generic);
	} else {
		n = snprintf((char *)buf, buflen,
			     "name:%s;bitsize:32;offset:%u;"
			     "encoding:uint;format:hex;"
			     "set:%s;gcc:%u;dwarf:%u;",
			     reg_info[reg_num].name,
			     reg_num * 4,
			     set_name,
			     reg_info[reg_num].dwarf,
			     reg_info[reg_num].dwarf);
	}

	if (n <= 0 || (size_t)n >= buflen) {
		return 0;
	}

	return (size_t)n;
}

#ifdef CONFIG_GDBSTUB_CUSTOM_BACKEND

static const struct device *gdb_uart_dev;

int z_gdb_backend_init(void)
{
	gdb_uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	if (!device_is_ready(gdb_uart_dev)) {
		return -1;
	}
	return 0;
}

void z_gdb_putchar(unsigned char ch)
{
	uart_poll_out(gdb_uart_dev, ch);
}

char z_gdb_getchar(void)
{
	unsigned char ch;

	while (uart_poll_in(gdb_uart_dev, &ch) < 0) {
	}
	return (char)ch;
}

#endif /* CONFIG_GDBSTUB_CUSTOM_BACKEND */

#endif /* CONFIG_GDBSTUB */
