/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_KNX_LAYER3_NETWORK_H_
#define ZEPHYR_SUBSYS_KNX_LAYER3_NETWORK_H_

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
void knx_n_data_individual_req(struct knx_pkt *pkt);

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
void knx_n_data_individual_con(struct knx_pkt *pkt);
void knx_n_data_individual_ind(struct knx_pkt *pkt);

/* 2.2.2 N_Data_Group service */
void knx_n_data_group_req(struct knx_pkt *pkt);
void knx_n_data_group_con(struct knx_pkt *pkt);
void knx_n_data_group_ind(struct knx_pkt *pkt);

/* 2.2.3 N_Data_Broadcast service */
void knx_n_data_broadcast_req(struct knx_pkt *pkt);
void knx_n_data_broadcast_con(struct knx_pkt *pkt);
void knx_n_data_broadcast_ind(struct knx_pkt *pkt);

/* 2.2.4 N_Data_SystemBroadcast service */
void knx_n_data_system_broadcast_req(struct knx_pkt *pkt);
void knx_n_data_system_broadcast_con(struct knx_pkt *pkt);
void knx_n_data_system_broadcast_ind(struct knx_pkt *pkt);

#endif /* ZEPHYR_SUBSYS_KNX_LAYER3_NETWORK_H_ */
