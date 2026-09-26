/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __LAYER2_DATA_LINK__
#define __LAYER2_DATA_LINK__

#include <zephyr/kernel.h>
#include <zephyr/knx/knx_pkt.h>

/* Application work-queue — defined in knx_core.c. */
extern struct k_work_q knx_app_wq;

/* 2.2 L_Data service */

void L_Data__req(struct knx_pkt *pkt);
void L_Data__con(struct knx_pkt *pkt);
void L_Data__ind(struct knx_pkt *pkt);

/* 2.3 L_SystemBroadcast service */

void L_SystemBroadcast__req(struct knx_pkt *pkt);
void L_SystemBroadcast__con(struct knx_pkt *pkt);
/* L_SystemBroadcast__ind deleted — see the comment at its former
 * definition site in layer2_data_link.c.
 */

void L_Run(void);
void L_Init(void);

void l_frame_rx(struct knx_pkt *pkt);
void l_reset(void);

#endif
