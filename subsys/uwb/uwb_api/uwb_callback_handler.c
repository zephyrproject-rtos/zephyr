/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/uwb/uwb.h"
#include "zephyr/uwb/types.h"
#include "zephyr/uwb/uwb_helpers.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(uwb_callback, CONFIG_UWB_LOG_LEVEL);

#define UWB_NTF_HANDLER_TASK_NAME "demo_ntf_handling_task"

#ifndef CONFIG_UWB_NOTIFICATION_CALLBACK_TASK_SIZE
#define CONFIG_UWB_NOTIFICATION_CALLBACK_TASK_SIZE 10000
#endif /* CONFIG_UWB_NOTIFICATION_CALLBACK_TASK_SIZE */

#ifndef CONFIG_UWB_NOTIFICATION_HANDLER_TASK_PRIO
#define CONFIG_UWB_NOTIFICATION_HANDLER_TASK_PRIO 5
#endif /* CONFIG_UWB_NOTIFICATION_HANDLER_TASK_PRIO */

UWB_DECLARE_QUEUE(g_demo_ntf_mng_queue, 20)
static k_tid_t g_demo_ntf_handler_task_handle;
K_THREAD_STACK_DEFINE(demo_ntf_handling_task_stack, CONFIG_UWB_NOTIFICATION_CALLBACK_TASK_SIZE);

static struct k_thread callback_thread;
static bool g_is_suspended = false;

static void uwb_handle_data_ntf(const uint8_t *const p_packet, const uint32_t packet_len)
{
	if ((NULL == p_packet) || (packet_len <= UCI_HEADER_SIZE)) {
		return;
	}

	uint16_t len = (uint16_t)(p_packet[3]) | ((uint16_t)p_packet[2] << 8);

	const uint16_t opType = (((uint16_t)(p_packet[0] & 0x0F)) << 8) | (uint16_t)p_packet[1];

	switch (opType) {
	case kGidOid_DataMessageRecv: {
		uwb_data_receive_notification_t pRcvDataPkt = {0};
		uwb_parse_data_rcv_ntf(p_packet, (uint16_t)len, &pRcvDataPkt);
		uwb_print_data_rcv_ntf(&pRcvDataPkt);
	} break;

	case kGidOid_LLDataMessageRecv: {
		uwb_ll_data_receive_notification_t pRcvDataPkt = {0};
		uwb_parse_ll_data_rcv_ntf(p_packet, (uint16_t)len, &pRcvDataPkt);
		uwb_print_ll_data_rcv_ntf(&pRcvDataPkt);
	} break;

	default:
		LOG_WRN("Unknown data message received");
		break;
	}
	k_free((void *)p_packet);
}

static void uwb_handle_ntf(const uint8_t *const p_packet, const uint32_t packet_len)
{
	if ((NULL == p_packet) || (packet_len <= UCI_HEADER_SIZE)) {
		return;
	}

	uci_control_packet_header_t *packet_header = (uci_control_packet_header_t *)p_packet;

	uint8_t *pData = (uint8_t *)&p_packet[UCI_HEADER_SIZE];

	uint16_t len = (uint16_t)(p_packet[3]) | ((uint16_t)p_packet[2] << 8);

	const uint16_t opType = (((uint16_t)(p_packet[0] & 0x0F)) << 8) | (uint16_t)p_packet[1];

	if (UCI_MT_DATA == packet_header->mt) {
		uwb_handle_data_ntf(p_packet, len);
		return;
	}

	switch (opType) {
	case kGidOid_SessionStart: {
		/* Action not yet implemented */
		break;
	}

	case kGidOid_CoreGenricErrorNtf: {
		LOG_ERR("Generic error : 0x%02X", *pData);
	} break;

	case kGidOid_SessionStatus: {
		uwb_session_status_notification_t *pSessionInfo =
			(uwb_session_status_notification_t *)pData;
		uwb_print_session_status_ntf(pSessionInfo);
	} break;

	case kGidOid_SessionUpdateControllerMulticastList: {
		uwb_session_update_controller_multicast_list_notification_t *pControleeNtfContext =
			(uwb_session_update_controller_multicast_list_notification_t *)pData;
		uwb_print_session_update_ctrl_multicast_list_ntf(pControleeNtfContext);
	} break;

	case kGidOid_SessionTransmitStatusNtf: {
		uwb_data_transmit_notification_t *pTransmitNtfContext =
			(uwb_data_transmit_notification_t *)pData;
		uwb_print_data_transmit_status_ntf(pTransmitNtfContext);
	} break;

	case kGidOid_SessionDataTransferPhaseConfig: {
		uwb_data_transfer_phase_config_ntf_t *pDataTxPhCfgNtf =
			(uwb_data_transfer_phase_config_ntf_t *)pData;
		uwb_print_data_transfer_phase_config_ntf(pDataTxPhCfgNtf);
	} break;

	case kGidOid_SessionLlCreate: {
		uwb_logical_link_create_ntf_t *pLLCreateNtf =
			(uwb_logical_link_create_ntf_t *)pData;
		uwb_print_ll_create_ntf(pLLCreateNtf);
	} break;

	case kGidOid_SessionLlUwbsClose: {
		uwb_logical_link_uwbs_close_ntf_t *pLLCloseNtf =
			(uwb_logical_link_uwbs_close_ntf_t *)pData;
		uwb_print_ll_uwbs_close_ntf(pLLCloseNtf);
	} break;

	case kGidOid_SessionLlUwbsCreate: {
		uwb_logical_link_uwbs_create_ntf_t *pLLUwbsCreateNtf =
			(uwb_logical_link_uwbs_create_ntf_t *)pData;
		uwb_print_ll_uwbs_create_ntf(pLLUwbsCreateNtf);
	} break;

	case kGidOid_SessionRoleChangeNtf: {
		uwb_session_role_change_ntf_t *pNewRole = (uwb_session_role_change_ntf_t *)pData;
		uwb_print_session_role_change_ntf(pNewRole);
	} break;

	default:
		LOG_DBG("%s : Unregistered Event : 0x%X ", __FUNCTION__, opType);
		break;
	}
	/* Free pData Memory*/
	k_free((void *)p_packet);
}

static void uwb_ntf_handling_task(void *args)
{
	LOG_DBG("Started uwb_ntf_handling_task");
	/* main loop */
	while (1) {
		uwb_message_t evt = {0};

		if (0 != k_msgq_get(&g_demo_ntf_mng_queue, &evt, K_FOREVER)) {
			LOG_DBG("%s : msgrcv timeout!!!", __FUNCTION__);
			continue;
		}
		uwb_handle_ntf(evt.pMsgData, evt.Size);
	}
}

int uwb_ntf_handler_start(void)
{
	if (false == g_is_suspended) {
		/* Task not created */
		k_msgq_init(&g_demo_ntf_mng_queue, UWB_QUEUE_BUFFER_HANDLE(g_demo_ntf_mng_queue),
			    sizeof(uwb_message_t), 20);

		int pthread_create_status = 0;

		g_demo_ntf_handler_task_handle = k_thread_create(
			&callback_thread, (k_thread_stack_t *)&demo_ntf_handling_task_stack,
			K_THREAD_STACK_SIZEOF(demo_ntf_handling_task_stack),
			(k_thread_entry_t)&uwb_ntf_handling_task, NULL, NULL, NULL,
			K_PRIO_PREEMPT(CONFIG_UWB_NOTIFICATION_HANDLER_TASK_PRIO), 0, K_NO_WAIT);
		if (NULL == g_demo_ntf_handler_task_handle) {
			pthread_create_status = -1;
		}

		return pthread_create_status;
	} else {
		/* Resume task */
		k_msgq_purge(&g_demo_ntf_mng_queue);
		k_thread_resume(g_demo_ntf_handler_task_handle);
		g_is_suspended = false;
		return 0;
	}
}

void uwb_ntf_handler_stop(void)
{
	k_thread_suspend(g_demo_ntf_handler_task_handle);
	k_msgq_purge(&g_demo_ntf_mng_queue);
	g_is_suspended = true;
}

void uwb_ntf_callback_handler(const uint8_t *const packet, const uint32_t len)
{
	if ((NULL == packet) || (len <= UCI_HEADER_SIZE)) {
		return;
	}

	const uint8_t *pData = packet;

	uwb_message_t uwb_message = {0};

	uwb_message.eMsgType = 0;
	uwb_message.Size = (uint16_t)len;
	uwb_message.pMsgData = (void *)k_malloc(uwb_message.Size * sizeof(uint8_t));
	if (uwb_message.pMsgData != NULL) {
		memcpy((uint8_t *)uwb_message.pMsgData, pData, uwb_message.Size);
		k_msgq_put(&g_demo_ntf_mng_queue, &uwb_message, K_FOREVER);
	} else {
		LOG_ERR("uwb_ntf_callback_handler: Unable to Allocate Memory of %d, Memory Full:\n",
			uwb_message.Size);
	}
}
