/*
 * Copyright (c) 2015-2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * x86 part of the GDB server
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <string.h>
#include <debug/gdb_arch.h>
#include <debug/gdb_server.h>

#define TRACE_FLAG	0x0100 /* EFLAGS:TF */
#define INT_FLAG	0x0200 /* EFLAGS:IF */

#define INSTRUCTION_HLT 0xf4
#define INSTRUCTION_STI 0xfb
#define INSTRUCTION_CLI 0xfa

#ifdef GDB_ARCH_HAS_RUNCONTROL
#ifdef GDB_ARCH_HAS_HW_BP
static int gdb_hw_bp_find(struct gdb_debug_regs *regs,
			  enum gdb_bp_type *bp_type,
			  long *address);
#endif
#endif

/**
 * @brief Initialize GDB server architecture part
 *
 * This routine initializes the architecture part of the GDB server.
 *
 * Does nothing currently.
 *
 * @return N/A
 */

void gdb_arch_init(void)
{
	/* currently empty */
}

/**
 * @brief Fill a GDB register set from a given ESF register set
 *
 * This routine fills the provided GDB register set with values from given
 * NANO_ESF register set.
 *
 * @param regs Destination GDB register set to fill
 * @param esf Source exception stack frame
 *
 * @return N/A
 */

void gdb_arch_regs_from_esf(struct gdb_reg_set *regs, NANO_ESF *esf)
{
	regs->regs.eax = esf->eax;
	regs->regs.ecx = esf->ecx;
	regs->regs.edx = esf->edx;
	regs->regs.ebx = esf->ebx;
	regs->regs.esp = esf->esp;
	regs->regs.ebp = esf->ebp;
	regs->regs.esi = esf->esi;
	regs->regs.edi = esf->edi;
	regs->regs.eip = esf->eip;
	regs->regs.eflags = esf->eflags;
	regs->regs.cs = esf->cs;
}

/**
 * @brief Fill a GDB register set from a given ISF register set
 *
 * This routine fills the provided GDB register set with values from given
 * NANO_ISF register set.
 *
 * @param regs Destination GDB register set to fill
 * @param isf Source interrupt stack frame
 *
 * @return N/A
 */

void gdb_arch_regs_from_isf(struct gdb_reg_set *regs, NANO_ISF *isf)
{
	memcpy(&regs->regs, isf, sizeof(regs->regs));
}

/**
 * @brief Fill an ESF register set from a given GDB register set
 *
 * This routine fills the provided NANO_ESF register set with values
 * from given GDB register set.
 *
 * @param regs Source GDB register set
 * @param esf Destination exception stack frame to fill
 *
 * @return N/A
 */

void gdb_arch_regs_to_esf(struct gdb_reg_set *regs, NANO_ESF *esf)
{
	esf->eax = regs->regs.eax;
	esf->ecx = regs->regs.ecx;
	esf->edx = regs->regs.edx;
	esf->ebx = regs->regs.ebx;
	esf->esp = regs->regs.esp;
	esf->ebp = regs->regs.ebp;
	esf->esi = regs->regs.esi;
	esf->edi = regs->regs.edi;
	esf->eip = regs->regs.eip;
	esf->eflags = regs->regs.eflags;
	esf->cs = regs->regs.cs;
}

/**
 * @brief Fill an ISF register set from a given GDB register set
 *
 * This routine fills the provided NANO_ISF register set with values
 * from given GDB register set.
 *
 * @param regs Source GDB register set
 * @param isf Destination interrupt stack frame to fill
 *
 * @return N/A
 */

void gdb_arch_regs_to_isf(struct gdb_reg_set *regs, NANO_ISF *isf)
{
	memcpy(isf, &regs->regs, sizeof(NANO_ISF));
}

/**
 * @brief Fill given buffer from given register set
 *
 * This routine fills the provided buffer with values from given register set.
 *
 * The provided buffer must be large enough to store all register values.
 * It is up to the caller to do this check.
 *
 * @param regs Source GDB register set
 * @param esf Destination buffer to fill
 *
 * @return N/A
 */

void gdb_arch_regs_get(struct gdb_reg_set *regs, char *buffer)
{
	*((uint32_t *) buffer) = regs->regs.eax;
	buffer += 4;
	*((uint32_t *) buffer) = regs->regs.ecx;
	buffer += 4;
	*((uint32_t *) buffer) = regs->regs.edx;
	buffer += 4;
	*((uint32_t *) buffer) = regs->regs.ebx;
	buffer += 4;
	*((uint32_t *) buffer) = regs->regs.esp;
	buffer += 4;
	*((uint32_t *) buffer) = regs->regs.ebp;
	buffer += 4;
	*((uint32_t *) buffer) = regs->regs.esi;
	buffer += 4;
	*((uint32_t *) buffer) = regs->regs.edi;
	buffer += 4;
	*((uint32_t *) buffer) = (uint32_t) regs->regs.eip;
	buffer += 4;
	*((uint32_t *) buffer) = regs->regs.eflags;
	buffer += 4;
	*((uint32_t *) buffer) = regs->regs.cs;
}

/**
 * @brief Write given registers buffer to GDB register set
 *
 * This routine fills given register set with value from provided buffer.
 * The provided buffer must be large enough to contain all register values.
 * It is up to the caller to do this check.
 *
 * @param regs Destination GDB register set to fill
 * @param esf Source buffer
 *
 * @return N/A
 */

void gdb_arch_regs_set(struct gdb_reg_set *regs, char *buffer)
{
	regs->regs.eax = *((uint32_t *)buffer);
	buffer += 4;
	regs->regs.ecx = *((uint32_t *)buffer);
	buffer += 4;
	regs->regs.edx = *((uint32_t *)buffer);
	buffer += 4;
	regs->regs.ebx = *((uint32_t *)buffer);
	buffer += 4;
	regs->regs.esp = *((uint32_t *)buffer);
	buffer += 4;
	regs->regs.ebp = *((uint32_t *)buffer);
	buffer += 4;
	regs->regs.esi = *((uint32_t *)buffer);
	buffer += 4;
	regs->regs.edi = *((uint32_t *)buffer);
	buffer += 4;
	regs->regs.eip = *((uint32_t *)buffer);
	buffer += 4;
	regs->regs.eflags = *((uint32_t *)buffer);
	buffer += 4;
	regs->regs.cs = *((uint32_t *)buffer);
}

/**
 * @brief Get size and offset of given register
 *
 * This routine returns size and offset of given register.
 *
 * @param reg_id Register identifier
 * @param size Container to return size of register, in bytes
 * @param offset Container to return offset of register, in bytes
 *
 * @return N/A
 */

void gdb_arch_reg_info_get(int reg_id, int *size, int *offset)
{
	/* Determine register size and offset */
	if (reg_id >= 0 && reg_id < GDB_NUM_REGS) {
		*size = 4;
		*offset = 4 * reg_id;
	}
}

#ifdef GDB_ARCH_HAS_RUNCONTROL
#ifdef GDB_ARCH_HAS_HW_BP
/**
 * @brief Get the HW breakpoint architecture type for a common GDB type
 *
 * This routine gets the specific architecture value that corresponds to a
 * common hardware breakpoint type.
 *
 * The values accepted for the @a type are GDB_HW_INST_BP,
 * GDB_HW_DATA_WRITE_BP, GDB_HW_DATA_ACCESS_BP and GDB_HW_DATA_READ_BP.
 *
 * @param type Common GDB breakpoint type
 * @param len Data length
 * @param err Error code on failure (return value of -1)
 *
 * @return The architecture type, -1 on failure
 */

static char gdb_hw_bp_type_get(enum gdb_bp_type type, int len,
				enum gdb_error_code *err)
{
	char hw_type = -1;

	switch (type) {
		/* Following combinations are supported on IA */
	case GDB_HW_INST_BP:
		hw_type = 0;
		break;
	case GDB_HW_DATA_WRITE_BP:
		if (len == 1) {
			hw_type = 0x1;
		} else if (len == 2) {
			hw_type = 0x5;
		} else if (len == 4) {
			hw_type = 0xd;
		} else if (len == 8) {
			hw_type = 0x9;
		}
		break;
	case GDB_HW_DATA_ACCESS_BP:
		if (len == 1) {
			hw_type = 0x3;
		} else if (len == 2) {
			hw_type = 0x7;
		} else if (len == 4) {
			hw_type = 0xf;
		} else if (len == 8) {
			hw_type = 0xb;
		}
		break;
	case GDB_HW_DATA_READ_BP:
		/* Data read not supported on IA */
		/*
		 * NOTE: Read only watchpoints are not supported by IA debug
		 * registers, but it could be possible to use RW watchpoints
		 * and ignore the RW watchpoint if it has been hit by a write
		 * operation.
		 */
		*err = GDB_ERROR_HW_BP_NOT_SUP;
		return -1;
	default:
		/* Unknown type */
		*err = GDB_ERROR_HW_BP_INVALID_TYPE;
		return -1;
	}
	return hw_type;
}

/**
 * @brief Set the debug registers for a specific HW BP.
 *
 * This routine sets the @a regs debug registers according to the HW breakpoint
 * description.
 *
 * @param regs Debug registers to set
 * @param addr Address of the breakpoint
 * @param type Common GDB breakpoint type
 * @param len Data length
 * @param err Error code on failure (return value of -1)
 *
 * @return 0 if debug registers have been modified, -1 on error
 */

int gdb_hw_bp_set(struct gdb_debug_regs *regs, long addr,
		  enum gdb_bp_type type,
		  int len, enum gdb_error_code *err)
{
	char hw_type;

	hw_type = gdb_hw_bp_type_get(type, len, err);
	if (hw_type < 0) {
		return -1;
	}

	if (regs->db0 == 0) {
		regs->db0 = addr;
		regs->db7 |= (hw_type << 16) | 0x02;
	} else if (regs->db1 == 0) {
		regs->db1 = addr;
		regs->db7 |= (hw_type << 20) | 0x08;
	} else if (regs->db2 == 0) {
		regs->db2 = addr;
		regs->db7 |= (hw_type << 24) | 0x20;
	} else if (regs->db3 == 0) {
		regs->db3 = addr;
		regs->db7 |= (hw_type << 28) | 0x80;
	} else {
		*err = GDB_ERROR_HW_BP_DBG_REGS_FULL;
		return -1;
	}

	/* set GE bit if it is data breakpoint */
	if (hw_type != 0) {
		regs->db7 |= 0x200;
	}
	return 0;
}

/**
 * @brief Clear the debug registers for a specific HW BP.
 *
 * This routine updates the @a regs debug registers to remove a HW breakpoint.
 *
 * @param regs Debug registers to clear
 * @param addr Address of the breakpoint
 * @param type Common GDB breakpoint type
 * @param len Data length
 * @param err Error code on failure (return value of -1)
 *
 * @return 0 if debug registers have been modified, -1 on error
 */

int gdb_hw_bp_clear(struct gdb_debug_regs *regs, long addr,
		    enum gdb_bp_type type, int len,
		    enum gdb_error_code *err)
{
	char hw_type;

	hw_type = gdb_hw_bp_type_get(type, len, err);
	if (hw_type < 0) {
		return -1;
	}

	if ((regs->db0 == addr) && (((regs->db7 >> 16) & 0xf) == hw_type)) {
		regs->db0 = 0;
		regs->db7 &= ~((hw_type << 16) | 0x02);
	} else if ((regs->db1 == addr)
		   && (((regs->db7 >> 20) & 0xf) == hw_type)) {
		regs->db1 = 0;
		regs->db7 &= ~((hw_type << 20) | 0x08);
	} else if ((regs->db2 == addr)
		   && (((regs->db7 >> 24) & 0xf) == hw_type)) {
		regs->db2 = 0;
		regs->db7 &= ~((hw_type << 24) | 0x20);
	} else if ((regs->db3 == addr)
		   && (((regs->db7 >> 28) & 0xf) == hw_type)) {
		regs->db3 = 0;
		regs->db7 &= ~((hw_type << 28) | 0x80);
	} else {
		/* Unknown breakpoint */
		*err = GDB_ERROR_INVALID_BP;
		return -1;
	}
	return 0;
}

/**
 * @brief Look for a Hardware breakpoint
 *
 * This routine checks from the @a regs debug register set if a hardware
 * breakpoint has been hit.
 *
 * @param regs Debug registers to check
 * @param bp_type Common GDB breakpoint type
 * @param address Address of the breakpoint
 *
 * @return 0 if a HW BP has been found, -1 otherwise
 */

static int gdb_hw_bp_find(struct gdb_debug_regs *regs,
			  enum gdb_bp_type *bp_type,
			  long *address)
{
	int ix;
	unsigned char type = 0;
	long addr = 0;
	int status_bit;
	int enable_bit;

	/* get address and type of breakpoint from DR6 and DR7 */
	for (ix = 0; ix < 4; ix++) {
		status_bit = 1 << ix;
		enable_bit = 2 << (ix << 1);

		if ((regs->db6 & status_bit) && (regs->db7 & enable_bit)) {
			switch (ix) {
			case 0:
				addr = regs->db0;
				type = (regs->db7 & 0x000f0000) >> 16;
				break;

			case 1:
				addr = regs->db1;
				type = (regs->db7 & 0x00f00000) >> 20;
				break;

			case 2:
				addr = regs->db2;
				type = (regs->db7 & 0x0f000000) >> 24;
				break;

			case 3:
				addr = regs->db3;
				type = (regs->db7 & 0xf0000000) >> 28;
				break;
			}
		}
	}

	if ((addr == 0) && (type == 0))
		return -1;

	*address = addr;
	switch (type) {
	case 0x1:
	case 0x5:
	case 0xd:
	case 0x9:
		*bp_type = GDB_HW_DATA_WRITE_BP;
		break;
	case 0x3:
	case 0x7:
	case 0xf:
	case 0xb:
		*bp_type = GDB_HW_DATA_ACCESS_BP;
		break;
	default:
		*bp_type = GDB_HW_INST_BP;
		break;
	}
	return 0;
}

/**
 * @brief Clear all debug registers.
 *
 * This routine clears all debug registers
 *
 * @return N/A
 */

void gdb_dbg_regs_clear(void)
{
	struct gdb_debug_regs regs;

	regs.db0 = 0;
	regs.db1 = 0;
	regs.db2 = 0;
	regs.db3 = 0;
	regs.db6 = 0;
	regs.db7 = 0;
	gdb_dbg_regs_set(&regs);
}
#endif /* GDB_ARCH_HAS_HW_BP */

/**
 * @brief Clear trace mode
 *
 * This routine makes CPU trace-disabled.
 *
 * @param regs GDB register set to modify.
 * @param arg Interrupt locking key
 *
 * @return N/A
 */

void gdb_trace_mode_clear(struct gdb_reg_set *regs, int arg)
{
	regs->regs.eflags &= ~INT_FLAG;
	regs->regs.eflags |= (arg & INT_FLAG);
	regs->regs.eflags &= ~TRACE_FLAG;
}

/**
 * @brief Test if single stepping is possible for current program counter
 *
 * @param regs GDB register set to fetch PC from.
 *
 * @return 1 if it is possible to step the instruction, 0 otherwise
 */

int gdb_arch_can_step(struct gdb_reg_set *regs)
{
	unsigned char *pc = (unsigned char *)regs->regs.eip;

	if (*pc == INSTRUCTION_HLT) {
		return 0;
	}

	return 1;
}

/**
 * @brief Set trace mode
 *
 * This routine makes CPU trace-enabled.
 *
 * In the event that the program counter currently points to a sti or a cli
 * instruction, the returned eflags will contain an IF bit as if that
 * instruction had executed (set for sti, cleared for cli).
 *
 * @param regs GDB register set to modify.
 *
 * @return eflags with IF bit possibly modified by current sti/cli instruction.
 */

int gdb_trace_mode_set(struct gdb_reg_set *regs)
{
	unsigned char *pc = (unsigned char *)regs->regs.eip;
	int simulated_eflags = regs->regs.eflags;

	if (*pc == INSTRUCTION_STI) {
		simulated_eflags |= INT_FLAG;
	}

	if (*pc == INSTRUCTION_CLI) {
		simulated_eflags &= ~INT_FLAG;
	}

	regs->regs.eflags &= ~INT_FLAG;
	regs->regs.eflags |= TRACE_FLAG;

	return simulated_eflags;
}

#ifdef GDB_ARCH_HAS_HW_BP
/**
 * @brief Implementation of GDB trace handler for HW breakpoint support
 *
 * @param esf Exception stack frame when taking the exception
 *
 * @return N/A
 */

static void _do_gdb_trace_handler(NANO_ESF *esf)
{
	struct gdb_debug_regs regs;

	gdb_dbg_regs_get(&regs);
	if ((regs.db6 & 0x00004000) == 0x00004000) {
		gdb_handler(GDB_EXC_TRACE, esf, GDB_SIG_TRAP);
	} else {
		int type;
		long addr;

		gdb_dbg_regs_clear();
		(void)gdb_hw_bp_find(&regs, &type, &addr);
		gdb_cpu_stop_hw_bp_addr = addr;
		gdb_cpu_stop_bp_type = type;
		gdb_debug_status = DEBUGGING;
		gdb_handler(GDB_EXC_BP, esf, GDB_SIG_TRAP);
	}
}
#else
/**
 * @brief Implementation of GDB trace handler for SW breakpoint support
 *
 * @param esf Exception stack frame when taking the exception
 *
 * @return N/A
 */

static void _do_gdb_trace_handler(NANO_ESF *esf)
{
	gdb_handler(GDB_EXC_TRACE, esf, GDB_SIG_TRAP);
}
#endif

/**
 * @brief GDB trace handler
 *
 * The GDB trace handler is used to catch and handle the trace mode exceptions
 * (single step).
 *
 * @param esf Exception stack frame when taking the exception
 *
 * @return N/A
 */

void gdb_trace_handler(NANO_ESF *esf)
{
	(void)irq_lock();
	_do_gdb_trace_handler(esf);
}
_EXCEPTION_CONNECT_NOCODE(gdb_trace_handler, IV_DEBUG);

/**
 * @brief GDB breakpoint handler
 *
 * The GDB breakpoint handler is used to catch and handle the breakpoint
 * exceptions.
 *
 * @param esf Exception stack frame when taking the exception
 *
 * @return N/A
 */

void gdb_bp_handler(NANO_ESF *esf)
{
	(void)irq_lock();

	gdb_debug_status = DEBUGGING;
	GDB_SET_STOP_BP_TYPE_SOFT(GDB_SOFT_BP);
	esf->eip -= sizeof(gdb_instr_t);

	gdb_handler(GDB_EXC_BP, esf, GDB_SIG_TRAP);
}
_EXCEPTION_CONNECT_NOCODE(gdb_bp_handler, IV_BREAKPOINT);

/**
 * @brief GDB division-by-zero handler
 *
 * This GDB handler is used to catch and handle the division-by-zero exception.
 *
 * @param esf Exception stack frame when taking the exception
 *
 * @return N/A
 */

void gdb_div_by_zero_handler(NANO_ESF *esf)
{
	(void)irq_lock();
	gdb_debug_status = DEBUGGING;
	gdb_handler(GDB_EXC_OTHER, esf, GDB_SIG_FPE);
}
_EXCEPTION_CONNECT_NOCODE(gdb_div_by_zero_handler, IV_DIVIDE_ERROR);

/**
 * @brief GDB page fault handler
 *
 * This GDB handler is used to catch and handle the page fault exceptions.
 *
 * @param esf Exception stack frame when taking the exception
 *
 * @return N/A
 */

void gdb_pfault_handler(NANO_ESF *esf)
{
	(void)irq_lock();
	gdb_debug_status = DEBUGGING;
	gdb_handler(GDB_EXC_OTHER, esf, GDB_SIG_SIGSEGV);
}
_EXCEPTION_CONNECT_CODE(gdb_pfault_handler, IV_PAGE_FAULT);

#endif /* GDB_ARCH_HAS_RUNCONTROL */
