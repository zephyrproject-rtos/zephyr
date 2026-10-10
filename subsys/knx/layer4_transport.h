/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __LAYER4_TRANSPORT__
#define __LAYER4_TRANSPORT__

#include <zephyr/kernel.h>
#include <zephyr/knx/knx_pkt.h>

/* Application work-queue — defined in knx_core.c, used by L4 timers. */
extern struct k_work_q knx_app_wq;

/* 3.2 T_Data_Group service */
void T_Data_Group__req(struct knx_pkt *pkt);
void T_Data_Group__con(struct knx_pkt *pkt);
void T_Data_Group__ind(struct knx_pkt *pkt);

/* 3.3 T_Data_Tag_Group service */
void T_Data_Tag_Group__req(struct knx_pkt *pkt);
void T_Data_Tag_Group__con(struct knx_pkt *pkt);
void T_Data_Tag_Group__ind(struct knx_pkt *pkt);

/* 3.4 T_Data_Broadcast service */
void T_Data_Broadcast__req(struct knx_pkt *pkt);
void T_Data_Broadcast__con(struct knx_pkt *pkt);
void T_Data_Broadcast__ind(struct knx_pkt *pkt);

/* 3.5 T_Data_SystemBroadcast service */
void T_Data_SystemBroadcast__req(struct knx_pkt *pkt);
void T_Data_SystemBroadcast__con(struct knx_pkt *pkt);
void T_Data_SystemBroadcast__ind(struct knx_pkt *pkt);

/* 3.6 T_Data_Individual service */
void T_Data_Individual__req(struct knx_pkt *pkt);
void T_Data_Individual__con(struct knx_pkt *pkt);
void T_Data_Individual__ind(struct knx_pkt *pkt);

/* 3.7 T_Connect service — pkt->dst = destination, pkt->priority, pkt->tsap */
void T_Connect__req(struct knx_pkt *pkt);
void T_Connect__con(struct knx_pkt *pkt);
void T_Connect__ind(struct knx_pkt *pkt);

/* 3.8 T_Disconnect service — pkt->priority, pkt->tsap */
void T_Disconnect__req(struct knx_pkt *pkt);
void T_Disconnect__con(struct knx_pkt *pkt);
void T_Disconnect__ind(struct knx_pkt *pkt);

/* 3.9 T_Data_Connected service — pkt->tsap, pkt->buf, pkt->lsdu_len */
void T_Data_Connected__req(struct knx_pkt *pkt);
void T_Data_Connected__con(struct knx_pkt *pkt);
void T_Data_Connected__ind(struct knx_pkt *pkt);

void T_Init(void);

#endif
