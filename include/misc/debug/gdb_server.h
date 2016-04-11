/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __MISC_DEBUG_GDB_SERVER__H_
#define __MISC_DEBUG_GDB_SERVER__H_

#ifdef __cplusplus
extern "C" {
#endif

/* define */

#define GDB_STOP_CHAR   0x3	/* GDB RSP default value */

#define NOT_DEBUGGING   0
#define DEBUGGING       1
#define STOP_RUNNING    2
#define SINGLE_STEP     3

#ifndef _ASMLANGUAGE

enum gdb_error_code {
	GDB_ERROR_BP_LIST_FULL = 1,	/* Reach max number of Hard&Soft BPs */
	GDB_ERROR_INVALID_BP,		/* No such BP in breakpoints list */
	GDB_ERROR_HW_BP_INVALID_TYPE,	/* Unsupported type/len combination */
	GDB_ERROR_HW_BP_DBG_REGS_FULL,	/* Debug register set full */
	GDB_ERROR_HW_BP_NOT_SUP,	/* hardware breakpoint not supported */
	GDB_ERROR_INVALID_MEM		/* trying to access invalid memory */
};

enum gdb_bp_type {
	GDB_SOFT_BP,             /* software breakpoint */
	GDB_HW_INST_BP,          /* hardware instruction breakpoint */
	GDB_HW_DATA_WRITE_BP,    /* write watchpoint */
	GDB_HW_DATA_READ_BP,     /* read watchpoint */
	GDB_HW_DATA_ACCESS_BP,   /* access watchpoint */
	GDB_UNKNOWN_BP_TYPE = -1 /* unknown breakpoint type */
};

enum gdb_exc_mode {
	GDB_EXC_TRACE,	/* trace exception */
	GDB_EXC_BP,	/* breakpoint exception */
	GDB_EXC_OTHER,	/* other exceptions */
};

enum gdb_signal {
	GDB_SIG_NULL = -1,
	GDB_SIG_INT = 2,
	GDB_SIG_TRAP = 5,
	GDB_SIG_FPE = 8,
	GDB_SIG_SIGSEGV = 11,
	GDB_SIG_STOP = 17
};

extern volatile int gdb_debug_status;

extern void gdb_system_stop_here(void *regs);
extern void gdb_handler(enum gdb_exc_mode mode, void *esf, int signal);

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif
#endif /* __MISC_DEBUG_GDB_SERVER__H_*/
