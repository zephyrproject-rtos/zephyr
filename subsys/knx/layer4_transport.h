/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_KNX_LAYER4_TRANSPORT_H_
#define ZEPHYR_SUBSYS_KNX_LAYER4_TRANSPORT_H_

#include <zephyr/kernel.h>
#include <zephyr/knx/knx_pkt.h>

/* Application work-queue — defined in knx_core.c, used by L4 timers. */
extern struct k_work_q knx_app_wq;

/* 3.2 T_Data_Group service */
void knx_t_data_group_req(struct knx_pkt *pkt);
void knx_t_data_group_con(struct knx_pkt *pkt);
void knx_t_data_group_ind(struct knx_pkt *pkt);

/* 3.3 T_Data_Tag_Group service */
void knx_t_data_tag_group_req(struct knx_pkt *pkt);
void knx_t_data_tag_group_con(struct knx_pkt *pkt);
void knx_t_data_tag_group_ind(struct knx_pkt *pkt);

/* 3.4 T_Data_Broadcast service */
void knx_t_data_broadcast_req(struct knx_pkt *pkt);
void knx_t_data_broadcast_con(struct knx_pkt *pkt);
void knx_t_data_broadcast_ind(struct knx_pkt *pkt);

/* 3.5 T_Data_SystemBroadcast service */
void knx_t_data_system_broadcast_req(struct knx_pkt *pkt);
void knx_t_data_system_broadcast_con(struct knx_pkt *pkt);
void knx_t_data_system_broadcast_ind(struct knx_pkt *pkt);

/* 3.6 T_Data_Individual service */
void knx_t_data_individual_req(struct knx_pkt *pkt);
void knx_t_data_individual_con(struct knx_pkt *pkt);
void knx_t_data_individual_ind(struct knx_pkt *pkt);

/* 3.7 T_Connect service — pkt->dst = destination, pkt->priority, pkt->tsap */
void knx_t_connect_req(struct knx_pkt *pkt);
void knx_t_connect_con(struct knx_pkt *pkt);
void knx_t_connect_ind(struct knx_pkt *pkt);

/* 3.8 T_Disconnect service — pkt->priority, pkt->tsap */
void knx_t_disconnect_req(struct knx_pkt *pkt);
void knx_t_disconnect_con(struct knx_pkt *pkt);
void knx_t_disconnect_ind(struct knx_pkt *pkt);

/* 3.9 T_Data_Connected service — pkt->tsap, pkt->buf, pkt->lsdu_len */
void knx_t_data_connected_req(struct knx_pkt *pkt);
void knx_t_data_connected_con(struct knx_pkt *pkt);
void knx_t_data_connected_ind(struct knx_pkt *pkt);

void knx_t_init(void);

#endif /* ZEPHYR_SUBSYS_KNX_LAYER4_TRANSPORT_H_ */
