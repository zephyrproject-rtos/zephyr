/* gdb_stub.s - extra work performed upon exception entry/exit for GDB */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
  DESCRIPTION

  Prep work done when entering exceptions consists of saving the callee-saved
  registers before they get used by exception handlers, and recording the fact
  that we are running in an exception.
*/

#ifndef _GDB_STUB__H_
#define _GDB_STUB__H_

#ifdef _ASMLANGUAGE

#if CONFIG_GDB_INFO
GTEXT(_GdbStubExcEntry)
_GDB_STUB_EXC_ENTRY:    .macro
	push {lr}
	bl irq_lock
	bl _GdbStubExcEntry
	bl irq_unlock
	pop {lr}
.endm

GTEXT(_GdbStubExcExit)
_GDB_STUB_EXC_EXIT:     .macro
	push {lr}
	bl irq_lock
	bl _GdbStubExcExit
	bl irq_unlock
	pop {lr}
.endm

GTEXT(_irq_vector_table_entry_with_gdb_stub)

#else
#define _GDB_STUB_EXC_ENTRY
#define _GDB_STUB_EXC_EXIT
#endif /* CONFIG_GDB_INFO */

#else
extern void _irq_vector_table_entry_with_gdb_stub(void);
#endif /* _ASMLANGUAGE */
#endif /* _GDB_STUB__H_ */
