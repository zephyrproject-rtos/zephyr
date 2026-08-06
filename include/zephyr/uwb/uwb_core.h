/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __UWB_CORE_H__
#define __UWB_CORE_H__

#include <stdbool.h>
#include <stdint.h>
#include "zephyr/uwb/uci.h"
#include "zephyr/uwb/status.h"
#include "zephyr/kernel.h"

/** Maximum buffer size of single UCI packet */
#ifndef CONFIG_UCI_FRAME_LEN
#define CONFIG_UCI_FRAME_LEN (UCI_HEADER_SIZE + UCI_MAX_CTRL_PACKET_PAYLOAD_SIZE)
#endif /** CONFIG_UCI_FRAME_LEN */

/**
 * \addtogroup uci_core
 * @{
 */

typedef struct {
	uint8_t gid: 4;
	uint8_t pbf: 1;
	uint8_t mt: 3;
	uint8_t oid;
	uint16_t len_le;
} uci_control_packet_header_t;

typedef struct {
	uint8_t dpf: 4;
	uint8_t pbf: 1;
	uint8_t mt: 3;
	uint8_t rfu;
	uint16_t len_le;
} uci_data_packet_header_t;

/**
 * @brief Callback function type for receiving unhandled UCI packets
 *
 * @param packet Pointer to received packet data
 * @param packet_len Length of received packet
 */
typedef void(uwb_uci_callback_t)(const uint8_t *const packet, const uint32_t packet_len);

/**
 * @brief Initialize the UCI transmission module
 *
 * Sets up the UCI context including mutex, semaphore and linked list of waiting packets.
 * This function must be called before any UCI operations.
 *
 * @return 0 on success, -1 on failure
 *
 * @note If already initialized, returns success immediately
 */
int uwb_uci_init(void);

/**
 * @brief Cleanup and deinitialize UCI transmission module
 *
 * Releases all resources including mutex and semaphore, purges waiting packets, and resets context.
 * Should be called during system shutdown.
 *
 */
void uwb_uci_deinit(void);

/**
 * @brief Handle received UCI packet from message queue
 *
 * Entry point for received packets.
 *
 * @param packet Pointer to received packet data
 * @param packet_len Length of received packet
 *
 * @note Message queue implementation is currently disabled
 */
void uwb_uci_handle_received_packet(const uint8_t *packet, uint32_t packet_len);

/**
 * @brief Transmit UCI packet with automatic retry and response handling
 *
 * High-level function to send UCI commands and optionally wait for responses.
 * Features:
 * - Automatic packet chaining for large payloads
 * - Retry mechanism (up to UCI_MAX_RETRIES)
 * - Response matching and timeout handling
 * - Special handling for retry notifications
 *
 * @param mt  Message type
 * @param gid Group ID for command
 * @param oid Opcode ID for command
 * @param payload Pointer to payload data
 * @param payload_len Length of payload
 * @param response_buf Buffer to store response (if \p wait_for_response is true)
 * @param response_len Pointer to response length (input: buffer size, output: actual size)
 * @param mt_rsp Expected Message Type of response
 * @param gid_rsp Expected Group ID of response
 * @param oid_rsp Expected Opcode ID of response
 * @param wait_for_response If true, waits for response; if false, returns immediately
 *
 * @return 0 on success, -1 on failure
 *
 * @note Supports packet chaining for payloads larger than MAX_PAYLOAD_PER_FRAME (255 bytes)
 * @note Automatically retries on timeout or retry notification
 * @note Response timeout is CONFIG_UCI_RESPONSE_TIMEOUT_MS (200ms)
 */
int uwb_uci_transceive(uint8_t mt, uint8_t gid, uint8_t oid, const uint8_t *payload,
		       uint16_t payload_len, uint8_t *response_buf, uint32_t *response_len,
		       uint8_t mt_rsp, uint8_t gid_rsp, uint8_t oid_rsp, bool wait_for_response);

/**
 * @brief Transmit UCI control packet with automatic retry and response handling
 *
 * High-level function to send UCI commands and optionally wait for responses.
 * Features:
 * - Automatic packet chaining for large payloads
 * - Retry mechanism (up to UCI_MAX_RETRIES)
 * - Wait for response matching same GID and OID
 * - Response matching and timeout handling
 * - Special handling for retry notifications
 *
 * @param gid Group ID for command
 * @param oid Opcode ID for command
 * @param payload Pointer to payload data
 * @param payload_len Length of payload
 * @param response_buf Buffer to store response
 * @param response_len Pointer to response length (input: buffer size, output: actual size)
 *
 * @return 0 on success, -1 on failure
 *
 * @note Supports packet chaining for payloads larger than MAX_PAYLOAD_PER_FRAME (255 bytes)
 * @note Automatically retries on timeout or retry notification
 * @note Response timeout is CONFIG_UCI_RESPONSE_TIMEOUT_MS (200ms)
 */
int uwb_uci_transceive_control_packet(uint8_t gid, uint8_t oid, const uint8_t *payload,
				      uint16_t payload_len, uint8_t *response_buf,
				      uint32_t *response_len);

/**
 * @brief Transmit UCI Data packet with automatic retry and response handling
 *
 * High-level function to send UCI commands and optionally wait for responses.
 * Features:
 * - Automatic packet chaining for large payloads
 * - Retry mechanism (up to UCI_MAX_RETRIES)
 * - Wait for data credit notification
 * - Response matching and timeout handling
 * - Special handling for retry notifications
 *
 * @param dpf Data packet format for command
 * @param payload Pointer to payload data
 * @param payload_len Length of payload
 * @param response_buf Buffer to store response
 * @param response_len Pointer to response length (input: buffer size, output: actual size)
 *
 * @return 0 on success, -1 on failure
 *
 * @note Supports packet chaining for payloads larger than MAX_PAYLOAD_PER_FRAME (255 bytes)
 * @note Automatically retries on timeout or retry notification
 * @note Response timeout is CONFIG_UCI_RESPONSE_TIMEOUT_MS (200ms)
 */
int uwb_uci_transceive_data_packet(uint8_t dpf, const uint8_t *payload, uint16_t payload_len,
				   uint8_t *response_buf, uint32_t *response_len);

/**
 * @brief Wait for a specific UCI packet with timeout
 *
 * Blocking function that waits for a specific UCI packet identified by
 * message type, group ID, and opcode ID.
 *
 * @param mt Message Type to wait for
 * @param gid Group ID / DPF to wait for
 * @param oid Opcode ID to wait for (should be 0x00 for data packet)
 * @param pBuffer Buffer to store received packet
 * @param pBufferLen Pointer to buffer length (input: size, output: actual length)
 * @param timeout Timeout in milliseconds
 *
 * @return 0 on success, -1 on timeout or failure
 *
 */
int uwb_uci_wait_for_packet(uint8_t mt, uint8_t gid, uint8_t oid, uint8_t *pBuffer,
			    uint32_t *pBufferLen, k_timeout_t timeout);

/**
 * @brief Schedule a packet read with external semaphore
 *
 * Non-blocking function to schedule waiting for a specific packet.
 * Caller provides their own semaphore for notification.
 *
 * @param mt Message Type to wait for
 * @param gid Group ID / DPF to wait for
 * @param oid Opcode ID to wait for (should be 0x00 for data packet)
 * @param pBuffer Buffer to store received packet
 * @param pBufferLen Pointer to buffer length
 * @param semaphore External semaphore to signal when packet arrives
 *
 * @return Opaque handle to scheduled packet, NULL on failure
 *
 * @note Caller must call uwb_uci_remove_scheduled_packet() to cleanup
 */
void *uwb_uci_schedule_packet_read(uint8_t mt, uint8_t gid, uint8_t oid, uint8_t *pBuffer,
				   uint32_t *pBufferLen, struct k_sem *semaphore);

/**
 * @brief Remove a scheduled packet read
 *
 * Cancels a previously scheduled packet read and frees associated resources.
 *
 * @param pResponse Handle returned by uwb_uci_schedule_packet_read()
 *
 * @note Must be called to cleanup scheduled reads that are no longer needed
 */
void uwb_uci_remove_scheduled_packet(void *pResponse);

/**
 * @brief Register a callback for unhandled UCI packets (notifications, responses without waiters)
 *
 * @param callback Callback function to handle received packets
 *
 * @return 0 on success, -1 on failure
 */
int uwb_uci_register_callback(uwb_uci_callback_t *callback);

/**
 * @brief Get currently registered callback
 */
int uwb_uci_get_registered_callback(uwb_uci_callback_t **pRegisteredCallback);

/**
 * @brief Configure maximum data payload size for UCI data packets
 */
int uwb_uci_configure_max_data_payload(const uint16_t maxDataPacketSize);

/**
 * @}
 */

#endif /* __UWB_CORE_H__ */
