/*
 * Copyright (c) 2026 Deere & Company
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/canbus/j1939.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/drivers/can.h>
#include "J1939Ac.h"
#include "J1939Tp.h"
#include "J1939Timer.h"
#include <Can_Transmit.h>
#include <string.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(j1939, CONFIG_J1939_LOG_LEVEL);

struct enriched_can_frame {
	const struct device *can_dev;
	struct can_frame frame;
};

K_MSGQ_DEFINE_STATIC_TYPE(j1939_can_msgq, struct enriched_can_frame, CONFIG_J1939_CAN_MSGQ_SIZE);

#ifdef J1939_ENABLE_RECEIVED_PGN_SUPPORT
typedef struct j1939_receive_pgn_handler_s {
	j1939_pgn_t pgn;
	j1939_received_pgn_callback_t handler;
} j1939_receive_pgn_handler_t;
#endif

static j1939_timer_t j1939_last_current_time;

#ifdef J1939_ENABLE_RECEIVED_PGN_SUPPORT
static j1939_receive_pgn_handler_t j1939_received_pgn_handlers[CONFIG_J1939_NODES_COUNT]
								  [J1939_RECEIVED_PGN_LIST_MAX];
static j1939_counter_t j1939_received_pgn_record_index[CONFIG_J1939_NODES_COUNT];
#endif

#if defined(J1939DM1_RECEIVE) || defined(J1939DM2_RECEIVE)
static j1939_timer_t j1939_update_dtc_timer;
#endif
#ifdef J1939_PERIODIC_ROUTINE
static j1939_timer_t j1939_periodic_routine_timer;
#endif

/**
 * @brief Determine by which means the message should be routed. Keeps the rest of the code cleaner
 * without having to be aware of loopback state
 * @param message Message to be processed
 * @param can_dev CAN device the message was received on
 * @return True if CAN will not free memory, false if CAN will free memory
 */
static bool j1939_route_can_messages(const struct can_frame *message, const struct device *can_dev);

/** @brief CAN RX callback entrypoint used by can_add_rx_filter(). */
static void j1939_rx_filter_callback(const struct device *dev, struct can_frame *frame,
					 void *user_data);

/** @brief Thread entry for periodic J1939 processing. */
static void j1939_task_thread(void *arg1, void *arg2, void *arg3);

/** @brief Returns true when this node is the first node configured on its CAN bus. */
static bool j1939_is_first_node_on_bus(size_t node_index);

#if defined(J1939_LOOPBACK_ENABLE)
/**
 * @brief When LoopBack feature is enabled, this function is used by the callback which will
 * process received message and pass the information to the proper processing routines when the
 * messages are dequeued.
 * @param message Message to be processed
 * @param can_dev CAN device the message was received on
 * @return True if CAN will not free memory, false if CAN will free memory
 */
static inline bool j1939_route_with_loopback(const struct can_frame *message,
						 const struct device *can_dev);
#else
/**
 * @brief When LoopBack feature is disabled, this function is used by the callback which will
 * process received message and pass the information to the proper processing routines when the
 * messages are dequeued.
 * @param message Message to be processed
 * @param can_dev CAN device the message was received on
 * @return True if CAN will not free memory, false if CAN will free memory
 */
static inline bool j1939_route_without_loopback(const struct can_frame *message,
						const struct device *can_dev);
#endif

/**
 * @brief Runs the specific funciton in J1939_App_RoutingTable for the PGN in the current message
 * @param message Message to be processed
 * @param pf PF of message being processed
 * @param node J1939 node associated with the message being processed
 * @return True if CAN will not free the memory, false if CAN will free message memory
 */
static inline bool j1939_run_message_callback(const struct can_frame *message,
						  j1939_pdu_format_t pf, j1939_node_t node);

K_THREAD_STACK_DEFINE(j1939_task_thread_stack, CONFIG_J1939_STACK_SIZE);
static struct k_thread j1939_task_thread_data;

/**************************************************************************************************/
void j1939_init(void)
{
	/** Nodes are initialized to off view the J1939_NODE_DEFINE struct */

	/* Set our initial number of PGN request messages to zero */
	STRUCT_SECTION_FOREACH(j1939_node_cfg, node) {
		node->j1939_requested_pgn_count = 0;
		for (uint8_t index = 0U; index < CONFIG_J1939_MAX_PGN_REQUEST_MESSAGES; index++) {
			node->j1939_pgn_request_list[index].isUsed = false;
			node->j1939_pgn_request_list[index].source = J1939_GLOBAL_ADDRESS;
			node->j1939_pgn_request_list[index].pgn = (j1939_pgn_t)0;
			node->j1939_pgn_request_list[index].isRequested = false;
		}

#ifdef J1939_ENABLE_RECEIVED_PGN_SUPPORT
		for (j1939_counter_t index = 0; index < J1939_RECEIVED_PGN_LIST_MAX; index++) {
			node->j1939_received_pgn_handlers[index].pgn = 0;
			node->j1939_received_pgn_handlers[index].handler = NULL;
		}

		node->j1939_received_pgn_record_index = 0;
#endif
	}

#ifdef CONFIG_J1939_NAME_MANAGEMENT
	j1939_nm_init();
#endif

	/* Initialize Address claimed. Whether we do or not we must still init! */
	j1939_ac_init();

#ifdef CONFIG_J1939_TRANSPORT_PROTOCOL
	/* Initialize transport protocol. */
	j1939_tp_init();
#endif

#ifdef J1939_MEMORY_ACCESS
	/* Initialize Memory Access */
	j1939_ma_init();
#endif

#ifdef J1939_DIAGNOSTICS
	/* Initialize Diagnostics */
	j1939_diag_init();
#endif

#if defined(J1939DM4_TRANSMIT) || defined(J1939DM4_RECEIVE)
	j1939_dm4_init();
#endif

	/* Init the elapsed time counter.  The first call returns an invalid value, so we ignore it
	 */
	j1939_last_current_time = 0;
	(void)j1939_timer_elapse(&j1939_last_current_time);

	/* Initialize the timers for the periodic routine and the DTC counter */
#ifdef J1939_PERIODIC_ROUTINE
	j1939_periodic_routine_timer = 0;
#endif

#if defined(J1939DM1_RECEIVE) || defined(J1939DM2_RECEIVE)
	j1939_update_dtc_timer = 0;
#endif

	j1939_app_init();

	/* Initialize callback such that we get all messages when they are dequeued. Send to the */
	/* Routing function for distribution to the J1939 services. */
	struct can_filter filter = {.mask = 0, .flags = CAN_FILTER_IDE};

	/* TODO - I feel like there should be a compile time way to do this, leaving for now */
	/* loop through all j1939 nodes and register the callback for each physical bus */
	for (size_t node_index = 0; node_index < CONFIG_J1939_NODES_COUNT; node_index++) {
		struct j1939_node_cfg *node;

		STRUCT_SECTION_GET(j1939_node_cfg, node_index, &node);
		const struct device *can_dev = node->can_dev;

		if ((can_dev == NULL) || !j1939_is_first_node_on_bus(node_index)) {
			continue;
		}

		int filter_id = can_add_rx_filter(can_dev, j1939_rx_filter_callback, NULL, &filter);

		if (filter_id < 0) {
			LOG_ERR("add filter failed for %s: %d\n", can_dev->name, filter_id);
		}
	}

	k_tid_t tid = k_thread_create(&j1939_task_thread_data, j1939_task_thread_stack,
						K_THREAD_STACK_SIZEOF(j1939_task_thread_stack),
						j1939_task_thread, NULL, NULL, NULL,
						CONFIG_J1939_TASK_PRIORITY, 0, K_NO_WAIT);

	k_thread_name_set(tid, "j1939_task");
}

/**************************************************************************************************/
bool j1939_transmit_pgn_request(j1939_pgn_t pgn, j1939_destination_address_t destination,
				j1939_node_t node)
{
	uint8_t data[CAN_MAX_DLC] = {0};

	/* Need to make sure the PGN we request is valid */
	if (!j1939_is_pgn_valid(pgn)) {
		return false;
	}

	data[0] = LOBYTE(LOWORD(pgn));
	data[1] = HIBYTE(LOWORD(pgn));
	data[2] = LOBYTE(HIWORD(pgn));

	/* This is one of the few J1939 messages we can send out with only 3 bytes. */
	return j1939_transmit_pgn(J1939_Priority_6, J1939_REQUEST_PGN, destination, data, 3, node);
}

/**************************************************************************************************/
bool j1939_is_pgn_requested(j1939_pgn_t pgn, j1939_source_address_t *source, j1939_node_t node)
{
	/* Note: We don't need multi-thread protection here because of how
	 * j1939_register_request_pgn()
	 */
	/* is implemented */
	for (uint8_t index = 0; index < node->j1939_requested_pgn_count; index++) {
		if (index < CONFIG_J1939_MAX_PGN_REQUEST_MESSAGES) {
			if ((pgn == node->j1939_pgn_request_list[index].pgn) &&
				(node->j1939_pgn_request_list[index].isUsed)) {
				if (source) {
					*source = node->j1939_pgn_request_list[index].source;
				}

				if (node->j1939_pgn_request_list[index].isRequested) {
					/* we reset the request, because we assume the application
					 * will respond.
					 */
					node->j1939_pgn_request_list[index].isRequested = false;
					return true;
				}
				/* The PGN is in the list, but is not requested. */
				return false;
			}
		}
	}

	/* PGN is not in the list. It may be requested, it will either be nacked or automatically */
	/* sent out as data from the transmit list. */
	return false;
}

void j1939_process_rx_msgs(void)
{
	struct enriched_can_frame msg;

	while (k_msgq_get(&j1939_can_msgq, &msg, K_NO_WAIT) == 0) {
		(void)j1939_route_can_messages(&msg.frame, msg.can_dev);
	}
}

/**************************************************************************************************/
void j1939_task(void)
{
	/* Find amount of time which has gone by since last time this function was called. */
	j1939_timer_t elapsedCanTime = j1939_timer_elapse(&j1939_last_current_time);

	(void)elapsedCanTime;

	j1939_process_rx_msgs();

	j1939_ac_task();

#ifdef CONFIG_J1939_TRANSPORT_PROTOCOL
	/* The following two functions process the transport protocol. */
#ifndef CONFIG_J1939TP_RECEIVE_DISABLED
	j1939_tp_update_receive_message_times(elapsedCanTime);
#endif
	j1939_tp_update_send_message_times(elapsedCanTime);
#endif

	STRUCT_SECTION_FOREACH(j1939_node_cfg, node) {
#ifdef J1939_MEMORY_ACCESS
		j1939_ma_task(node);

#ifdef J1939MA_TRANSMIT_READ_WRITE_REQUEST
		j1939_ma_process_transmit_commands(node);
#endif
#endif

#ifdef J1939_DIAGNOSTICS
#ifdef J1939DM1_TRANSMIT
		j1939_dm1_transmit(node);
#endif

#ifdef J1939DM13_ENABLE
		j1939_dm13_process(node);
#endif
#endif
	}

#if defined(J1939DM1_RECEIVE)
	j1939_update_dtc_timer = (j1939_update_dtc_timer + elapsedCanTime);
	if (j1939_update_dtc_timer >= J1939DM1_DTC_UPDATE_MS) {
		j1939_dm1_update_active_dtc_table(j1939_update_dtc_timer);
		j1939_update_dtc_timer = 0;
	}
#endif

	j1939_source_address_t requestedSource;

	STRUCT_SECTION_FOREACH(j1939_node_cfg, node) {
		if (j1939_is_pgn_requested(J1939_ECU_ID_INFO_PGN, &requestedSource, node)) {
			j1939_app_transmit_ecu_id((j1939_destination_address_t)requestedSource,
						  node);
		}
		if (j1939_is_pgn_requested(J1939_SOFTWARE_ID_PGN, &requestedSource, node)) {
			j1939_app_transmit_software_id((j1939_destination_address_t)requestedSource,
							   node);
		}
#ifdef CONFIG_J1939_NAME_MANAGEMENT
		if (j1939_is_pgn_requested(J1939NM_PGN, &requestedSource, node)) {
			j1939_nm_pgn_request_handler((j1939_destination_address_t)requestedSource,
							 node);
		}
#endif
	}

#ifdef J1939_TRANSMIT_QUEUE_HANDLING
	Can_Transmit_SendQueues();
#endif
}

/**************************************************************************************************/
void j1939_acknowledge(const j1939_pgn_t pgn, j1939_response_t control,
			   const j1939_destination_address_t destination, j1939_node_t node)
{
	j1939_arbitration_t id;
	j1939_source_address_t source = node->source_address;
	uint8_t data[CAN_MAX_DLC];

	/* Build the response ID */
	id = j1939_build_message_id(
		0, 0, J1939_Priority_6,
		j1939_build_pgn_from_pdu(J1939_PGN_ACK_PF, (j1939_pdu_specific_t)destination),
		source);

	/* Build the response message */
	data[0] = (uint8_t)control;
	data[1] = J1939_BYTE_UNAVAILABLE;
	data[2] = J1939_BYTE_UNAVAILABLE;
	data[3] = J1939_BYTE_UNAVAILABLE;
	data[4] = J1939_BYTE_UNAVAILABLE;
	data[5] = LOBYTE(LOWORD(pgn));
	data[6] = HIBYTE(LOWORD(pgn));
	data[7] = LOBYTE(HIWORD(pgn));

	/* Send out the message */
	(void)j1939_build_and_queue_message(node, id, 8, true, data);
}

/**************************************************************************************************/
bool j1939_flag_pgn_request(j1939_pgn_t pgn, j1939_source_address_t source, j1939_node_t node)
{
	bool result = false;

	for (uint8_t index = 0; index < node->j1939_requested_pgn_count; index++) {
		if (index < CONFIG_J1939_MAX_PGN_REQUEST_MESSAGES) {
			if ((pgn == node->j1939_pgn_request_list[index].pgn) &&
				node->j1939_pgn_request_list[index].isUsed) {
				node->j1939_pgn_request_list[index].source = source;
				node->j1939_pgn_request_list[index].isRequested = true;
				result = true;
				break;
			}
		}
	}

	return result;
}

/**************************************************************************************************/
bool j1939_register_request_pgn(j1939_pgn_t pgn, j1939_node_t node)
{
	j1939_counter_t index;
	bool result = false;
	bool setCurrentEntry = false;

	if (!j1939_is_pgn_valid(pgn)) {
		/* Invalid PGN */
		return false;
	}

	for (index = 0; index < CONFIG_J1939_MAX_PGN_REQUEST_MESSAGES; index++) {
		/* Find if the PGN is in the list and update it or put it in the first unused space.
		 */
		if (pgn == node->j1939_pgn_request_list[index].pgn) {
			setCurrentEntry = true;
			break;
		} else if (!node->j1939_pgn_request_list[index].isUsed) {
			/* We need to add a PGN to the list, increment the count. */
			node->j1939_requested_pgn_count++;
			setCurrentEntry = true;
			break;
		}
	}

	/* Update the entry at the current index value.  Note that we update all three fields */
	/* regardless of if this is an update or a new entry.  This is probably not necessary in */
	/* the case of an update, but we left it that way to preserve behavior just in case */
	/* someone was relying on it for something */
	if (setCurrentEntry && (index < CONFIG_J1939_MAX_PGN_REQUEST_MESSAGES)) {
		node->j1939_pgn_request_list[index].pgn = pgn;
		node->j1939_pgn_request_list[index].isUsed = true;
		node->j1939_pgn_request_list[index].isRequested = false;
		result = true;
	}

	/* No room in list */
	return result;
}

/**************************************************************************************************/
bool j1939_transmit_pgn(j1939_priority_t priority, j1939_pgn_t pgn,
			j1939_destination_address_t destination, uint8_t *data,
			j1939_counter_t count, j1939_node_t node)
{
	j1939_arbitration_t id;
	j1939_pgn_t tempPgn = pgn;
	bool dataPage;
	bool extendedDataPage;
	bool result;

	(void)data;
	(void)tempPgn; /* Not used with some configurations, silence compiler warning */

	/* The PGN should not have a destination address embedded in it. */
	if (j1939_is_pgn_valid(pgn)) {
		/* If PF < 240(0xF0), we must make sure the Id contains the destination */
		/* address information, or add it in.  We have already checked the */
		/* passed in PGN to make sure it is a valid PGN. */
		if ((pgn & 0xFF00) < 0xF000) {
			/* Add in the destination information for our message ID only if */
			/* eight or less bytes of data to send. */
			pgn = pgn + (j1939_pgn_t)destination;
		}

		/* Extract the Data Page bits from the PGN */
		dataPage = j1939_get_data_page_from_pgn(pgn);
		extendedDataPage = j1939_get_extended_data_page_from_pgn(pgn);

		/* First check how many bytes to send.  If more than 8 we use transport */
		/* protocol.  Otherwise we queue it up. */
		if (count <= CAN_MAX_DLC) {
			/* Create the identifier for the message */
			id = j1939_build_message_id(dataPage, extendedDataPage, priority, pgn,
							node->source_address);

			/* Send the message out to the appropriate node. */
			result = j1939_build_and_queue_message(node, id, count, true, data);

#ifdef CONFIG_J1939_TRANSPORT_PROTOCOL
			/* Attempt to release the buffer in case the caller allocated a transport
			 * buffer
			 */
			/* Release the transport buffer. */
			j1939_tp_free_buffer(data);
#endif
		} else {
#ifdef CONFIG_J1939_TRANSPORT_PROTOCOL
			/* Send out the message via transport protocol. */
			if (j1939_tp_message_accepted ==
				j1939_tp_transmit_multi_packet(tempPgn, destination, count, data,
							   node)) {
				result = true;
			} else {
				result = false;
			}

			/* Release the transport buffer. */
			j1939_tp_free_buffer(data);
#else
			/* No transport layer, cannot send multipacket. Drop down and return false.
			 */
			result = false;
#endif
		}
	} else {
#ifdef CONFIG_J1939_TRANSPORT_PROTOCOL
		/* Attempt to release the buffer in case the caller allocated a transport buffer */
		/* Release the transport buffer. */
		j1939_tp_free_buffer(data);
#endif

		/* Invalid PGN - We should not transmit an invalid PGN */
		result = false;
	}

	return result;
}

/**************************************************************************************************/
bool j1939_request_pgn(const struct can_frame *message, j1939_node_t node)
{
	j1939_pgn_t pgn;
	j1939_source_address_t requestSource;
	j1939_pdu_specific_t ps;
	j1939_pdu_format_t pf;
	j1939_source_address_t source;

	if (message) {
		ps = j1939_get_pdu_specific(message->id);
		pf = j1939_get_pdu_format(message->id);
		source = j1939_get_source_address(message->id);

		/* Determine the source address, if it was sent globally then set the source address
		 * to global
		 */
		if (ps == J1939_GLOBAL_ADDRESS) {
			requestSource = J1939_GLOBAL_ADDRESS;
		} else {
			requestSource = source;
		}

		/* Build up the ID. */
		pgn = MAKEDWORD(MAKEWORD(message->data[0], message->data[1]),
				MAKEWORD(message->data[2], 0));

		switch (pgn) {
		case J1939_ADDRESS_CLAIMED_PGN:
			/* Handle the address claim request */
			j1939_ac_process_request(node);
			break;

#ifdef J1939DM1_TRANSMIT
		case J1939_DM1_PGN:
			/* Send DM1 active errors */
			j1939_dm1_response(source, ps, message->node);
			break;
#endif

#ifdef J1939DM2_TRANSMIT
		case J1939_DM2_PGN:
			/* Send DM2 prev active dtcs. */
			j1939_dm2_process_request(source, ps, message->node);
			break;
#endif

#ifdef J1939DM3_ENABLE
		case J1939_DM3_PGN:
			/* Clear prev active dtcs & send Ack. */
			j1939_dm3_task(message);
			break;
#endif

#if defined(J1939DM4_TRANSMIT)
		case J1939_DM4_PGN:
			j1939_dm4_process_request(source, ps, message->node);
			break;
#endif

#ifdef J1939DM5_ENABLE
		case J1939_DM5_PGN:
			/* Diagnostic readiness. Number of active and prev active dtcs etc... */
			j1939_dm5_process(source, message->node);
			break;
#endif

#ifdef J1939DM11_ENABLE
		case J1939_DM11_PGN:
			/* clear active dtcs & send Ack. */
			j1939_dm11_task(message);
			break;
#endif

		default:
			/* Check if the PGN is in the request list. If so set the boolean for the
			 * PGN request to true.
			 */
			if (!j1939_flag_pgn_request(pgn, requestSource, node)) {
				/* The PGN was not in the request list. If we are here, we do not
				 * support the request. J1939 says we should Nack the PGN request
				 * if it was not to a global destination address.
				 */
				if (!((pf >= J1939_PDUF_240) || (ps == J1939_GLOBAL_ADDRESS))) {
					j1939_acknowledge(pgn, J1939_Response_Nack,
							  (j1939_destination_address_t)source,
							  node);
				}
			}
			break;
		}
	}

	/* Allow driver to handle message pointer. */
	return false;
}

/**************************************************************************************************/
bool j1939_is_pgn_valid(j1939_pgn_t pgn)
{
	return (!(((pgn & 0xFF00) < 0xF000) && (pgn & 0x00FF)));
}

/**************************************************************************************************/
static inline bool J1939_IsMessageForMe(const struct can_frame *message, j1939_node_t node)
{
	bool result = false;
	j1939_pdu_specific_t ps;
	j1939_pdu_format_t pf;

	__ASSERT_NO_MSG(message != NULL);

	ps = j1939_get_pdu_specific(message->id);
	pf = j1939_get_pdu_format(message->id);

	if ((pf >= J1939_PDUF_240) || (ps == J1939_GLOBAL_ADDRESS)) {
		result = true;
	} else if ((j1939_ac_get_state(node) == J1939_AC_STATE_CLAIMED) &&
		   ((j1939_pdu_specific_t)j1939_ac_get_source_address(node) == ps)) {
		/* ignore messages to our source address until we have successfully claimed it */
		result = true;
	} else {
		/* message not for node*/
		result = false;
	}

	return result;
}

/**************************************************************************************************/
static bool j1939_route_can_messages(const struct can_frame *message, const struct device *can_dev)
{
	/*lint -esym(838, result) ignore issues with not using previous set version of this
	 * variable
	 */
	bool result = false; /* Allow Driver to free the message memory by default */

#if defined(J1939_LOOPBACK_ENABLE)
	result = j1939_route_with_loopback(message, can_dev);
#else
	result = j1939_route_without_loopback(message, can_dev);
#endif

	return result;
}

/**************************************************************************************************/
static void j1939_rx_filter_callback(const struct device *dev, struct can_frame *frame,
					 void *user_data)
{
	(void)user_data;

	/* queue message */
	struct enriched_can_frame enriched_frame = {
		.can_dev = dev,
		.frame = *frame,
	};
	(void)k_msgq_put(&j1939_can_msgq, &enriched_frame, K_NO_WAIT);
}

/**************************************************************************************************/
static bool j1939_is_first_node_on_bus(size_t node_index)
{
	for (size_t index = 0; index < node_index; index++) {
		struct j1939_node_cfg *new_node, *old_node;

		STRUCT_SECTION_GET(j1939_node_cfg, index, &old_node);
		STRUCT_SECTION_GET(j1939_node_cfg, node_index, &new_node);
		if (old_node->can_dev == new_node->can_dev) {
			return false;
		}
	}

	return true;
}

/**************************************************************************************************/
bool j1939_default_route(const struct can_frame *message, j1939_node_t node)
{
#ifdef J1939_ENABLE_RECEIVED_PGN_SUPPORT

	__ASSERT_NO_MSG(message != NULL);

	/* Extract the PGN */
	j1939_pgn_t pgn = j1939_get_pgn(message->id);

	/* Search through the list and see if the PGN is in the list */
	/* NOTE: Multi-thread protection not needed because of how
	 * j1939_register_receive_pgn_callback()
	 */
	/* is implemented */
	for (j1939_counter_t index = 0; index < j1939_received_pgn_record_index[node]; index++) {
		if (pgn == j1939_received_pgn_handlers[node][index].pgn) {
			if (j1939_received_pgn_handlers[node][index].handler) {
				(void)j1939_received_pgn_handlers[node][index].handler(message);
			}
		}
	}
#else
	/* Suppress any warnings. */
	(void)message;
#endif

	/* Allow driver to handle message pointer. */
	return false;
}

/**************************************************************************************************/
bool j1939_pdu_f254_process(const struct can_frame *message, j1939_node_t node)
{
#if defined(J1939DM1_RECEIVE) || defined(J1939DM2_RECEIVE) || defined(J1939DM4_RECEIVE)
	j1939_pdu_specific_t ps;
	j1939_source_address_t source;

	__ASSERT_NO_MSG(message != NULL);

	if (message && (message->node < CAN_NUM_NODES)) {
		ps = j1939_get_pdu_specific(message->arbitration);
		source = j1939_get_source_address(message->arbitration);

		/* These variables are not used in all cases so we need to suppress a warning */
		(void)ps;
		(void)source;

#ifdef J1939DM1_RECEIVE
		if (ps == J1939_ACTIVE_DTC_PS) {
			/* Send address and message data to DM1 processing function. */
			j1939_dm1_receive(source, (can_data_byte_t *)message->data, 6,
					  message->node);

#ifndef J1939_PASSTHROUGH_FOR_254_MESSAGES
			return false;
#endif
		}
#endif

#ifdef J1939DM2_RECEIVE
		if (ps == J1939_PREVIOUS_ACTIVE_PS) {
			j1939_dm2_process_received(source, (can_data_byte_t *)message->data, 6,
						   message->node);

#ifndef J1939_PASSTHROUGH_FOR_254_MESSAGES
			return false;
#endif
		}
#endif

#if defined(J1939DM4_RECEIVE)
		if (ps == J1939_FREEZE_FRAME_PS) {
			/* Send address and message data to DM4 processing function. */
			j1939_dm4_process_received(source, message->data, message->length,
						   message->node);

#ifndef J1939_PASSTHROUGH_FOR_254_MESSAGES
			return false;
#endif
		}
#endif
	}

#endif

	/* Let the application specific routines handle it */
	/*lint -esym(613, message)*/
	return j1939_app_pf254_process(message, node);
}

#ifdef J1939_ENABLE_RECEIVED_PGN_SUPPORT
/**************************************************************************************************/
bool j1939_register_receive_pgn_callback(j1939_pgn_t pgn, j1939_received_pgn_callback_t handler,
					 j1939_node_t node)
{
	bool result = false;

	if (handler != NULL) {
		if (j1939_received_pgn_record_index[node] < J1939_RECEIVED_PGN_LIST_MAX) {
			j1939_received_pgn_handlers[node][j1939_received_pgn_record_index[node]]
				.pgn = pgn;
			j1939_received_pgn_handlers[node][j1939_received_pgn_record_index[node]]
				.handler = handler;
			j1939_received_pgn_record_index[node]++;
			result = true;
		}
	}

	return result;
}
#endif

/**************************************************************************************************/
j1939_pgn_t j1939_get_pgn(j1939_arbitration_t messageId)
{
	j1939_pgn_t result;

	/* Per the J1939 standard, a PGN is a 24-bit value with the bits defined as: */
	/* [0-7] Group Extension */
	/* [8-15] PDU Format */
	/* [16] Data Page */
	/* [17] Extended Data Page */
	/* [18-23] Zero */

	/* Extract the lower 18 bits */
	result = ((messageId >> 8) & 0x3FFFF);

	/* If the PF is less than 0xF0, then the group extension should be set to zero */
	if (j1939_get_pdu_format(result) < J1939_PDUF_240) {
		result &= 0x3FF00;
	}

	return result;
}

/**************************************************************************************************/
j1939_pgn_t j1939_build_pgn(bool extendedDataPage, bool dataPage, j1939_pdu_format_t pduf,
				j1939_group_extension_t groupExtension)
{
	j1939_pgn_t result;

	result = MAKEWORD(groupExtension, pduf);
	result |= (((uint32_t)dataPage) << 16);
	result |= (((uint32_t)extendedDataPage) << 17);

	if (pduf < J1939_PDUF_240) {
		result &= 0x3FF00;
	}

	return result;
}

/**************************************************************************************************/
void j1939_enable_virtual_mode_transmit(j1939_node_t node)
{
	node->transmission_enabled = true;
}

/**************************************************************************************************/
void j1939_disable_virtual_mode_transmit(j1939_node_t node)
{
	node->transmission_enabled = false;
}

/**************************************************************************************************/
bool j1939_is_virtual_node_transmit_enabled(j1939_node_t node)
{
	return node->transmission_enabled;
}

/**************************************************************************************************/
bool j1939_build_and_queue_message(j1939_node_t node, j1939_arbitration_t arbitration,
				   uint16_t dataLength, bool isExtendedMessage, const uint8_t *data)
{
	bool result = false;

	result = Can_Transmit_BuildAndQueueMessage(node->can_dev, arbitration, (uint16_t)dataLength,
						   isExtendedMessage, false, data);
	return result;
}

#if defined(J1939_LOOPBACK_ENABLE)
/**************************************************************************************************/
static inline bool j1939_route_with_loopback(const struct can_frame *message)
{
	/* Msg object to replace physical node to address node when running handler */
	struct can_frame msg;
	bool result = false;
	j1939_node_t addressNode;
	uint16_t physicalNode;
	j1939_pdu_format_t pf = j1939_get_pdu_format(message->arbitration);
	j1939_pdu_specific_t ps = j1939_get_pdu_specific(message->arbitration);

	j1939_node_t loopbackNode;

	/* Is this a broadcast message? */
	if (pf >= J1939_PDUF_240 || ps == J1939_GLOBAL_ADDRESS) {
		/* Get address node that is sending loopBack message */
		if (message->TransmitLoopbackMessage) {
			loopbackNode = j1939_get_j1939_node_from_source_address(
				j1939_get_source_address(message->arbitration));
			physicalNode = j1939_get_can_node(loopbackNode);
		} else {
			/* If this is not a loopback message set values to transmit to all
			 * addressNodes
			 */
			loopbackNode = CONFIG_J1939_NODES_COUNT + 1;
			physicalNode = message->node;
		}
		/* Transmit to all address nodes */
		for (addressNode = 0; addressNode < CONFIG_J1939_NODES_COUNT; addressNode++) {
			/* Linked to the physical node of the loopback addressNode */
			if (j1939_get_can_node(addressNode) == physicalNode) {
				/* Except for the transmitting loopback not itself */
				if (addressNode != loopbackNode) {
					msg = *message;
					msg.node = addressNode;
					result = j1939_run_message_callback(&msg, pf);
				}
			}
		}
	} else if (j1939_get_j1939_node_from_source_address(ps) < CONFIG_J1939_NODES_COUNT) {
		/* Is a message for a specific address node? */
		msg = *message;
		msg.node = j1939_get_j1939_node_from_source_address(ps);
		result = j1939_run_message_callback(&msg, pf);
	} else {
		/* Not a message for any address node */
		result = false;
	}

	return result;
}
#else

static inline bool j1939_get_node(uint8_t sourceAddress, j1939_node_t *node)
{
	STRUCT_SECTION_FOREACH(j1939_node_cfg, search_node) {
		if (search_node->source_address == sourceAddress) {
			*node = search_node;
			return true;
		}
	}

	return false; /* Return false if not found */
}

/**************************************************************************************************/
static inline bool j1939_route_without_loopback(const struct can_frame *message,
						const struct device *can_dev)
{
	bool result = false;
	j1939_pdu_specific_t ps = j1939_get_pdu_specific(message->id);
	j1939_pdu_format_t pf = j1939_get_pdu_format(message->id);

	j1939_node_t node;

	/* Determine if the message is for us or global and what we should do with it. */
	if ((j1939_get_node(ps, &node))) {
		/* TODO - global messages are not handled, do we need to forward to all nodes?*/
		/* || (pf >= J1939_PDUF_240) || (ps == J1939_GLOBAL_ADDRESS))) { */
		result = j1939_run_message_callback(message, pf, node);
	}

	return result;
}
#endif

/**************************************************************************************************/
static inline bool j1939_run_message_callback(const struct can_frame *message,
						  j1939_pdu_format_t pf, j1939_node_t node)
{
	bool result = false;
	/* Call the function in the routing table.  This eliminates lots of checks with a case */
	/* statement since the call is made based on the PF of the message. */
#ifdef J1939_NEVER_EATS_MESSAGES
	/* If the J1939 layer doesn't eat messages, we get a compiler warning if we assign to */
	/* result, so we cast to void in that case to eliminate it */
	(void)J1939_App_RoutingTable[pf](message, node);
#else
	result = J1939_App_RoutingTable[pf](message, node);
#endif

	return result;
}

char *stpcpy(char *dest, const char *src)
{
	while ((*dest = *src)) {
		dest++;
		src++;
	}
	return dest;
}

void j1939_app_transmit_ecu_id(j1939_destination_address_t destination, j1939_node_t node)
{
	/* To minimize stack usage we allocate a buffer from the Transport Layer */
	/* The code below does not perform any range checking.  The application developer is
	 * responsible for allocating a buffer large enough to hold the largest possible string.
	 */

	/* Pointer to the beginning of the transport buffer */
	char *data = (char *)j1939_tp_get_buffer(150);

	if (!data) {
		return;
	}
	char *ptr = data;         /* Temporary pointer used to build up the string */

	j1939_device_info_t deviceInfo;

	j1939_app_get_device_info(&deviceInfo);

	ptr = stpcpy(ptr, deviceInfo.hardware_part_number);
	ptr = stpcpy(ptr, ",");
	ptr = stpcpy(ptr, deviceInfo.hardware_revision);
	ptr = stpcpy(ptr, "*");
	ptr = stpcpy(ptr, deviceInfo.hardware_serial_number);
	ptr = stpcpy(ptr, "*");
	/* ECU Location */
	ptr = stpcpy(ptr, "*");
	/* ECU Type */
	ptr = stpcpy(ptr, "*");
	/* ECU Manufacturer Name */
	ptr = stpcpy(ptr, "*");
	/* ECU Hardware ID */
	ptr = stpcpy(ptr, "*");

	/* Note that if j1939_transmit_pgn() succeeds, the transport layer will take care of
	 * freeing the buffer we allocated when the transport session is completed.
	 * If it fails, we need to take care of it ourselves
	 */
	if (!(j1939_transmit_pgn(J1939_Priority_6, J1939_ECU_ID_INFO_PGN, destination, data,
		(j1939_counter_t)(ptr - data), node))) {
		j1939_tp_free_buffer(data);
	}
} /* lint !e438 */

void j1939_app_transmit_software_id(j1939_destination_address_t destination, j1939_node_t node)
{
	/* To minimize stack usage we allocate a buffer from the Transport Layer */
	/* The code below does not perform any range checking.  The application developer is
	 * responsible
	 */
	/* for allocating a buffer large enough to hold the largest possible string. */
	uint8_t *data =
		j1939_tp_get_buffer(150); /* Pointer to the beginning of the transport buffer */

	if (!data) {
		return;
	}
	uint8_t *ptr = data;         /* Temporary pointer used to build up the string */

	/* Note there are a number of projects that have built these up properly from PCD
	 * information
	 */
	*ptr++ = 1;
	/* RV: version of s/w id that this Controller uses */
	/* Note that this matches the current JDOS/JDCBB format, but not the VNSM database
	 */
	ptr = stpcpy(ptr, "RV0001,,01.00");
	ptr = stpcpy(ptr, "#");

	j1939_get_software_id(&ptr);

	/* Note that if j1939_transmit_pgn() succeeds, the transport layer will take care of
	 * freeing the buffer we allocated when the transport session is completed.  If it fails,
	 * we need to take care of it ourselves
	 */
	if (!(j1939_transmit_pgn(
			J1939_Priority_6,      /* Priority. */
			J1939_SOFTWARE_ID_PGN, /* PGN. */
			destination,           /* Source Address to reply to */
			data,                  /* Data pointer. */
			(j1939_counter_t)(ptr - data), /* Byte counter for pointer. */
			node))) {               /* CAN Node. */
		j1939_tp_free_buffer(data);
	}
} /* lint !e438 */


static void j1939_task_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (true) {
		j1939_task();
		k_sleep(K_MSEC(CONFIG_J1939_TASK_INTERVAL_MS));
	}
}
