/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __UWB_HELPERS_H__
#define __UWB_HELPERS_H__

#include <stdint.h>
#include <zephyr/uwb/types.h>

void uwb_print_session_status_ntf(const uwb_session_status_notification_t *p_session_info);
void uwb_print_data_transfer_phase_config_ntf(
	const uwb_data_transfer_phase_config_ntf_t *p_data_tx_ph_cfg_ntf);
void uwb_print_ll_create_ntf(const uwb_logical_link_create_ntf_t *p_ll_create_ntf);
void uwb_print_ll_uwbs_close_ntf(uwb_logical_link_uwbs_close_ntf_t *p_ll_close_ntf);
void uwb_print_ll_uwbs_create_ntf(const uwb_logical_link_uwbs_create_ntf_t *p_ll_uwbs_create_ntf);
void uwb_print_session_role_change_ntf(const uwb_session_role_change_ntf_t *p_new_role);
void uwb_print_session_update_ctrl_multicast_list_ntf(
	const uwb_session_update_controller_multicast_list_notification_t *p_controlee_ntf_context);
void uwb_print_data_transmit_status_ntf(
	const uwb_data_transmit_notification_t *p_transmit_ntf_context);
void uwb_print_credit_status_ntf(const uwb_data_credit_notification_t *p_credit_ntf_context);
void uwb_print_data_rcv_ntf(const uwb_data_receive_notification_t *p_rcv_data_pkt);
void uwb_print_ll_data_rcv_ntf(const uwb_ll_data_receive_notification_t *p_rcv_data_pkt);

/**
 * @brief             Extracts Data notification from the given byte array
 *                    and updates structure uwb_data_receive_notification_t
 *
 * \param p           Pointer to byte array containing data receive notification
 * \param len         Length of input array \p p
 * \param pRcvDataPkt Pointer to uwb_data_receive_notification_t structure to be populated
 *
 */
void uwb_parse_data_rcv_ntf(const uint8_t *const p, uint16_t len,
			    uwb_data_receive_notification_t *p_rcv_data_pkt);

/**
 * @brief             Extracts Data notification from the given byte array
 *                    and updates structure uwb_ll_data_receive_notification_t
 *
 * \param p           Pointer to byte array containing data receive notification
 * \param len         Length of input array \p p
 * \param pRcvDataPkt Pointer to uwb_ll_data_receive_notification_t structure to be populated
 *
 */
void uwb_parse_ll_data_rcv_ntf(const uint8_t *const p, uint16_t len,
			       uwb_ll_data_receive_notification_t *p_rcv_data_pkt);

#endif /* __UWB_HELPERS_H__ */
