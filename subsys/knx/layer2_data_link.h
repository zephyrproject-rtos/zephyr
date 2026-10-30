/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_KNX_LAYER2_DATA_LINK_H_
#define ZEPHYR_SUBSYS_KNX_LAYER2_DATA_LINK_H_

#include <zephyr/kernel.h>
#include <zephyr/knx/knx_pkt.h>

/* Application work-queue — defined in knx_core.c. */
extern struct k_work_q knx_app_wq;

/* 2.2 L_Data service */

void knx_l_data_req(struct knx_pkt *pkt);
void knx_l_data_con(struct knx_pkt *pkt);
void knx_l_data_ind(struct knx_pkt *pkt);

/* 2.3 L_SystemBroadcast service */

void knx_l_system_broadcast_req(struct knx_pkt *pkt);
/* L_SystemBroadcast__con and L_SystemBroadcast__ind deleted — see the
 * comment at their former definition site in layer2_data_link.c.
 */

void knx_l_init(void);

void knx_l_frame_rx(struct knx_pkt *pkt);
void knx_l_reset(void);

#endif /* ZEPHYR_SUBSYS_KNX_LAYER2_DATA_LINK_H_ */
