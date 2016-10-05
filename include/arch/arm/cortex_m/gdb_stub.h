/*
 * Copyright (c) 2014 Wind River Systems, Inc.
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

/**
 * @file
 * @brief Extra work performed upon exception entry/exit for GDB
 *
 *
 * Prep work done when entering exceptions consists of saving the callee-saved
 * registers before they get used by exception handlers, and recording the fact
 * that we are running in an exception.
 */

#ifndef _GDB_STUB__H_
#define _GDB_STUB__H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _ASMLANGUAGE

#if CONFIG_GDB_INFO
GTEXT(_GdbStubExcEntry)
_GDB_STUB_EXC_ENTRY :    .macro
	push {lr}
	bl irq_lock
	bl _GdbStubExcEntry
	bl irq_unlock
	pop {r1}
	move lr, r1
.endm

GTEXT(_GdbStubExcExit)
_GDB_STUB_EXC_EXIT :     .macro
	push {lr}
	bl irq_lock
	bl _GdbStubExcExit
	bl irq_unlock
	pop {r1}
	move lr, r1
.endm

GTEXT(_irq_vector_table_entry_with_gdb_stub)

#else
#define _GDB_STUB_EXC_ENTRY
#define _GDB_STUB_EXC_EXIT
#endif /* CONFIG_GDB_INFO */

#else
extern void _irq_vector_table_entry_with_gdb_stub(void);
#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _GDB_STUB__H_ */
