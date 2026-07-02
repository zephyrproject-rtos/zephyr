/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "zephyr/uwb/uci.h"
#include "zephyr/uwb/uwb_core.h"
#include "zephyr/uwb/tml.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(uwb_core, CONFIG_UWB_LOG_LEVEL);

#ifndef CONFIG_UCI_RESPONSE_TIMEOUT_MS
#define CONFIG_UCI_RESPONSE_TIMEOUT_MS 200
#endif /** CONFIG_UCI_RESPONSE_TIMEOUT_MS */

#ifndef CONFIG_UCI_CREDIT_NTF_TIMEOUT_MS
#define CONFIG_UCI_CREDIT_NTF_TIMEOUT_MS 500
#endif /** CONFIG_UCI_CREDIT_NTF_TIMEOUT_MS */

#define UCI_MAX_RETRIES 5

/**
 * @brief Structure representing a waiting packet in the response queue
 *
 * This structure is used to track packets that are waiting for responses
 * from the UWB device. It forms a linked list of pending responses.
 */
typedef struct uci_waiting_packet {
	uint8_t mt;                      /**< Message Type */
	uint8_t gid;                     /**< Group ID */
	uint8_t oid;                     /**< Opcode ID */
	uint8_t *response_buf;           /**< Buffer to store response data */
	uint32_t *response_len;          /**< Length of response data */
	struct k_sem *sem;               /**< Semaphore for synchronization */
	struct uci_waiting_packet *next; /**< Pointer to next waiting packet */
} uci_waiting_packets_t;

/**
 * @brief UCI transceiver context structure
 *
 * Global context maintaining the state of UCI transmission/reception,
 * including pending responses, synchronization primitives, and task handles.
 */
typedef struct {
	uci_waiting_packets_t *waiting_packets; /**< Linked list of waiting packets */
	uci_waiting_packets_t *response_packet; /**< Current response packet being processed */
	uint32_t num_waiting_responses;         /**< Count of pending responses */
	struct k_mutex mutex;                   /**< Mutex for thread-safe access to this context */
	struct k_sem response_semaphore;        /**< Semaphore for response signaling */
	bool isInitialized;                     /**< Initialization status flag */
	bool isSuspended;
	uwb_uci_callback_t *callback;          /**< Generic callback for unhandled packets */
	uint16_t max_data_packet_payload_size; /**< Maximum payload size of each data packet */
} uwb_uci_trx_context_t;

static uwb_uci_trx_context_t g_uci_context = {0};

/**
 * @brief Set the payload length in UCI header in big-endian format
 *
 * Converts the length from host byte order to little-endian format
 * and stores it in the UCI header structure.
 *
 * @param header Pointer to UCI header structure
 * @param length Payload length to set
 */
static inline void uci_set_length_le(uci_control_packet_header_t *header, uint16_t length)
{
	/* Convert from host to little endian */
	header->len_le = ((length & 0xFF) << 8) | ((length >> 8) & 0xFF);
}
static inline void uci_set_length_be(uci_control_packet_header_t *header, uint16_t length)
{
	/* Keeping it in big endian */
	header->len_le = length;
}

/**
 * @brief Add a waiting response to the pending response queue
 *
 * Creates a new waiting packet entry and adds it to the linked list of
 * packets waiting for responses. Thread-safe operation using mutex.
 *
 * @param mt Message Type to wait for
 * @param gid Group ID to wait for
 * @param oid Opcode ID to wait for
 * @param response_buf Buffer to store the response
 * @param response_len Pointer to store response length
 * @param sem Semaphore to signal when response arrives
 *
 * @return Pointer to created waiting packet structure, NULL on failure
 *
 * @note Allocates memory that must be freed when response is received or timeout occurs
 */
static uci_waiting_packets_t *uci_add_waiting_response(uint8_t mt, uint8_t gid, uint8_t oid,
						       uint8_t *response_buf,
						       uint32_t *response_len, struct k_sem *sem)
{
	uci_waiting_packets_t *response =
		(uci_waiting_packets_t *)k_malloc(sizeof(uci_waiting_packets_t));

	if (response == NULL) {
		return NULL;
	}

	response->mt = mt;
	response->gid = gid;
	response->oid = oid;
	response->response_buf = response_buf;
	response->response_len = response_len;
	response->sem = sem;
	response->next = NULL;

	k_mutex_lock(&g_uci_context.mutex, K_FOREVER);

	if (g_uci_context.waiting_packets == NULL) {
		g_uci_context.waiting_packets = response;
	} else {
		uci_waiting_packets_t *current = g_uci_context.waiting_packets;

		while (current->next != NULL) {
			current = current->next;
		}
		current->next = response;
	}
	g_uci_context.num_waiting_responses++;

	k_mutex_unlock(&g_uci_context.mutex);

	return response;
}

/**
 * @brief Remove a waiting response from the queue (without locking)
 *
 * Internal function to remove a waiting packet from the linked list.
 * Assumes mutex is already locked by caller.
 *
 * @param response Pointer to waiting packet to remove
 *
 * @return 0 on success, -1 if packet not found
 *
 * @note Frees the memory allocated for the response structure
 * @warning Must be called with mutex locked
 */
static int uci_remove_waiting_response_nolock(uci_waiting_packets_t *response)
{
	if (g_uci_context.waiting_packets == NULL) {
		return -1;
	}

	if (g_uci_context.waiting_packets == response) {
		g_uci_context.waiting_packets = response->next;
		g_uci_context.num_waiting_responses--;
	} else {
		uci_waiting_packets_t *current = g_uci_context.waiting_packets;

		while ((current != NULL) && (current->next != response)) {
			current = current->next;
		}
		if (NULL == current) {
			/* Could not find matching response in the list */
			return -1;
		}
		current->next = response->next;
		g_uci_context.num_waiting_responses--;
		k_free(response);
	}
	return 0;
}

/**
 * @brief Remove a waiting response from the queue (thread-safe)
 *
 * Thread-safe wrapper around uci_remove_waiting_response_nolock.
 * Acquires mutex before removing the response.
 *
 * @param response Pointer to waiting packet to remove
 *
 * @return 0 on success, -1 if packet not found
 */
static int uci_remove_waiting_response(uci_waiting_packets_t *response)
{
	k_mutex_lock(&g_uci_context.mutex, K_FOREVER);

	int ret = uci_remove_waiting_response_nolock(response);

	k_mutex_unlock(&g_uci_context.mutex);
	return ret;
}

/**
 * @brief Transmit a single UCI frame to hardware
 *
 * Low-level function to send a UCI frame through the TML (Transport Mapping Layer).
 *
 * @param frame Pointer to frame data to transmit
 * @param frame_len Length of frame data
 *
 * @return 0 on success, -1 on failure
 *
 * @note Uses phTmlUwb_Write for actual hardware transmission
 */
static int uci_transmit_frame(const uint8_t *frame, uint16_t frame_len)
{
	if (kUwb_StatusCode_Success != uwb_tml_write((uint8_t *)frame, frame_len)) {
		return -1;
	}
	return 0;
}

/**
 * @brief Process received UCI packets and match with waiting responses
 *
 * Main RX handler that:
 * 1. Validates packet length
 * 2. Handles special cases (retry notifications)
 * 3. Matches received packets with waiting responses
 * 4. Copies response data and signals waiting threads
 *
 * @param packet Pointer to received packet data
 * @param packet_len Length of received packet
 *
 * @note Thread-safe operation using mutex
 * @note Special handling for UCI_MSG_CORE_GENERIC_ERROR_NTF with retry status
 */
static void uwb_uci_rx_handler(const uint8_t *const packet, const uint32_t packet_len)
{
	uci_waiting_packets_t *p_waiting_response = NULL;
	bool sent_to_waiter = false;

	LOG_HEXDUMP_INF(packet, packet_len, "RECV <:");

	const uci_control_packet_header_t *header = (const uci_control_packet_header_t *)packet;
	LOG_DBG("Received mt=%02x gid=%02x oid=%02x", (header->mt << 1), header->gid, header->oid);

	k_mutex_lock(&g_uci_context.mutex, K_FOREVER);

	if ((header->mt == UCI_MT_NTF) && (header->gid == UCI_GID_CORE) &&
	    (header->oid == UCI_MSG_CORE_GENERIC_ERROR_NTF)) {
		/* Generic error notification */
		/* Check if error code is retry */
		if (kUci_Status_UciMessageRetry == packet[UCI_HEADER_SIZE]) {
			/** Check if we had sent a packet from uwb_uci */
			if ((g_uci_context.response_packet) &&
			    (g_uci_context.response_packet->response_buf) &&
			    (g_uci_context.response_packet->response_len)) {
				/** Packet was sent, copy packet to output and trigger semaphore */
				if ((*g_uci_context.response_packet->response_len) >= packet_len) {
					memcpy(g_uci_context.response_packet->response_buf, packet,
					       packet_len);
					*g_uci_context.response_packet->response_len = packet_len;
					sent_to_waiter = true;
					k_sem_give(g_uci_context.response_packet->sem);
				}
				k_mutex_unlock(&g_uci_context.mutex);
				return;
			} else {
				LOG_WRN("Retry notification received but no response expected");
			}
		}
	}

	/* Find if there is a waiting response for this packet */
	if (g_uci_context.num_waiting_responses) {
		p_waiting_response = g_uci_context.waiting_packets;
		while (p_waiting_response) {
			LOG_DBG("Waiting mt=%02x gid=%02x oid=%02x", p_waiting_response->mt,
				p_waiting_response->gid, p_waiting_response->oid);
			if ((p_waiting_response->mt == header->mt) &&
			    (p_waiting_response->gid == header->gid) &&
			    (p_waiting_response->oid == header->oid)) {
				LOG_DBG("Found waiting response");
				/* Found waiting response */
				break;
			}
			p_waiting_response = p_waiting_response->next;
		}
		if (p_waiting_response) {
			LOG_DBG("Inform waiting response if possible");
			/** Found waiting response - update response buffer */
			if ((NULL != p_waiting_response->response_buf) &&
			    (NULL != p_waiting_response->response_len)) {
				if (*(p_waiting_response->response_len) >= (packet_len)) {
					memcpy(p_waiting_response->response_buf, packet,
					       packet_len);
					*(p_waiting_response->response_len) = packet_len;
					sent_to_waiter = true;
					/* Produce semaphore if someone was waiting */
					LOG_DBG("Produce waiting response semaphore");
					k_sem_give(p_waiting_response->sem);
				} else {
					LOG_WRN("Could not copy response, buffer too small");
				}
			}
		} else {
			/* Send to callback */
			LOG_DBG("Could not find a waiting response");
		}
	}

	if ((header->mt == UCI_MT_NTF) || (header->mt == UCI_MT_DATA) ||
	    (false == sent_to_waiter)) {
		LOG_DBG("No waiting responses");
		if (g_uci_context.callback != NULL) {
			LOG_DBG("Sending to application callback");
			g_uci_context.callback(packet, packet_len);
		}
	}

	k_mutex_unlock(&g_uci_context.mutex);
	LOG_DBG("Unlocked mutex");
}

int uwb_uci_init(void)
{
	if (g_uci_context.isInitialized) {
		if (!g_uci_context.isSuspended) {
			/* Already running - nothing to do */
			return 0;
		}
		/* Initialized but suspended */
		k_sem_reset(&g_uci_context.response_semaphore);
		g_uci_context.isSuspended = false;
		return 0;
	}
	/* Not initialized */
	if (kUwb_StatusCode_Success != k_mutex_init(&g_uci_context.mutex)) {
		return -1;
	}
	if (0 != k_sem_init(&g_uci_context.response_semaphore, 0, 1)) {
		return -1;
	}
	g_uci_context.isInitialized = true;
	g_uci_context.isSuspended = false;
	return 0;
}

int uwb_uci_register_callback(uwb_uci_callback_t *callback)
{
	if (!g_uci_context.isInitialized || g_uci_context.isSuspended) {
		return -1;
	}

	k_mutex_lock(&g_uci_context.mutex, K_FOREVER);
	g_uci_context.callback = callback;
	k_mutex_unlock(&g_uci_context.mutex);

	return 0;
}

int uwb_uci_get_registered_callback(uwb_uci_callback_t **pRegisteredCallback)
{
	if (!g_uci_context.isInitialized || g_uci_context.isSuspended) {
		return -1;
	}

	*pRegisteredCallback = g_uci_context.callback;
	return 0;
}

void uwb_uci_handle_received_packet(const uint8_t *packet, uint32_t packet_len)
{
	if (!g_uci_context.isInitialized || g_uci_context.isSuspended) {
		/* UCI handler not initialized */
		return;
	}
	/* Validate input parameters */
	if (NULL == packet) {
		LOG_ERR("Invalid parameter: packet is NULL");
		return;
	}

	if (packet_len < UCI_HEADER_SIZE) {
		LOG_ERR("Invalid parameter: packet_len (%u) is less than UCI_HEADER_SIZE (%u)",
			packet_len, UCI_HEADER_SIZE);
		return;
	}
	uwb_uci_rx_handler(packet, packet_len);
}

int uwb_uci_transceive(uint8_t mt, uint8_t gid, uint8_t oid, const uint8_t *payload,
		       uint16_t payload_len, uint8_t *response_buf, uint32_t *response_len,
		       uint8_t mt_rsp, uint8_t gid_rsp, uint8_t oid_rsp, bool wait_for_response)
{
	if (!g_uci_context.isInitialized || g_uci_context.isSuspended) {
		return -1;
	}

	int ret = -1;
	uint8_t retry_count = 0;
	const uint32_t response_len_dup = (response_len) ? (*response_len) : 0;
	const k_timeout_t response_timeout = Z_TIMEOUT_MS(CONFIG_UCI_RESPONSE_TIMEOUT_MS);

	uci_waiting_packets_t *p_waiting_response = NULL;
	const uint16_t uci_max_packet_payload_size =
		(UCI_MT_DATA == mt) ? g_uci_context.max_data_packet_payload_size
				    : UCI_MAX_CTRL_PACKET_PAYLOAD_SIZE;

	/* Setup pending response tracking before transmitting frames */
	if (wait_for_response) {
		p_waiting_response =
			uci_add_waiting_response(mt_rsp, gid_rsp, oid_rsp, response_buf,
						 response_len, &g_uci_context.response_semaphore);
		if (NULL == p_waiting_response) {
			/** Could not add waiting response */
			return -1;
		}
		k_mutex_lock(&g_uci_context.mutex, K_FOREVER);
		g_uci_context.response_packet = p_waiting_response;
		k_mutex_unlock(&g_uci_context.mutex);
	}

	/* Calculate total frames needed for chained transmission */
	while (retry_count < UCI_MAX_RETRIES) {
		uint16_t remaining_len = payload_len;
		uint16_t offset = 0;
		bool transmission_success = true;

		if (wait_for_response) {
			/* Reset waiting response buffer and length pointer before every retry */
			k_mutex_lock(&g_uci_context.mutex, K_FOREVER);
			p_waiting_response->response_buf = response_buf;
			if (p_waiting_response->response_len) {
				*response_len = response_len_dup;
			}
			k_mutex_unlock(&g_uci_context.mutex);
		}

		/* Transmit all frames (chained if necessary) */
		do {
			uint16_t current_payload_len = (remaining_len > uci_max_packet_payload_size)
							       ? uci_max_packet_payload_size
							       : remaining_len;
			uint8_t pbf = (remaining_len > uci_max_packet_payload_size) ? 1 : 0;

			uint8_t frame[CONFIG_UCI_FRAME_LEN] = {0};
			uci_control_packet_header_t *header =
				(uci_control_packet_header_t *)(frame);

			if (current_payload_len > CONFIG_UCI_FRAME_LEN) {
				LOG_ERR("Cannot send more than %d bytes in a packet\n"
					"Configure CONFIG_UCI_FRAME_LEN to maximum supported bytes "
					"to transfer in a single packet",
					CONFIG_UCI_FRAME_LEN);
				goto exit;
			}

			/* Build header using structure */
			header->mt = mt & 0x07;
			header->pbf = pbf & 0x01;
			header->gid = gid & 0x0F;
			header->oid = oid;
			if (UCI_MT_DATA == mt) {
				uci_set_length_be(header, current_payload_len);
			} else {
				uci_set_length_le(header, current_payload_len);
			}

			/* Copy payload */
			if (payload != NULL && current_payload_len > 0) {
				memcpy(frame + UCI_HEADER_SIZE, payload + offset,
				       current_payload_len);
			}
			/* Transmit frame */
			if (uci_transmit_frame(frame, UCI_HEADER_SIZE + current_payload_len) != 0) {
				transmission_success = false;
				break;
			}

			offset += current_payload_len;
			remaining_len -= current_payload_len;
		} while (remaining_len > 0);

		if (!transmission_success) {
			retry_count++;
			k_msleep(10); /* Small delay before retry */
			continue;
		}

		/* Wait for response if required (after all frames are sent) */
		if (wait_for_response) {
			offset = 0;
			uint8_t *rsp_ptr = response_buf;
			uint8_t pbf = 0;
			do {
				uwb_status_code_t sem_ret = k_sem_take(
					&g_uci_context.response_semaphore, response_timeout);
				if (sem_ret == kUwb_StatusCode_Success) {
					uci_control_packet_header_t *header =
						(uci_control_packet_header_t *)response_buf;
					if ((header->mt == UCI_MT_NTF) &&
					    (header->gid == UCI_GID_CORE) &&
					    (header->oid == UCI_MSG_CORE_GENERIC_ERROR_NTF)) {
						if (kUci_Status_UciMessageRetry ==
						    response_buf[UCI_HEADER_SIZE]) {
							/* Received retry */
							LOG_DBG("Retry received");
							retry_count++;
							memset(response_buf, 0, *response_len);
							*response_len = response_len_dup;
							break;
						}
					}
					pbf = header->pbf;
					if (offset > 0) {
						/* Not the first packet, remove UCI header */
						if (*response_len < UCI_HEADER_SIZE) {
							LOG_ERR("Invalid response length");
							goto exit;
						}
						*response_len -= UCI_HEADER_SIZE;
						memmove(rsp_ptr, rsp_ptr + UCI_HEADER_SIZE,
							*response_len);
					}
					rsp_ptr += *response_len;
					offset += *response_len;
					ret = 0;
					/* Update response pointer so that reader thread copies
					 * data at correct location
					 */
					k_mutex_lock(&g_uci_context.mutex, K_FOREVER);
					p_waiting_response->response_buf = rsp_ptr;
					if (p_waiting_response->response_len) {
						*(p_waiting_response->response_len) =
							response_len_dup - offset;
					}
					k_mutex_unlock(&g_uci_context.mutex);
				} else {
					/* Timeout or no response, retry entire packet */
					LOG_ERR("Timed out waiting for response");
					retry_count++;
					ret = -1;
					break;
				}
			} while (pbf == 1);
			if (ret == 0) {
				/* Success, re-assign response_len as correct number of bytes read
				 */
				if (response_len) {
					*response_len = offset;
				}
				break;
			} else {
				continue;
			}
		} else {
			/* No response needed, transmission successful */
			ret = 0;
			break;
		}
	}

exit:
	/* Cleanup */
	if (wait_for_response) {
		uci_remove_waiting_response(p_waiting_response);
		k_mutex_lock(&g_uci_context.mutex, K_FOREVER);
		g_uci_context.response_packet = NULL;
		k_mutex_unlock(&g_uci_context.mutex);
	}

	return ret;
}

int uwb_uci_transceive_control_packet(uint8_t gid, uint8_t oid, const uint8_t *payload,
				      uint16_t payload_len, uint8_t *response_buf,
				      uint32_t *response_len)
{
	return uwb_uci_transceive(UCI_MT_CMD, gid, oid, payload, payload_len, response_buf,
				  response_len, UCI_MT_RSP, gid, oid, true);
}

int uwb_uci_transceive_data_packet(uint8_t dpf, const uint8_t *payload, uint16_t payload_len,
				   uint8_t *response_buf, uint32_t *response_len)
{
	if (!g_uci_context.isInitialized || g_uci_context.isSuspended) {
		return -1;
	}

	k_mutex_lock(&g_uci_context.mutex, K_FOREVER);
	const uint16_t max_data_size = g_uci_context.max_data_packet_payload_size;
	k_mutex_unlock(&g_uci_context.mutex);

	int ret = -1;
	const uint8_t *pPayload = payload;
	const uint32_t response_len_dup = *response_len;
	uci_waiting_packets_t *p_waiting_response = NULL;
	const k_timeout_t credit_ntf_timeout =
		Z_TIMEOUT_MS(CONFIG_UCI_CREDIT_NTF_TIMEOUT_MS / UCI_MAX_RETRIES);

	p_waiting_response = uci_add_waiting_response(
		UCI_MT_NTF, UCI_GID_RANGE_MANAGE, UCI_MSG_DATA_CREDIT_NTF, response_buf,
		response_len, &g_uci_context.response_semaphore);
	if (NULL == p_waiting_response) {
		/* Could not add waiting response */
		return -1;
	}
	k_mutex_lock(&g_uci_context.mutex, K_FOREVER);
	g_uci_context.response_packet = p_waiting_response;
	k_mutex_unlock(&g_uci_context.mutex);
	uint8_t retry_count = 0;
	uint16_t offset = 0;
	uint16_t remaining_len = payload_len;

	do {
		uint16_t current_payload_len;
retry:
		current_payload_len =
			(remaining_len > max_data_size) ? max_data_size : remaining_len;
		*response_len = response_len_dup;

		ret = uwb_uci_transceive(UCI_MT_DATA, dpf, 0 /** No OID for data packet */,
					 pPayload + offset, current_payload_len, response_buf,
					 response_len, 0, 0, 0, false);
		if (0 != ret) {
			LOG_ERR("UCI Transceive failed");
			break;
		}

		ret = -1;
		uint8_t credit_ntf_retry_count = 0;
		while (credit_ntf_retry_count < UCI_MAX_RETRIES) {
			if (k_sem_take(&g_uci_context.response_semaphore, credit_ntf_timeout) ==
			    kUwb_StatusCode_Success) {
				uci_control_packet_header_t *header =
					(uci_control_packet_header_t *)response_buf;
				if ((header->mt == UCI_MT_NTF) && (header->gid == UCI_GID_CORE) &&
				    (header->oid == UCI_MSG_CORE_GENERIC_ERROR_NTF)) {
					if (kUci_Status_UciMessageRetry ==
					    response_buf[UCI_HEADER_SIZE]) {
						/* Received retry */
						LOG_DBG("Retry notification received, retry "
							"sending only the last frame");
						retry_count++;
						memset(response_buf, 0, *response_len);
						goto retry;
					}
				}
				if (UCI_STATUS_CREDIT_AVAILABLE !=
				    response_buf[UCI_CREDIT_NTF_STATUS_OFFSET]) {
					LOG_DBG("Did not receive credit notification with status "
						"available");
					credit_ntf_retry_count++;
					continue;
				}
				ret = 0;
				break;
			} else {
				/* Timeout or no response, retry entire packet */
				credit_ntf_retry_count++;
				continue;
			}
		}
		if (0 != ret) {
			LOG_ERR("Could not get credit notification");
			break;
		}
		offset += current_payload_len;
		remaining_len -= current_payload_len;
	} while ((remaining_len > 0) && (retry_count < UCI_MAX_RETRIES));

	uci_remove_waiting_response(p_waiting_response);
	k_mutex_lock(&g_uci_context.mutex, K_FOREVER);
	g_uci_context.response_packet = NULL;
	k_mutex_unlock(&g_uci_context.mutex);

	if (0 == remaining_len) {
		return 0;
	}
	return -1;
}

int uwb_uci_wait_for_packet(uint8_t mt, uint8_t gid, uint8_t oid, uint8_t *pBuffer,
			    uint32_t *pBufferLen, k_timeout_t timeout)
{
	if (!g_uci_context.isInitialized || g_uci_context.isSuspended) {
		return -1;
	}

	struct k_sem semaphore;
	uwb_status_code_t status = k_sem_init(&semaphore, 0, 1);
	if (kUwb_StatusCode_Success != status) {
		return -1;
	}

	uci_waiting_packets_t *p_waiting_response =
		uci_add_waiting_response(mt, gid, oid, pBuffer, pBufferLen, &semaphore);
	if (p_waiting_response == NULL) {
		k_sem_reset(&semaphore);
		return -1;
	}

	status = k_sem_take(&semaphore, timeout);
	k_mutex_lock(&g_uci_context.mutex, K_FOREVER);
	if (kUwb_StatusCode_Success != status) {
		/* Consume semaphore timeout */
		uci_remove_waiting_response_nolock(p_waiting_response);
		k_sem_reset(&semaphore);
		k_mutex_unlock(&g_uci_context.mutex);
		return -1;
	}
	/* Got the response */
	uci_remove_waiting_response_nolock(p_waiting_response);
	k_sem_reset(&semaphore);
	k_mutex_unlock(&g_uci_context.mutex);

	return 0;
}

void *uwb_uci_schedule_packet_read(uint8_t mt, uint8_t gid, uint8_t oid, uint8_t *pBuffer,
				   uint32_t *pBufferLen, struct k_sem *semaphore)
{
	if (!g_uci_context.isInitialized || g_uci_context.isSuspended) {
		return NULL;
	}

	uci_waiting_packets_t *p_waiting_response =
		uci_add_waiting_response(mt, gid, oid, pBuffer, pBufferLen, semaphore);
	if (p_waiting_response == NULL) {
		return NULL;
	}

	return (void *)p_waiting_response;
}

void uwb_uci_remove_scheduled_packet(void *pResponse)
{
	if (!g_uci_context.isInitialized || g_uci_context.isSuspended) {
		return;
	}
	uci_waiting_packets_t *p_waiting_response = (uci_waiting_packets_t *)pResponse;
	uci_remove_waiting_response(p_waiting_response);
}

void uwb_uci_deinit(void)
{
	if (false == g_uci_context.isInitialized || g_uci_context.isSuspended) {
		return;
	}

	k_mutex_lock(&g_uci_context.mutex, K_FOREVER);
	if (g_uci_context.waiting_packets != NULL) {
		uci_waiting_packets_t *current = g_uci_context.waiting_packets;
		while (current) {
			uci_remove_waiting_response_nolock(current);
			current = g_uci_context.waiting_packets;
		}
	}
	k_sem_reset(&g_uci_context.response_semaphore);
	k_mutex_unlock(&g_uci_context.mutex);
	g_uci_context.isSuspended = true;
}

int uwb_uci_configure_max_data_payload(const uint16_t maxDataPacketSize)
{
	if (false == g_uci_context.isInitialized || g_uci_context.isSuspended) {
		return -1;
	}

	k_mutex_lock(&g_uci_context.mutex, K_FOREVER);
	g_uci_context.max_data_packet_payload_size = maxDataPacketSize;
	k_mutex_unlock(&g_uci_context.mutex);

	return 0;
}
