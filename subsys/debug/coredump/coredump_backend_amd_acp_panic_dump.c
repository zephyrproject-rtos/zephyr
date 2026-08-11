/*
 * Copyright (c) 2024-2026 AMD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * AMD ACP panic dump coredump backend.
 *
 * Stores the Zephyr coredump together with a SOF compatible DSP "oops" record
 * in the ACP exception buffer, writes the panic code where the SOF host
 * driver expects it and raises a DSP to host interrupt so the host can collect
 * the dump.
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <zephyr/fatal.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/toolchain.h>
#include <zephyr/debug/coredump.h>
#include <zephyr/logging/log.h>
#include "coredump_internal.h"

LOG_MODULE_REGISTER(coredump_amd_acp, CONFIG_DEBUG_COREDUMP_LOG_LEVEL);

/* ACP register and exception buffer layout */
#define PU_REGISTER_BASE	(0x9FD00000 - 0x01240000)
#define PU_SCRATCH_REG_BASE	(0x9FF00000 - 0x01250000)
#define ACP_REG_OFFSET	0x1250000
#define ACP_SW_INTR_TRIG	0x1241890

/* Mailbox buffer sizes */
#define SRAM_OUTBOX_SIZE	0x400
#define SRAM_INBOX_SIZE		0x400
#define SRAM_DEBUG_SIZE		0x400
#define SRAM_EXCEPT_SIZE	0x400
#define SRAM_STREAM_SIZE	0x400
#define SRAM_TRACE_SIZE		0x400

/*
 * Host-side (Linux SOF AMD driver) mailbox layout. The driver reads the panic
 * status at debug_box.offset and the DSP oops at dsp_box.offset + sizeof(u32),
 * which differ from the firmware MAILBOX_* bases; the panic dump targets these
 * host offsets directly so the kernel can decode the crash.
 */
#define ACP_DRV_DEBUG_BOX_OFFSET	0x400
#define ACP_DRV_OOPS_OFFSET	0x4
#define EXCEPT_BUFFER_OFFSET	(SRAM_OUTBOX_SIZE + SRAM_INBOX_SIZE + SRAM_DEBUG_SIZE)
#define DEBUG_BOX_OFFSET	(SRAM_OUTBOX_SIZE + SRAM_INBOX_SIZE)

/*
 * SOF IPC panic codes. These values must
 * match what the SOF host driver expects.
 */
#define SOF_IPC_PANIC_MAGIC		0x0dead000
#define SOF_IPC_PANIC_MAGIC_MASK	0x0ffff000
#define SOF_IPC_PANIC_CODE_MASK		0x00000fff

#define SOF_IPC_PANIC_MEM		(SOF_IPC_PANIC_MAGIC | 0x0)
#define SOF_IPC_PANIC_WORK		(SOF_IPC_PANIC_MAGIC | 0x1)
#define SOF_IPC_PANIC_IPC		(SOF_IPC_PANIC_MAGIC | 0x2)
#define SOF_IPC_PANIC_ARCH		(SOF_IPC_PANIC_MAGIC | 0x3)
#define SOF_IPC_PANIC_PLATFORM		(SOF_IPC_PANIC_MAGIC | 0x4)
#define SOF_IPC_PANIC_TASK		(SOF_IPC_PANIC_MAGIC | 0x5)
#define SOF_IPC_PANIC_EXCEPTION		(SOF_IPC_PANIC_MAGIC | 0x6)
#define SOF_IPC_PANIC_DEADLOCK		(SOF_IPC_PANIC_MAGIC | 0x7)
#define SOF_IPC_PANIC_STACK		(SOF_IPC_PANIC_MAGIC | 0x8)
#define SOF_IPC_PANIC_IDLE		(SOF_IPC_PANIC_MAGIC | 0x9)
#define SOF_IPC_PANIC_WFI		(SOF_IPC_PANIC_MAGIC | 0xa)
#define SOF_IPC_PANIC_ASSERT		(SOF_IPC_PANIC_MAGIC | 0xb)

/* Computed exception buffer addresses */
#define ACP_EXCEPT_BUFFER_BASE \
	(PU_SCRATCH_REG_BASE + ACP_REG_OFFSET + EXCEPT_BUFFER_OFFSET)
#define ACP_EXCEPT_BUFFER_SIZE	SRAM_EXCEPT_SIZE
#define ACP_SCRATCH_MEM_BASE	(PU_SCRATCH_REG_BASE + ACP_REG_OFFSET)
#define ACP_DEBUG_BOX_BASE \
	(PU_SCRATCH_REG_BASE + ACP_REG_OFFSET + DEBUG_BOX_OFFSET)

#define ARCHITECTURE_ID_XTENSA	0x1
#define SOF_STACK_SIZE		0x300	/* stack bytes captured in the dump */
#define XTENSA_NUM_AREGS	64

/* SOF DSP oops record parsed by the host driver. */
struct sof_ipc_dsp_oops_arch_hdr {
	uint32_t arch;
	uint32_t totalsize;
} __packed __aligned(4);

struct sof_ipc_dsp_oops_plat_hdr {
	uint32_t configidhi;
	uint32_t configidlo;
	uint32_t numaregs;
	uint32_t stackoffset;
	uint32_t stackptr;
} __packed __aligned(4);

struct sof_ipc_dsp_oops_xtensa {
	struct sof_ipc_dsp_oops_arch_hdr arch_hdr;
	struct sof_ipc_dsp_oops_plat_hdr plat_hdr;
	uint32_t exccause;
	uint32_t excvaddr;
	uint32_t ps;
	uint32_t epc1;
	uint32_t epc2;
	uint32_t epc3;
	uint32_t epc4;
	uint32_t epc5;
	uint32_t epc6;
	uint32_t epc7;
	uint32_t eps2;
	uint32_t eps3;
	uint32_t eps4;
	uint32_t eps5;
	uint32_t eps6;
	uint32_t eps7;
	uint32_t depc;
	uint32_t intenable;
	uint32_t interrupt;
	uint32_t sar;
	uint32_t debugcause;
	uint32_t windowbase;
	uint32_t windowstart;
	uint32_t excsave1;
	uint32_t ar[XTENSA_NUM_AREGS];
} __packed __aligned(4);

#define ARCH_OOPS_SIZE	sizeof(struct sof_ipc_dsp_oops_xtensa)

#define SOF_TRACE_FILENAME_SIZE	32

/* SOF IPC header and panic info record (mirror sof ipc/header.h, ipc/trace.h).
 * The host driver reads this record, right after the oops, for the panic code,
 * source file and line number.
 */
struct sof_ipc_hdr {
	uint32_t size;
	uint32_t cmd;
} __packed;

struct sof_ipc_panic_info {
	struct sof_ipc_hdr hdr;
	uint32_t code;
	char filename[SOF_TRACE_FILENAME_SIZE];
	uint32_t linenum;
} __packed __aligned(4);

/* Stack dump offset in the exception buffer: oops record then panic info. */
#define ACP_STACK_OFFSET	(ARCH_OOPS_SIZE + sizeof(struct sof_ipc_panic_info))

/* Xtensa arch block emitted by arch/xtensa/core/coredump.c. */
struct xtensa_arch_block {
	uint8_t soc;
	uint16_t version;
	uint8_t toolchain;
	struct {
		uint32_t pc;
		uint32_t exccause;
		uint32_t excvaddr;
		uint32_t sar;
		uint32_t ps;
		uint32_t a0;
		uint32_t a1;	/* original exception stack pointer */
		uint32_t a2;
		uint32_t a3;
		uint32_t a4;
		uint32_t a5;
		uint32_t a6;
		uint32_t a7;
		uint32_t a8;
		uint32_t a9;
		uint32_t a10;
		uint32_t a11;
		uint32_t a12;
		uint32_t a13;
		uint32_t a14;
		uint32_t a15;
	} r;
} __packed;

/* ACP scratch memory layout, mirrors the SOF fw_scratch_mem definition. */
struct acp_scratch_mem_config {
	uint8_t out_box[SRAM_OUTBOX_SIZE];
	uint8_t in_box[SRAM_INBOX_SIZE];
	uint8_t debug_box[SRAM_DEBUG_SIZE];
	uint8_t except_box[SRAM_EXCEPT_SIZE];
	uint8_t stream_box[SRAM_STREAM_SIZE];
	uint8_t trace_box[SRAM_TRACE_SIZE];
	uint32_t acp_host_msg_write;
	uint32_t acp_host_ack_write;
	uint32_t acp_dsp_msg_write;	/* panic code read by the host */
	uint32_t acp_dsp_ack_write;
} __packed __aligned(4);

union acp_sw_intr_trig {
	struct {
		uint32_t reserved0 : 1;
		uint32_t trig_dsp0_to_host_intr : 1;
		uint32_t reserved : 30;
	} bits;
	uint32_t u32all;
};

static int error;
static uint32_t mem_wptr;
static bool header_processed;
static bool arch_block_processed;
static uint32_t exception_sp;
static uint32_t saved_panic_code;

static inline uint32_t acp_reg_read(uint32_t offset)
{
	return sys_read32(PU_REGISTER_BASE + offset);
}

static inline void acp_reg_write(uint32_t offset, uint32_t value)
{
	sys_write32(value, PU_REGISTER_BASE + offset);
}

/* Read an Xtensa special register by name. */
#define ACP_RSR(sr)						\
	({							\
		uint32_t reg;					\
		__asm__ volatile("rsr." #sr " %0" : "=r"(reg));	\
		reg;						\
	})

static uint32_t build_panic_code(unsigned int reason)
{
	uint32_t exccause;

	switch (reason) {
	case K_ERR_STACK_CHK_FAIL:
		return SOF_IPC_PANIC_STACK;
	case K_ERR_KERNEL_OOPS:
	case K_ERR_KERNEL_PANIC:
	case K_ERR_SPURIOUS_IRQ:
		return SOF_IPC_PANIC_EXCEPTION;
	case K_ERR_CPU_EXCEPTION:
		exccause = ACP_RSR(exccause);
		LOG_DBG("reason %u exccause 0x%x", reason, exccause);

		switch (exccause) {
		case 2:		/* instruction fetch error */
		case 3:		/* load/store error */
		case 9:		/* unaligned load/store */
		case 12:	/* PIF data error on instruction fetch */
		case 13:	/* PIF data error on load/store */
		case 14:	/* PIF address error on instruction fetch */
		case 15:	/* PIF address error on load/store */
		case 16:	/* ITLB miss */
		case 17:	/* ITLB multihit */
		case 18:	/* instruction ring privilege violation */
		case 20:	/* instruction fetch prohibited */
		case 24:	/* DTLB miss */
		case 25:	/* DTLB multihit */
		case 26:	/* load/store ring privilege violation */
		case 28:	/* load prohibited */
		case 29:	/* store prohibited */
			return SOF_IPC_PANIC_MEM;
		case 5:		/* alloca (MOVSP) window extension assist */
			return SOF_IPC_PANIC_EXCEPTION;
		case 6:		/* integer divide by zero */
		case 8:		/* privileged instruction */
		case 32:	/* coprocessor 0..7 disabled */
		case 33:
		case 34:
		case 35:
		case 36:
		case 37:
		case 38:
		case 39:
			return SOF_IPC_PANIC_ARCH;
		default:
			return SOF_IPC_PANIC_EXCEPTION;
		}
	default:
		return SOF_IPC_PANIC_EXCEPTION;
	}
}

static void acp_trigger_host_interrupt(void)
{
	union acp_sw_intr_trig sw_intr_trig;

	sw_intr_trig.u32all = acp_reg_read(ACP_SW_INTR_TRIG);
	sw_intr_trig.bits.trig_dsp0_to_host_intr = 1;
	acp_reg_write(ACP_SW_INTR_TRIG, sw_intr_trig.u32all);
}

/* Host driver polls debug_box[0] for the panic magic. */
static void acp_write_panic_to_debug_box(uint32_t panic_code)
{
	sys_write32(panic_code, ACP_DEBUG_BOX_BASE);
	/* Pre-FW_READY the host reads the panic status from dsp_box (offset 0). */
	sys_write32(panic_code, ACP_SCRATCH_MEM_BASE);
}

/* acp_dsp_msg_write is the mailbox field the host reads to detect a panic. */
static void acp_write_panic_code(uint32_t panic_code)
{
	sys_write32(panic_code,
		    ACP_SCRATCH_MEM_BASE +
		    offsetof(struct acp_scratch_mem_config, acp_dsp_msg_write));
}

/* Write the SOF oops record and a stack snapshot into the exception buffer. */
static void write_sof_oops_to_exception_buffer(uint32_t panic_code)
{
	volatile struct sof_ipc_dsp_oops_xtensa *oops =
		(volatile struct sof_ipc_dsp_oops_xtensa *)ACP_EXCEPT_BUFFER_BASE;
	volatile struct sof_ipc_panic_info *pinfo =
		(volatile struct sof_ipc_panic_info *)(ACP_EXCEPT_BUFFER_BASE + ARCH_OOPS_SIZE);
	static const char panic_name[] = "zephyr_coredump";
	volatile uint32_t *stack_dst;
	uint32_t *stack_src;
	uint32_t stack_ptr;
	size_t nwords;
	int i;

	__asm__ volatile("mov %0, a1" : "=r"(stack_ptr));

	oops->arch_hdr.arch = ARCHITECTURE_ID_XTENSA;
	oops->arch_hdr.totalsize = sizeof(struct sof_ipc_dsp_oops_xtensa);

	oops->plat_hdr.configidhi = 0;
	oops->plat_hdr.configidlo = 0;
	oops->plat_hdr.numaregs = XTENSA_NUM_AREGS;
	oops->plat_hdr.stackoffset = ACP_STACK_OFFSET;
	oops->plat_hdr.stackptr = stack_ptr;

	oops->exccause = ACP_RSR(exccause);
	oops->excvaddr = ACP_RSR(excvaddr);
	oops->ps = ACP_RSR(ps);
	oops->epc1 = ACP_RSR(epc1);
	oops->epc2 = ACP_RSR(epc2);
	oops->epc3 = ACP_RSR(epc3);
	oops->epc4 = ACP_RSR(epc4);
	oops->epc5 = ACP_RSR(epc5);
	oops->epc6 = ACP_RSR(epc6);
	oops->epc7 = ACP_RSR(epc7);
	oops->eps2 = ACP_RSR(eps2);
	oops->eps3 = ACP_RSR(eps3);
	oops->eps4 = ACP_RSR(eps4);
	oops->eps5 = ACP_RSR(eps5);
	oops->eps6 = ACP_RSR(eps6);
	oops->eps7 = ACP_RSR(eps7);
	oops->depc = ACP_RSR(depc);
	oops->intenable = ACP_RSR(intenable);
	oops->interrupt = ACP_RSR(interrupt);
	oops->sar = ACP_RSR(sar);
	oops->debugcause = ACP_RSR(debugcause);
	oops->windowbase = ACP_RSR(windowbase);
	oops->windowstart = ACP_RSR(windowstart);
	oops->excsave1 = ACP_RSR(excsave1);

	for (i = 0; i < XTENSA_NUM_AREGS; i++) {
		oops->ar[i] = 0;
	}

	pinfo->hdr.size = sizeof(struct sof_ipc_panic_info);
	pinfo->hdr.cmd = 0;
	pinfo->code = panic_code;
	pinfo->linenum = 0;
	for (i = 0; i < (int)sizeof(pinfo->filename); i++) {
		pinfo->filename[i] = (i < (int)sizeof(panic_name)) ? panic_name[i] : 0;
	}

	stack_dst = (volatile uint32_t *)(ACP_EXCEPT_BUFFER_BASE + ACP_STACK_OFFSET);
	stack_src = (uint32_t *)stack_ptr;
	nwords = MIN(SOF_STACK_SIZE, ACP_EXCEPT_BUFFER_SIZE - ACP_STACK_OFFSET) / 4;

	for (i = 0; i < (int)nwords; i++) {
		stack_dst[i] = stack_src[i];
	}

	LOG_DBG("oops written: exccause 0x%x excvaddr 0x%x epc1 0x%x",
		oops->exccause, oops->excvaddr, oops->epc1);
}

/* Re-copy the stack using the original exception stack pointer. */
static void update_stack_from_exception_context(uint32_t orig_sp)
{
	volatile struct sof_ipc_dsp_oops_xtensa *oops =
		(volatile struct sof_ipc_dsp_oops_xtensa *)ACP_EXCEPT_BUFFER_BASE;
	volatile uint32_t *stack_dst;
	uint32_t *stack_src;
	size_t nwords;

	if (orig_sp == 0) {
		return;
	}

	oops->plat_hdr.stackptr = orig_sp;

	stack_dst = (volatile uint32_t *)(ACP_EXCEPT_BUFFER_BASE + ACP_STACK_OFFSET);
	stack_src = (uint32_t *)orig_sp;
	nwords = MIN(SOF_STACK_SIZE, ACP_EXCEPT_BUFFER_SIZE - ACP_STACK_OFFSET) / 4;

	for (int i = 0; i < (int)nwords; i++) {
		stack_dst[i] = stack_src[i];
	}

	LOG_DBG("stack updated from exception sp 0x%x", orig_sp);
}

static void coredump_acp_backend_start(void)
{
	error = 0;
	mem_wptr = 0;
	header_processed = false;
	arch_block_processed = false;
	exception_sp = 0;

	LOG_DBG("start: exccause 0x%x excvaddr 0x%x epc1 0x%x ps 0x%x",
		ACP_RSR(exccause), ACP_RSR(excvaddr),
		ACP_RSR(epc1), ACP_RSR(ps));
}

static void coredump_acp_backend_end(void)
{
	if (error != 0) {
		LOG_ERR("coredump error %d", error);
	}

	LOG_DBG("end: %u bytes written", mem_wptr);

	/* Publish the panic flag, then notify the host (data-before-flag). */
	acp_write_panic_code(saved_panic_code);
	acp_write_panic_to_debug_box(saved_panic_code);
	acp_trigger_host_interrupt();
}

static void coredump_acp_backend_buffer_output(uint8_t *buf, size_t buflen)
{
	uint8_t *except_buffer;
	uint8_t *coredump_data = buf;
	size_t data_left;

	if (buf == NULL) {
		LOG_ERR("NULL buffer");
		error = -EINVAL;
		return;
	}

	/*
	 * The first buffer_output call carries the coredump header. Extract the
	 * reason, build the panic code and lay down the SOF oops record.
	 */
	if (!header_processed && buflen >= sizeof(struct coredump_hdr_t)) {
		struct coredump_hdr_t *hdr = (struct coredump_hdr_t *)buf;
		uint32_t panic_code;

		if (hdr->id[0] == 'Z' && hdr->id[1] == 'E') {
			unsigned int reason = hdr->reason;

			panic_code = build_panic_code(reason);
			saved_panic_code = panic_code;
			LOG_DBG("header reason %u panic_code 0x%x", reason, panic_code);

			header_processed = true;

			write_sof_oops_to_exception_buffer(panic_code);

			/*
			 * The exception buffer now holds the SOF panic layout
			 * (oops, panic info, stack) like other SOF platforms, so
			 * do not overlay the raw coredump stream on top of it.
			 */
			mem_wptr = ACP_EXCEPT_BUFFER_SIZE;
		}
	}

	/*
	 * The arch block carries the original exception context. Use its saved
	 * EXCCAUSE and stack pointer, since the live EXCCAUSE register may have
	 * been overwritten by an ALLOCA (5) during exception handling.
	 */
	if (header_processed && !arch_block_processed &&
	    buflen >= sizeof(struct xtensa_arch_block)) {
		struct xtensa_arch_block *arch_blk = (struct xtensa_arch_block *)buf;

		if (arch_blk->version == 1 || arch_blk->version == 2) {
			volatile struct sof_ipc_dsp_oops_xtensa *oops =
				(volatile struct sof_ipc_dsp_oops_xtensa *)
				ACP_EXCEPT_BUFFER_BASE;

			/*
			 * The arch block holds the exception-time context. Use it for
			 * the register dump; the live special registers may have changed
			 * during coredump handling (e.g. ALLOCA overwriting exccause).
			 */
			LOG_DBG("fixup regs from arch block: pc 0x%x exccause 0x%x",
				arch_blk->r.pc, arch_blk->r.exccause);
			oops->exccause = arch_blk->r.exccause;
			oops->excvaddr = arch_blk->r.excvaddr;
			oops->ps = arch_blk->r.ps;
			oops->sar = arch_blk->r.sar;
			oops->epc1 = arch_blk->r.pc;
			oops->ar[0] = arch_blk->r.a0;
			oops->ar[1] = arch_blk->r.a1;
			oops->ar[2] = arch_blk->r.a2;
			oops->ar[3] = arch_blk->r.a3;
			oops->ar[4] = arch_blk->r.a4;
			oops->ar[5] = arch_blk->r.a5;
			oops->ar[6] = arch_blk->r.a6;
			oops->ar[7] = arch_blk->r.a7;
			oops->ar[8] = arch_blk->r.a8;
			oops->ar[9] = arch_blk->r.a9;
			oops->ar[10] = arch_blk->r.a10;
			oops->ar[11] = arch_blk->r.a11;
			oops->ar[12] = arch_blk->r.a12;
			oops->ar[13] = arch_blk->r.a13;
			oops->ar[14] = arch_blk->r.a14;
			oops->ar[15] = arch_blk->r.a15;

			exception_sp = arch_blk->r.a1;
			if (exception_sp != 0) {
				update_stack_from_exception_context(exception_sp);
			}

			arch_block_processed = true;
		}
	}

	if (mem_wptr >= ACP_EXCEPT_BUFFER_SIZE) {
		return;
	}

	except_buffer = (uint8_t *)(ACP_EXCEPT_BUFFER_BASE + mem_wptr);

	for (data_left = buflen; data_left > 0; data_left--) {
		if (mem_wptr >= ACP_EXCEPT_BUFFER_SIZE) {
			LOG_WRN("exception buffer full, dump truncated");
			break;
		}
		*except_buffer = *coredump_data;
		except_buffer++;
		coredump_data++;
		mem_wptr++;
	}
}

static int coredump_acp_backend_query(enum coredump_query_id query_id, void *arg)
{
	int ret;

	ARG_UNUSED(arg);

	if (query_id == COREDUMP_QUERY_GET_ERROR) {
		ret = error;
	} else {
		ret = -ENOTSUP;
	}

	return ret;
}

static int coredump_acp_backend_cmd(enum coredump_cmd_id cmd_id, void *arg)
{
	int ret;

	ARG_UNUSED(arg);

	if (cmd_id == COREDUMP_CMD_CLEAR_ERROR) {
		error = 0;
		ret = 0;
	} else {
		ret = -ENOTSUP;
	}

	return ret;
}

struct coredump_backend_api coredump_backend_amd_acp_panic_dump = {
	.start = coredump_acp_backend_start,
	.end = coredump_acp_backend_end,
	.buffer_output = coredump_acp_backend_buffer_output,
	.query = coredump_acp_backend_query,
	.cmd = coredump_acp_backend_cmd,
};
