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

#ifndef _GDB_ARCH__H_
#define _GDB_ARCH__H_

#ifdef __cplusplus
extern "C" {
#endif

#include <kernel_structs.h>
#include <misc/debug/gdb_server.h>

#define GDB_ARCH_HAS_ALL_REGS

#ifndef CONFIG_GDB_SERVER_BOOTLOADER
	#undef  GDB_ARCH_HAS_HW_BP
	#define GDB_ARCH_HAS_REMOTE_SERIAL_EXT_USING_NOTIF_PACKETS
	#define GDB_ARCH_HAS_RUNCONTROL
	#define GDB_ARCH_CAN_STEP gdb_arch_can_step
#endif

#ifndef CONFIG_GDB_RAM_SIZE
	#define CONFIG_GDB_RAM_SIZE (CONFIG_RAM_SIZE * 1024)
#endif
#ifndef CONFIG_GDB_RAM_ADDRESS
	#define CONFIG_GDB_RAM_ADDRESS 0x100000
#endif

/* Default GDB buffer size */
#ifdef CONFIG_GDB_SERVER_BOOTLOADER
	#define GDB_BUF_SIZE 8192
#else
	#define GDB_BUF_SIZE  600
#endif

#define GDB_TGT_ARCH "i386"

#define GDB_NUM_REGS 16
#define GDB_NUM_REG_BYTES (GDB_NUM_REGS * 4)

#define GDB_PC_REG 8

#define GDB_BREAK_INSTRUCTION 0xcc /* 'int3' opcode */

#ifndef _ASMLANGUAGE

typedef unsigned char gdb_instr_t;

struct gdb_reg_set {
	NANO_ISF regs;
	unsigned int pad1; /* padding for ss register */
	unsigned int pad2; /* padding for ds register */
	unsigned int pad3; /* padding for es register */
	unsigned int pad4; /* padding for fs register */
	unsigned int pad5; /* padding for gs register */
};

struct gdb_debug_regs {
	unsigned int db0; /* debug register 0 */
	unsigned int db1; /* debug register 1 */
	unsigned int db2; /* debug register 2 */
	unsigned int db3; /* debug register 3 */
	unsigned int db6; /* debug register 6 */
	unsigned int db7; /* debug register 7 */
};

#if defined(GDB_ARCH_HAS_RUNCONTROL) && defined(GDB_ARCH_HAS_HW_BP)
extern volatile int gdb_cpu_stop_bp_type;
extern long gdb_cpu_stop_hw_bp_addr;
#define GDB_SET_STOP_BP_TYPE_SOFT(x) \
	do { gdb_cpu_stop_bp_type = (x); } while ((0))
#else
#define GDB_SET_STOP_BP_TYPE_SOFT(x)
#endif

extern void gdb_arch_init(void);
extern void gdb_arch_regs_from_esf(struct gdb_reg_set *regs,
				   NANO_ESF *esf);
extern void gdb_arch_regs_to_esf(struct gdb_reg_set *regs,
				 NANO_ESF *esf);
extern void gdb_arch_regs_from_isf(struct gdb_reg_set *regs,
				   NANO_ISF *esf);
extern void gdb_arch_regs_to_isf(struct gdb_reg_set *regs,
				 NANO_ISF *esf);
extern void gdb_arch_regs_get(struct gdb_reg_set *regs, char *buffer);
extern void gdb_arch_regs_set(struct gdb_reg_set *regs, char *buffer);
extern void gdb_arch_reg_info_get(int reg_id, int *size, int *offset);
extern void gdb_trace_mode_clear(struct gdb_reg_set *regs, int arg);
extern int gdb_trace_mode_set(struct gdb_reg_set *regs);
extern int gdb_arch_can_step(struct gdb_reg_set *regs);
#ifdef GDB_ARCH_HAS_HW_BP
extern int gdb_hw_bp_set(struct gdb_debug_regs *regs, long addr,
			 enum gdb_bp_type type, int length,
			 enum gdb_error_code *err);
extern int gdb_hw_bp_clear(struct gdb_debug_regs *regs, long addr,
			   enum gdb_bp_type type, int length,
			   enum gdb_error_code *err);
extern void gdb_dbg_regs_set(struct gdb_debug_regs *regs);
extern void gdb_dbg_regs_get(struct gdb_debug_regs *regs);
extern void gdb_dbg_regs_clear(void);
#endif

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif
#endif /* _GDB_ARCH__H_ */
