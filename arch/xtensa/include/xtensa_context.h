/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * XTENSA CONTEXT FRAMES AND MACROS FOR RTOS ASSEMBLER SOURCES
 *
 * This header contains definitions and macros for use primarily by Xtensa RTOS
 * assembly coded source files. It includes and uses the Xtensa hardware
 * abstraction layer (HAL) to deal with config specifics. It may also be
 * included in C source files.
 *
 * Supports only Xtensa Exception Architecture 2 (XEA2). XEA1 not supported.
 *
 * NOTE: The Xtensa architecture requires stack pointer alignment to 16 bytes.
 */

#ifndef ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_CONTEXT_H_
#define ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_CONTEXT_H_

#ifdef __ASSEMBLER__
#include    <xtensa/coreasm.h>
#endif

#include    <xtensa/config/tie.h>
#include    <xtensa/corebits.h>
#include    <xtensa/config/system.h>
#include    <xtensa/xtruntime-frames.h>


/* Align a value up to nearest n-byte boundary, where n is a power of 2. */
#define ALIGNUP(n, val) (((val) + (n)-1) & -(n))

/*
 * INTERRUPT/EXCEPTION STACK FRAME FOR A THREAD OR NESTED INTERRUPT
 *
 * A stack frame of this structure is allocated for any interrupt or exception.
 * It goes on the current stack. If the RTOS has a system stack for handling
 * interrupts, every thread stack must allow space for just one interrupt stack
 * frame, then nested interrupt stack frames go on the system stack.
 *
 * The frame includes basic registers (explicit) and "extra" registers
 * introduced by user TIE or the use of the MAC16 option in the user's Xtensa
 * config.  The frame size is minimized by omitting regs not applicable to
 * user's config.
 *
 * For Windowed ABI, this stack frame includes the interruptee's base save
 * area, another base save area to manage gcc nested functions, and a little
 * temporary space to help manage the spilling of the register windows.
 */

STRUCT_BEGIN
STRUCT_FIELD(long, 4, XT_STK_,     exit) /* exit point for dispatch */
STRUCT_FIELD(long, 4, XT_STK_,       pc)   /* return PC */
STRUCT_FIELD(long, 4, XT_STK_,       ps)   /* return PS */
STRUCT_FIELD(long, 4, XT_STK_,       a0)
STRUCT_FIELD(long, 4, XT_STK_,       a1)   /* stack pointer before irq */
STRUCT_FIELD(long, 4, XT_STK_,       a2)
STRUCT_FIELD(long, 4, XT_STK_,       a3)
STRUCT_FIELD(long, 4, XT_STK_,       a4)
STRUCT_FIELD(long, 4, XT_STK_,       a5)
STRUCT_FIELD(long, 4, XT_STK_,       a6)
STRUCT_FIELD(long, 4, XT_STK_,       a7)
STRUCT_FIELD(long, 4, XT_STK_,       a8)
STRUCT_FIELD(long, 4, XT_STK_,       a9)
STRUCT_FIELD(long, 4, XT_STK_,      a10)
STRUCT_FIELD(long, 4, XT_STK_,      a11)
STRUCT_FIELD(long, 4, XT_STK_,      a12)
STRUCT_FIELD(long, 4, XT_STK_,      a13)
STRUCT_FIELD(long, 4, XT_STK_,      a14)
STRUCT_FIELD(long, 4, XT_STK_,      a15)
STRUCT_FIELD(long, 4, XT_STK_,      sar)
STRUCT_FIELD(long, 4, XT_STK_, exccause)
STRUCT_FIELD(long, 4, XT_STK_, excvaddr)
#if XCHAL_HAVE_LOOPS
STRUCT_FIELD(long, 4, XT_STK_,   lbeg)
STRUCT_FIELD(long, 4, XT_STK_,   lend)
STRUCT_FIELD(long, 4, XT_STK_, lcount)
#endif
#ifndef __XTENSA_CALL0_ABI__
/* Temporary space for saving stuff during window spill */
STRUCT_FIELD(long, 4, XT_STK_,   tmp0)
STRUCT_FIELD(long, 4, XT_STK_,   tmp1)
STRUCT_FIELD(long, 4, XT_STK_,   tmp2)
#endif
#ifdef XT_USE_SWPRI
/* Storage for virtual priority mask */
STRUCT_FIELD(long, 4, XT_STK_,   vpri)
#endif
#ifdef XT_USE_OVLY
/* Storage for overlay state */
STRUCT_FIELD(long, 4, XT_STK_,   ovly)
#endif
STRUCT_END(XtExcFrame)

#if defined(_ASMLANGUAGE) || defined(__ASSEMBLER__)
#define XT_STK_NEXT1      XtExcFrameSize
#else
#define XT_STK_NEXT1      sizeof(XtExcFrame)
#endif

/* Allocate extra storage if needed */
#if XCHAL_EXTRA_SA_SIZE != 0

#if XCHAL_EXTRA_SA_ALIGN <= 16
#define XT_STK_EXTRA            ALIGNUP(XCHAL_EXTRA_SA_ALIGN, XT_STK_NEXT1)
#else
/* If need more alignment than stack, add space for dynamic alignment */
#define XT_STK_EXTRA		(ALIGNUP(XCHAL_EXTRA_SA_ALIGN, XT_STK_NEXT1) \
				 + XCHAL_EXTRA_SA_ALIGN)
#endif
#define XT_STK_NEXT2            (XT_STK_EXTRA + XCHAL_EXTRA_SA_SIZE)

#else

#define XT_STK_NEXT2            XT_STK_NEXT1

#endif

/*
 * This is the frame size. Add space for 4 registers (interruptee's base save
 * area) and some space for gcc nested functions if any.
 */
#define XT_STK_FRMSZ            (ALIGNUP(0x10, XT_STK_NEXT2) + 0x20)


/*
 * SOLICITED STACK FRAME FOR A THREAD
 *
 * A stack frame of this structure is allocated whenever a thread enters the
 * RTOS kernel intentionally (and synchronously) to submit to thread
 * scheduling.  It goes on the current thread's stack.
 *
 * The solicited frame only includes registers that are required to be
 * preserved by the callee according to the compiler's ABI conventions, some
 * space to save the return address for returning to the caller, and the
 * caller's PS register. For Windowed ABI, this stack frame includes the
 * caller's base save area.
 *
 * Note on XT_SOL_EXIT field:
 *
 * It is necessary to distinguish a solicited from an interrupt stack frame.
 * This field corresponds to XT_STK_EXIT in the interrupt stack frame and is
 * always at the same offset (0). It can be written with a code (usually 0) to
 * distinguish a solicted frame from an interrupt frame. An RTOS port may opt
 * to ignore this field if it has another way of distinguishing frames.
 */

STRUCT_BEGIN
STRUCT_FIELD(long, 4, XT_SOL_, exit)
STRUCT_FIELD(long, 4, XT_SOL_,   pc)
STRUCT_FIELD(long, 4, XT_SOL_,   ps)
STRUCT_FIELD(long, 4, XT_SOL_, next)
#ifdef __XTENSA_CALL0_ABI__
STRUCT_FIELD(long, 4, XT_SOL_,  a12)    /* should be on 16-byte alignment */
STRUCT_FIELD(long, 4, XT_SOL_,  a13)
STRUCT_FIELD(long, 4, XT_SOL_,  a14)
STRUCT_FIELD(long, 4, XT_SOL_,  a15)
#else
STRUCT_FIELD(long, 4, XT_SOL_,   a0)    /* should be on 16-byte alignment */
STRUCT_FIELD(long, 4, XT_SOL_,   a1)
STRUCT_FIELD(long, 4, XT_SOL_,   a2)
STRUCT_FIELD(long, 4, XT_SOL_,   a3)
#endif
STRUCT_END(XtSolFrame)

/* Size of solicited stack frame */
#define XT_SOL_FRMSZ            ALIGNUP(0x10, XtSolFrameSize)


/*
 * CO-PROCESSOR STATE SAVE AREA FOR A THREAD
 *
 * The RTOS must provide an area per thread to save the state of co-processors
 * when that thread does not have control. Co-processors are context-switched
 * lazily (on demand) only when a new thread uses a co-processor instruction,
 * otherwise a thread retains ownership of the co-processor even when it loses
 * control of the processor. An Xtensa co-processor exception is triggered when
 * any co-processor instruction is executed by a thread that is not the owner,
 * and the context switch of that co-processor is then peformed by the handler.
 * Ownership represents which thread's state is currently in the co-processor.
 *
 * Co-processors may not be used by interrupt or exception handlers. If a
 * co-processor instruction is executed by an interrupt or exception handler,
 * the co-processor exception handler will trigger a kernel panic and freeze.
 * This restriction is introduced to reduce the overhead of saving and
 * restoring  co-processor state (which can be quite large) and in particular
 * remove that overhead from interrupt handlers.
 *
 * The co-processor state save area may be in any convenient per-thread
 * location such as in the thread control block or above the thread stack area.
 * It need not be in the interrupt stack frame since interrupts don't use
 * co-processors.
 *
 * Along with the save area for each co-processor, two bitmasks with flags per
 * co-processor (laid out as in the CPENABLE reg) help manage context-switching
 * co-processors as efficiently as possible:
 *
 * XT_CPENABLE
 *
 * The contents of a non-running thread's CPENABLE register.  It represents the
 * co-processors owned (and whose state is still needed) by the thread. When a
 * thread is preempted, its CPENABLE is saved here.  When a thread solicits a
 * context-swtich, its CPENABLE is cleared - the compiler has saved the
 * (caller-saved) co-proc state if it needs to.  When a non-running thread
 * loses ownership of a CP, its bit is cleared.  When a thread runs, it's
 * XT_CPENABLE is loaded into the CPENABLE reg. Avoids co-processor exceptions
 * when no change of ownership is needed.
 *
 * XT_CPSTORED
 *
 * A bitmask with the same layout as CPENABLE, a bit per co-processor.
 * Indicates whether the state of each co-processor is saved in the state save
 * area. When a thread enters the kernel, only the state of co-procs still
 * enabled in CPENABLE is saved. When the co-processor exception handler
 * assigns ownership of a co-processor to a thread, it restores the saved state
 * only if this bit is set, and clears this bit.
 *
 * XT_CP_CS_ST
 *
 * A bitmask with the same layout as CPENABLE, a bit per co-processor.
 * Indicates whether callee-saved state is saved in the state save area.
 * Callee-saved state is saved by itself on a solicited context switch, and
 * restored when needed by the coprocessor exception handler.  Unsolicited
 * switches will cause the entire coprocessor to be saved when necessary.
 *
 * XT_CP_ASA
 *
 * Pointer to the aligned save area.  Allows it to be aligned more than the
 * overall save area (which might only be stack-aligned or TCB-aligned).
 * Especially relevant for Xtensa cores configured with a very large data path
 * that requires alignment greater than 16 bytes (ABI stack alignment).
 */

#define XT_CP_DESCR_SIZE 12

#if XCHAL_CP_NUM > 0

/*  Offsets of each coprocessor save area within the 'aligned save area':  */
#define XT_CP0_SA   0
#define XT_CP1_SA   ALIGNUP(XCHAL_CP1_SA_ALIGN, XT_CP0_SA + XCHAL_CP0_SA_SIZE)
#define XT_CP2_SA   ALIGNUP(XCHAL_CP2_SA_ALIGN, XT_CP1_SA + XCHAL_CP1_SA_SIZE)
#define XT_CP3_SA   ALIGNUP(XCHAL_CP3_SA_ALIGN, XT_CP2_SA + XCHAL_CP2_SA_SIZE)
#define XT_CP4_SA   ALIGNUP(XCHAL_CP4_SA_ALIGN, XT_CP3_SA + XCHAL_CP3_SA_SIZE)
#define XT_CP5_SA   ALIGNUP(XCHAL_CP5_SA_ALIGN, XT_CP4_SA + XCHAL_CP4_SA_SIZE)
#define XT_CP6_SA   ALIGNUP(XCHAL_CP6_SA_ALIGN, XT_CP5_SA + XCHAL_CP5_SA_SIZE)
#define XT_CP7_SA   ALIGNUP(XCHAL_CP7_SA_ALIGN, XT_CP6_SA + XCHAL_CP6_SA_SIZE)
#define XT_CP_SA_SIZE   ALIGNUP(16, XT_CP7_SA + XCHAL_CP7_SA_SIZE)

/*  Offsets within the overall save area:  */

/* (2 bytes) coprocessors active for this thread */
#define XT_CPENABLE 0

 /* (2 bytes) coprocessors saved for this thread */
#define XT_CPSTORED 2

/* (2 bytes) coprocessor callee-saved regs stored for this thread */
#define XT_CP_CS_ST 4

/* (4 bytes) ptr to aligned save area */
#define XT_CP_ASA   8

/* Overall size allows for dynamic alignment:  */
#define XT_CP_SIZE ALIGNUP(XCHAL_TOTAL_SA_ALIGN, \
	XT_CP_DESCR_SIZE + XT_CP_SA_SIZE)
#else
#define XT_CP_SIZE  0
#endif


/*
 * MACROS TO HANDLE ABI SPECIFICS OF FUNCTION ENTRY AND RETURN
 *
 * Convenient where the frame size requirements are the same for both ABIs.
 * ENTRY(sz), RET(sz) are for framed functions (have locals or make calls).
 * ENTRY0,    RET0    are for frameless functions (no locals, no calls).
 *
 * where size = size of stack frame in bytes (must be >0 and aligned to 16).
 * For framed functions the frame is created and the return address saved at
 * base of frame (Call0 ABI) or as determined by hardware (Windowed ABI).  For
 * frameless functions, there is no frame and return address remains in
 * a0.
 *
 * Note: Because CPP macros expand to a single line, macros requiring
 * multi-line expansions are implemented as assembler macros.
 */

#ifdef __ASSEMBLER__
#ifdef __XTENSA_CALL0_ABI__
/* Call0 */
#define ENTRY(sz)     entry1  sz
.macro  entry1 size=0x10
addi    sp, sp, -\size
s32i    a0, sp, 0
.endm
#define ENTRY0
#define RET(sz)       ret1    sz
.macro  ret1 size=0x10
l32i    a0, sp, 0
addi    sp, sp, \size
ret
.endm
#define RET0          ret
#else
/* Windowed */
#define ENTRY(sz)     entry   sp, sz
#define ENTRY0        entry   sp, 0x10
#define RET(sz)       retw
#define RET0          retw
#endif /* __XTENSA_CALL0_ABI__ */
#endif /* __ASSEMBLER__ */


#endif /* ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_CONTEXT_H_ */

