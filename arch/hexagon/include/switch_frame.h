/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_HEXAGON_INCLUDE_SWITCH_FRAME_H_
#define ZEPHYR_ARCH_HEXAGON_INCLUDE_SWITCH_FRAME_H_

/*
 * Switch frame layout for context switching.
 *
 * This header is shared between switch.S, thread.c, and tls.c to
 * ensure the stack frame offsets stay in sync.
 *
 * The frame stores callee-saved registers (r16-r27), the frame
 * pointer and link register (r30, r31), and optionally the UGP
 * register for thread-local storage and/or GOSP (G2) for userspace.
 */

#define SWITCH_R1716     0x00
#define SWITCH_R1918     0x08
#define SWITCH_R2120     0x10
#define SWITCH_R2322     0x18
#define SWITCH_R2524     0x20
#define SWITCH_R2726     0x28
#define SWITCH_FP_LR     0x30

#ifdef CONFIG_THREAD_LOCAL_STORAGE
#define SWITCH_UGP       0x38
#define SWITCH_TAIL_SIZE 0x08
#else
#define SWITCH_TAIL_SIZE 0x00
#endif

/*
 * GOSP (G2) is real per-thread hardware state: HVM swaps it with r29 on
 * every user<->guest transition (see arch_user_mode_enter()), so it must
 * be saved/restored across a context switch like r16-r31 and UGP --
 * otherwise a switch taken mid-syscall leaves one thread's GOSP in the
 * register when a different thread's later vmrte reads it back.
 */
#ifdef CONFIG_USERSPACE
#define SWITCH_GOSP      (0x38 + SWITCH_TAIL_SIZE)
#define SWITCH_FRAME_SIZE (0x38 + SWITCH_TAIL_SIZE + 0x08)
#else
#define SWITCH_FRAME_SIZE (0x38 + SWITCH_TAIL_SIZE)
#endif

#endif /* ZEPHYR_ARCH_HEXAGON_INCLUDE_SWITCH_FRAME_H_ */
