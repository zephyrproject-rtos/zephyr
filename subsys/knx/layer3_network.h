/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __LAYER_NETWORK__
#define __LAYER_NETWORK__

#include <zephyr/knx/knx_pkt.h>

/* 2.2.1 N_Data_Individual service */
/**
 * \fn N_Data_Individual.req(ack_request, destination_address, hop_count_type, octet_count,
 * priority, nsdu) \param ack_request Data Link Layer acknowledge requested or don’t care \param
 * destination_address  Individual Address of the destination \param hop_count_type hop count 7 or
 * Network Layer Parameter \param octet_count length information as described in Data Link Layer
 * \param priority system, urgent, normal or low priority
 * \param nsdu this is the user data to be transferred by the Network Layer
 */
void N_Data_Individual__req(struct knx_pkt *pkt);

/**
 * \fn N_Data_Individual.con(ack_request, destination_address, hop_count_type, octet_count,
 * priority, nsdu, n_status)
 * \param ack_request Data Link Layer acknowledge requested or don’t care
 * \param destination_address Individual Address of the destination
 * \param hop_count_type hop count 7 or Network Layer Parameter
 * \param octet_count length information as described in Data Link Layer
 * \param priority system, urgent, normal or low priority
 * \param nsdu this is the user data that has been transferred by Network Layer
 * \param n_status ok: N_Data_Individual sent successfully with L_Data not_ok: transmission of the
 * associated L_Data request frame did not succeed
 */
void N_Data_Individual__con(struct knx_pkt *pkt);
void N_Data_Individual__ind(struct knx_pkt *pkt);

/* 2.2.2 N_Data_Group service */
void N_Data_Group__req(struct knx_pkt *pkt);
void N_Data_Group__con(struct knx_pkt *pkt);
void N_Data_Group__ind(struct knx_pkt *pkt);

/* 2.2.3 N_Data_Broadcast service */
void N_Data_Broadcast__req(struct knx_pkt *pkt);
void N_Data_Broadcast__con(struct knx_pkt *pkt);
void N_Data_Broadcast__ind(struct knx_pkt *pkt);

/* 2.2.4 N_Data_SystemBroadcast service */
void N_Data_SystemBroadcast__req(struct knx_pkt *pkt);
void N_Data_SystemBroadcast__con(struct knx_pkt *pkt);
void N_Data_SystemBroadcast__ind(struct knx_pkt *pkt);

#endif
