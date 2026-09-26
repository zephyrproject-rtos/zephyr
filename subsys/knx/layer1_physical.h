/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 *
 * KNX Physical Layer — subsystem-internal header.
 * The public L1 interface (enums, primitives, burst TX) is in:
 *   <zephyr/knx/knx_l1.h>
 */
#ifndef __LAYER1_PHYSICAL__
#define __LAYER1_PHYSICAL__

#include <zephyr/knx/knx_l1.h>
#include <zephyr/knx/knx_pkt.h>

/* Frame accumulator state flags (used in layer1_physical.c) */
#define FRAME_STATE_EMPTY     0
#define FRAME_STATE_RECIVEING 1
#define FRAME_STATE_COMPLETE  2
#define FRAME_STATE_SENDING   4
#define FRAME_STATE_SENT      5

void Ph_Init(void);
void Ph_Loop(void);

#endif /* __LAYER1_PHYSICAL__ */
