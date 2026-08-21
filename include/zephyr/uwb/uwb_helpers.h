/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_UWB_HELPERS_H_
#define ZEPHYR_INCLUDE_DRIVERS_UWB_HELPERS_H_

#include <stdint.h>
#include <zephyr/uwb/types.h>

/**
 * @brief Print session status notification fields to debug output
 *
 * \param p_session_info Pointer to session status notification structure
 */
void uwb_print_session_status_ntf(const uwb_session_status_notification_t *p_session_info);

/**
 * @brief Print data transfer phase config notification fields to debug output
 *
 * \param p_data_tx_ph_cfg_ntf Pointer to data transfer phase config notification structure
 */
void uwb_print_data_transfer_phase_config_ntf(
	const uwb_data_transfer_phase_config_ntf_t *p_data_tx_ph_cfg_ntf);

/**
 * @brief Print logical link create notification fields to debug output
 *
 * \param p_ll_create_ntf Pointer to logical link create notification structure
 */
void uwb_print_ll_create_ntf(const uwb_logical_link_create_ntf_t *p_ll_create_ntf);

/**
 * @brief Print UWBS logical link close notification fields to debug output
 *
 * \param p_ll_close_ntf Pointer to logical link UWBS close notification structure
 */
void uwb_print_ll_uwbs_close_ntf(uwb_logical_link_uwbs_close_ntf_t *p_ll_close_ntf);

/**
 * @brief Print UWBS logical link create notification fields to debug output
 *
 * \param p_ll_uwbs_create_ntf Pointer to logical link UWBS create notification structure
 */
void uwb_print_ll_uwbs_create_ntf(const uwb_logical_link_uwbs_create_ntf_t *p_ll_uwbs_create_ntf);

/**
 * @brief Print session role change notification fields to debug output
 *
 * \param p_new_role Pointer to session role change notification structure
 */
void uwb_print_session_role_change_ntf(const uwb_session_role_change_ntf_t *p_new_role);

/**
 * @brief Print session update controller multicast list notification fields to debug output
 *
 * \param p_controlee_ntf_context Pointer to multicast list notification structure
 */
void uwb_print_session_update_ctrl_multicast_list_ntf(
	const uwb_session_update_controller_multicast_list_notification_t *p_controlee_ntf_context);

/**
 * @brief Print data transmit status notification fields to debug output
 *
 * \param p_transmit_ntf_context Pointer to data transmit status notification structure
 */
void uwb_print_data_transmit_status_ntf(
	const uwb_data_transmit_notification_t *p_transmit_ntf_context);

/**
 * @brief Print data credit notification fields to debug output
 *
 * \param p_credit_ntf_context Pointer to data credit notification structure
 */
void uwb_print_credit_status_ntf(const uwb_data_credit_notification_t *p_credit_ntf_context);

/**
 * @brief Print data receive notification fields to debug output
 *
 * \param p_rcv_data_pkt Pointer to data receive notification structure
 */
void uwb_print_data_rcv_ntf(const uwb_data_receive_notification_t *p_rcv_data_pkt);

/**
 * @brief Print logical link data receive notification fields to debug output
 *
 * \param p_rcv_data_pkt Pointer to logical link data receive notification structure
 */
void uwb_print_ll_data_rcv_ntf(const uwb_ll_data_receive_notification_t *p_rcv_data_pkt);

/**
 * @brief             Extracts Data notification from the given byte array
 *                    and updates structure uwb_data_receive_notification_t
 *
 * \param p           Pointer to byte array containing data receive notification
 * \param len         Length of input array \p p
 * \param p_rcv_data_pkt Pointer to uwb_data_receive_notification_t structure to be populated
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
 * \param p_rcv_data_pkt Pointer to uwb_ll_data_receive_notification_t structure to be populated
 *
 */
void uwb_parse_ll_data_rcv_ntf(const uint8_t *const p, uint16_t len,
			       uwb_ll_data_receive_notification_t *p_rcv_data_pkt);

#endif /* ZEPHYR_INCLUDE_DRIVERS_UWB_HELPERS_H_ */
