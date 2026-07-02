/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inttypes.h>
#include "zephyr/uwb/uwb_helpers.h"
#include "zephyr/uwb/types.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(uwb_helper, CONFIG_UWB_LOG_LEVEL);

void uwb_print_session_status_ntf(const uwb_session_status_notification_t *p_session_info)
{
	if (p_session_info != NULL) {
		LOG_INF("p_session_info->sessionHandle          : %" PRIu32 "",
			p_session_info->sessionHandle);
		LOG_INF("p_session_info->state               : %hu", p_session_info->state);
		LOG_INF("p_session_info->reason_code         : %hu", p_session_info->reason_code);
	} else {
		LOG_ERR("p_session_info is NULL");
	}
}

void uwb_print_data_transfer_phase_config_ntf(
	const uwb_data_transfer_phase_config_ntf_t *p_data_tx_ph_cfg_ntf)
{
	if (p_data_tx_ph_cfg_ntf != NULL) {
		LOG_INF("p_data_tx_ph_cfg_ntf->sessionHandle                : 0x%x\n",
			p_data_tx_ph_cfg_ntf->sessionHandle);
		LOG_INF("p_data_tx_ph_cfg_ntf->status                       : 0x%x\n",
			p_data_tx_ph_cfg_ntf->status);
	} else {
		LOG_ERR("pRcvRadaNtfPkt is NULL");
	}
}

void uwb_print_ll_create_ntf(const uwb_logical_link_create_ntf_t *p_ll_create_ntf)
{
	if (p_ll_create_ntf != NULL) {
		LOG_DBG("p_ll_create_ntf->ll_connect_id                       : %x\n",
			p_ll_create_ntf->ll_connect_id);
		LOG_DBG("p_ll_create_ntf->status                              : %x\n",
			p_ll_create_ntf->status);
	} else {
		LOG_ERR("p_ll_create_ntf is NULL");
	}
}

void uwb_print_ll_uwbs_close_ntf(uwb_logical_link_uwbs_close_ntf_t *p_ll_close_ntf)
{
	if (p_ll_close_ntf != NULL) {
		LOG_DBG("p_ll_close_ntf->ll_connect_id                        : %x\n",
			p_ll_close_ntf->ll_connect_id);
		LOG_DBG("p_ll_close_ntf->status                               : %x\n",
			p_ll_close_ntf->status);
	} else {
		LOG_ERR("p_ll_close_ntf is NULL");
	}
}

void uwb_print_ll_uwbs_create_ntf(const uwb_logical_link_uwbs_create_ntf_t *p_ll_uwbs_create_ntf)
{
	if (p_ll_uwbs_create_ntf != NULL) {
		LOG_DBG("p_ll_uwbs_create_ntf->sessionHandle                           : %x\n",
			p_ll_uwbs_create_ntf->sessionHandle);
		LOG_DBG("p_ll_uwbs_create_ntf->ll_connect_id                           : %x\n",
			p_ll_uwbs_create_ntf->ll_connect_id);
		LOG_DBG("p_ll_uwbs_create_ntf->llm_selector                            : %x\n",
			p_ll_uwbs_create_ntf->llm_selector);
		LOG_HEXDUMP_DBG(p_ll_uwbs_create_ntf->src_address, UWB_EXTENDED_MAC_ADDRESS_LEN,
				"p_ll_uwbs_create_ntf->src_address                             : ");
	} else {
		LOG_ERR("p_ll_uwbs_create_ntf is NULL");
	}
}

void uwb_print_session_role_change_ntf(const uwb_session_role_change_ntf_t *p_new_role)
{
	if (p_new_role != NULL) {
		LOG_DBG("p_new_role->sessionHandle              : 0x%X", p_new_role->sessionHandle);
		LOG_DBG("p_new_role->new_role          : %d", p_new_role->new_role);
	} else {
		LOG_ERR("p_new_role is NULL");
	}
}

void uwb_print_session_update_ctrl_multicast_list_ntf(
	const uwb_session_update_controller_multicast_list_notification_t *p_controlee_ntf_context)
{
	if (p_controlee_ntf_context != NULL) {
		LOG_INF("p_controlee_ntf_context->sessionHandle          : %" PRIu32 " ",
			p_controlee_ntf_context->sessionHandle);
		LOG_INF("p_controlee_ntf_context->no_of_controlees    : %hu",
			p_controlee_ntf_context->no_of_controlees);

		for (uint8_t i = 0; i < p_controlee_ntf_context->no_of_controlees; i++) {
#if UWBIOT_UWBD_SR1XXT_SR2XXT
			LOG_INF("p_controlee_ntf_context->controleeStatusList[%hu].controlee_mac_"
				"address  : %" PRIu16 "",
				i,
				p_controlee_ntf_context->controleeStatusList[i]
					.controlee_mac_address);
#endif // UWBIOT_UWBD_SR1XXT_SR2XXT
			LOG_INF("p_controlee_ntf_context->controleeStatusList[%hu].status          "
				"       : %hu",
				i, p_controlee_ntf_context->controleeStatusList[i].status);
		}
	} else {
		LOG_ERR("phControleeNtfContext_t is NULL");
	}
}

void uwb_parse_data_rcv_ntf(const uint8_t *const p, uint16_t len,
			    uwb_data_receive_notification_t *p_rcv_data_pkt)
{
	uint32_t offset = 0;
	UWB_STREAM_TO_UINT32(p_rcv_data_pkt->connection_identifier, p, offset);
	UWB_STREAM_TO_UINT8(p_rcv_data_pkt->status, p, offset);
	UWB_STREAM_TO_ARRAY(p_rcv_data_pkt->src_address, p, UWB_EXTENDED_MAC_ADDRESS_LEN, offset);
	UWB_STREAM_TO_UINT16(p_rcv_data_pkt->sequence_number, p, offset);
	UWB_STREAM_TO_UINT16(p_rcv_data_pkt->data_size, p, offset);
	UWB_STREAM_TO_ARRAY(p_rcv_data_pkt->data, p, p_rcv_data_pkt->data_size, offset);
}

void uwb_print_data_transmit_status_ntf(
	const uwb_data_transmit_notification_t *p_transmit_ntf_context)
{
	if (p_transmit_ntf_context != NULL) {
		LOG_INF("p_transmit_ntf_context->connectionId       : 0%x\n",
			p_transmit_ntf_context->connection_identifier);
		LOG_INF("p_transmit_ntf_context->sequence_number     : %d\n",
			p_transmit_ntf_context->sequence_number);
		LOG_INF("p_transmit_ntf_context->status              : %d\n",
			p_transmit_ntf_context->status);
		LOG_INF("p_transmit_ntf_context->txcount             : %d\n",
			p_transmit_ntf_context->txcount);
	} else {
		LOG_ERR("p_transmit_ntf_context is NULL");
	}
}

void uwb_print_credit_status_ntf(const uwb_data_credit_notification_t *p_credit_ntf_context)
{
	if (p_credit_ntf_context != NULL) {
		LOG_INF("p_credit_ntf_context->connectionId               : 0%x\n",
			p_credit_ntf_context->connectionId);
		LOG_INF("p_credit_ntf_context->credit_availability        : %d\n",
			p_credit_ntf_context->credit_availability);
	} else {
		LOG_ERR("p_credit_ntf_context is NULL");
	}
}

void uwb_print_data_rcv_ntf(const uwb_data_receive_notification_t *p_rcv_data_pkt)
{
	if (p_rcv_data_pkt != NULL) {
		LOG_INF("p_rcv_data_pkt->connection_identifier            : 0x%x",
			p_rcv_data_pkt->connection_identifier);
		LOG_INF("p_rcv_data_pkt->status                   : 0x%x", p_rcv_data_pkt->status);
		LOG_HEXDUMP_DBG(p_rcv_data_pkt->src_address, UWB_EXTENDED_MAC_ADDRESS_LEN,
				"SrcMacAddr                              : ");
		LOG_INF("p_rcv_data_pkt->sequence_number          : %d",
			p_rcv_data_pkt->sequence_number);
		if (p_rcv_data_pkt->status == kUci_Status_Ok) {
			LOG_INF("data_size                             : %d",
				p_rcv_data_pkt->data_size);
			LOG_HEXDUMP_INF(p_rcv_data_pkt->data, p_rcv_data_pkt->data_size,
					"DataRcv                                 : ");
		}
	} else {
		LOG_ERR("p_rcv_data_pkt is NULL");
	}
}

void uwb_parse_ll_data_rcv_ntf(const uint8_t *const p, uint16_t len,
			       uwb_ll_data_receive_notification_t *p_rcv_data_pkt)
{
	uint16_t offset = 0;
	UWB_STREAM_TO_UINT32(p_rcv_data_pkt->llConnectId, p, offset);
	UWB_STREAM_TO_UINT16(p_rcv_data_pkt->sequence_number, p, offset);
	UWB_STREAM_TO_UINT16(p_rcv_data_pkt->data_size, p, offset);
	if (p_rcv_data_pkt->data) {
		UWB_STREAM_TO_ARRAY(p_rcv_data_pkt->data, p, p_rcv_data_pkt->data_size, offset);
	}
}

void uwb_print_ll_data_rcv_ntf(const uwb_ll_data_receive_notification_t *p_rcv_data_pkt)
{
	if (p_rcv_data_pkt != NULL) {
		LOG_DBG("p_rcv_data_pkt->llConnectId              : 0x%X",
			p_rcv_data_pkt->llConnectId);
		LOG_DBG("p_rcv_data_pkt->sequence_number          : %d",
			p_rcv_data_pkt->sequence_number);
		LOG_INF("p_rcv_data_pkt->data_size                : %d", p_rcv_data_pkt->data_size);
		LOG_HEXDUMP_DBG(p_rcv_data_pkt->data, p_rcv_data_pkt->data_size,
				"DataRcv                                 : ");
	} else {
		LOG_ERR("p_rcv_data_pkt is NULL");
	}
}
