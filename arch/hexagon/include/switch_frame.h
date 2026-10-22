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
 * register for thread-local storage.
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
#define SWITCH_FRAME_SIZE 0x40
#else
#define SWITCH_FRAME_SIZE 0x38
#endif

#endif /* ZEPHYR_ARCH_HEXAGON_INCLUDE_SWITCH_FRAME_H_ */
