/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/uwb/uwb.h>
#include <zephyr/uwb/types.h>
#include <zephyr/uwb/uwb_helpers.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(uwb_callback, CONFIG_UWB_LOG_LEVEL);

#define UWB_NTF_HANDLER_TASK_NAME "demo_ntf_handling_task"

UWB_DECLARE_QUEUE(g_demo_ntf_mng_queue, 20)
static k_tid_t g_demo_ntf_handler_task_handle;
K_THREAD_STACK_DEFINE(demo_ntf_handling_task_stack, CONFIG_UWB_NOTIFICATION_CALLBACK_STACK_SIZE);

static struct k_thread callback_thread;

typedef struct {
	bool g_is_initialized;
} uwb_callback_context_t;

static uwb_callback_context_t g_uwb_cb_context = {0};

static void uwb_handle_data_ntf(const uint8_t *const p_packet, const uint32_t packet_len)
{
	if ((!p_packet) || (packet_len <= UCI_HEADER_SIZE)) {
		return;
	}

	uint16_t len = (uint16_t)(p_packet[3]) | ((uint16_t)p_packet[2] << 8);

	const uint16_t opType = (((uint16_t)(p_packet[0] & 0x0F)) << 8) | (uint16_t)p_packet[1];

	switch (opType) {
	case UCI_DATA_GID_OID_DATA_MESSAGE_RECV: {
		uwb_data_receive_notification_t pRcvDataPkt = {0};

		uwb_parse_data_rcv_ntf(p_packet, (uint16_t)len, &pRcvDataPkt);
		uwb_print_data_rcv_ntf(&pRcvDataPkt);
	} break;

	case UCI_DATA_GID_OID_LL_DATA_MESSAGE_RECV: {
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
	if ((!p_packet) || (packet_len <= UCI_HEADER_SIZE)) {
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
	case UCI_CONTROL_GID_OID_SESSION_START: {
		/* Action not yet implemented */
	} break;

	case UCI_CONTROL_GID_OID_CORE_GENERIC_ERROR_NTF: {
		LOG_ERR("Generic error : 0x%02X", *pData);
	} break;

	case UCI_CONTROL_GID_OID_SESSION_STATUS: {
		/* Action not yet implemented */
	} break;

	case UCI_CONTROL_GID_OID_SESSION_UPDATE_CONTROLLER_MULTICAST_LIST: {
		/* Action not yet implemented */
	} break;

	case UCI_CONTROL_GID_OID_SESSION_TRANSMIT_STATUS_NTF: {
		/* Action not yet implemented */
	} break;

	case UCI_CONTROL_GID_OID_SESSION_DATA_TRANSFER_PHASE_CONFIG: {
		/* Action not yet implemented */
	} break;

	case UCI_CONTROL_GID_OID_SESSION_LL_CREATE: {
		/* Action not yet implemented */
	} break;

	case UCI_CONTROL_GID_OID_SESSION_LL_UWBS_CLOSE: {
		/* Action not yet implemented */
	} break;

	case UCI_CONTROL_GID_OID_SESSION_LL_UWBS_CREATE: {
		/* Action not yet implemented */
	} break;

	case UCI_CONTROL_GID_OID_SESSION_ROLE_CHANGE_NTF: {
		/* Action not yet implemented */
	} break;

	default:
		LOG_DBG("%s : Unregistered Event : 0x%X ", __func__, opType);
		break;
	}
	/* Free pData Memory*/
	k_free((void *)p_packet);
}

static void uwb_ntf_handling_task(void *args)
{
	LOG_DBG("Started %s", __func__);
	/* main loop */
	while (1) {
		uwb_message_t evt = {0};

		if (0 != k_msgq_get(&g_demo_ntf_mng_queue, &evt, K_FOREVER)) {
			LOG_DBG("%s : msgrcv timeout!!!", __func__);
			continue;
		}
		uwb_handle_ntf(evt.pMsgData, evt.Size);
	}
}

int uwb_ntf_handler_start(void)
{
	if (!g_uwb_cb_context.g_is_initialized) {
		/* Task not created */
		k_msgq_init(&g_demo_ntf_mng_queue, UWB_QUEUE_BUFFER_HANDLE(g_demo_ntf_mng_queue),
			    sizeof(uwb_message_t), 20);

		int pthread_create_status = 0;

		g_demo_ntf_handler_task_handle = k_thread_create(
			&callback_thread, (k_thread_stack_t *)&demo_ntf_handling_task_stack,
			K_THREAD_STACK_SIZEOF(demo_ntf_handling_task_stack),
			(k_thread_entry_t)&uwb_ntf_handling_task, NULL, NULL, NULL,
			K_PRIO_PREEMPT(CONFIG_UWB_NOTIFICATION_HANDLER_THREAD_PRIO), 0, K_NO_WAIT);
		if (!g_demo_ntf_handler_task_handle) {
			pthread_create_status = -1;
		}
		g_uwb_cb_context.g_is_initialized = true;

		return pthread_create_status;
	}

	/* Resume task */
	k_msgq_purge(&g_demo_ntf_mng_queue);
	k_thread_resume(g_demo_ntf_handler_task_handle);
	return 0;
}

void uwb_ntf_handler_stop(void)
{
	k_thread_suspend(g_demo_ntf_handler_task_handle);
	k_msgq_purge(&g_demo_ntf_mng_queue);
}

void uwb_ntf_callback_handler(const uint8_t *const packet, const uint32_t len)
{
	if ((!packet) || (len <= UCI_HEADER_SIZE)) {
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
		LOG_ERR("%s: Unable to Allocate Memory of %d, Memory Full:\n", __func__,
			uwb_message.Size);
	}
}
