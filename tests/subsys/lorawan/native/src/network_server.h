/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TESTS_SUBSYS_LORAWAN_NATIVE_NETWORK_SERVER_H_
#define TESTS_SUBSYS_LORAWAN_NATIVE_NETWORK_SERVER_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/lorawan/lorawan.h>

enum network_server_invalid_join_response {
	NETWORK_SERVER_INVALID_JOIN_RESPONSE_NONE,
	NETWORK_SERVER_INVALID_JOIN_RESPONSE_WRONG_MHDR,
	NETWORK_SERVER_INVALID_JOIN_RESPONSE_UNSUPPORTED_MAJOR,
	NETWORK_SERVER_INVALID_JOIN_RESPONSE_EMPTY,
	NETWORK_SERVER_INVALID_JOIN_RESPONSE_SHORT,
	NETWORK_SERVER_INVALID_JOIN_RESPONSE_INTERMEDIATE,
	NETWORK_SERVER_INVALID_JOIN_RESPONSE_OVERSIZE,
	NETWORK_SERVER_INVALID_JOIN_RESPONSE_BAD_CIPHERTEXT,
	NETWORK_SERVER_INVALID_JOIN_RESPONSE_BAD_MIC,
	NETWORK_SERVER_INVALID_JOIN_RESPONSE_INVALID_RX1_OFFSET,
	NETWORK_SERVER_INVALID_JOIN_RESPONSE_INVALID_RX2_DATARATE,
};

enum network_server_invalid_downlink {
	NETWORK_SERVER_INVALID_DOWNLINK_NONE,
	NETWORK_SERVER_INVALID_DOWNLINK_WRONG_MHDR,
	NETWORK_SERVER_INVALID_DOWNLINK_UNSUPPORTED_MAJOR,
	NETWORK_SERVER_INVALID_DOWNLINK_SHORT,
	NETWORK_SERVER_INVALID_DOWNLINK_WRONG_DEV_ADDR,
	NETWORK_SERVER_INVALID_DOWNLINK_BAD_MIC,
	NETWORK_SERVER_INVALID_DOWNLINK_BAD_FOPTS_LEN,
};

/**
 * @brief Get the OTAA configuration used by the test network server.
 *
 * @return LoRaWAN join configuration.
 */
struct lorawan_join_config network_server_join_config(void);

/** @brief Get the known LoRaWAN 1.0.x vector OTAA configuration. */
struct lorawan_join_config network_server_vector_join_config(void);

/** @brief Prepare the known LoRaWAN 1.0.x vector join exchange. */
void network_server_prepare_vector_join(void);

/** @brief Prepare the known LoRaWAN 1.0.x vector application uplink. */
void network_server_prepare_vector_uplink(void);

/**
 * @brief Prepare the server to return a join-accept.
 *
 * @param rx_window Receive window in which to return the join-accept.
 */
void network_server_prepare_join(uint32_t rx_window);

/**
 * @brief Prepare the server to return a join-accept with a specific RxDelay.
 *
 * @param rx_delay Receive delay in seconds.
 */
void network_server_prepare_join_with_rx_delay(uint8_t rx_delay);

/**
 * @brief Prepare the server to return an invalid join-accept in RX1.
 *
 * A valid join-accept is returned in RX2.
 */
void network_server_prepare_invalid_join_response(void);

/**
 * @brief Prepare the server to return an invalid join-accept in both RX windows.
 *
 * @param response The invalid response to return.
 */
void network_server_prepare_invalid_join_responses(
	enum network_server_invalid_join_response response);

/**
 * @brief Prepare the server to return a join-accept containing a CFList.
 *
 * @param rx_window Receive window in which to return the join-accept.
 */
void network_server_prepare_join_with_cflist(uint32_t rx_window);

/** @brief Prepare the server to ignore receive windows for a join request. */
void network_server_prepare_join_no_response(void);

/**
 * @brief Prepare the server to ignore receive windows for an application uplink.
 *
 * @param type Expected LoRaWAN message type.
 * @param port Expected application port.
 * @param payload Expected plaintext payload.
 * @param payload_len Length of the expected payload.
 */
void network_server_prepare_uplink_no_response(enum lorawan_message_type type, uint8_t port,
					       const uint8_t *payload, size_t payload_len);

/** @brief Prepare the server to expect an uplink acknowledging a confirmed downlink. */
void network_server_prepare_uplink_ack(uint8_t port, const uint8_t *payload, size_t payload_len);

/**
 * @brief Prepare an application downlink for the specified receive window.
 *
 * @param rx_window Receive window in which to return the downlink.
 * @param port Application port.
 * @param payload Plaintext application payload.
 * @param payload_len Length of @p payload.
 */
void network_server_prepare_downlink(uint32_t rx_window, uint8_t port, const uint8_t *payload,
				     size_t payload_len);

/** @brief Prepare an application downlink with the data-pending flag set. */
void network_server_prepare_data_pending_downlink(uint32_t rx_window, uint8_t port,
						  const uint8_t *payload, size_t payload_len);

/** @brief Prepare a confirmed application downlink for the specified receive window. */
void network_server_prepare_confirmed_downlink(uint32_t rx_window, uint8_t port,
					       const uint8_t *payload, size_t payload_len);

/**
 * @brief Prepare an invalid downlink in RX1.
 *
 * @param response Invalid downlink variant to return.
 * @param port Application port.
 * @param payload Plaintext application payload.
 * @param payload_len Length of @p payload.
 */
void network_server_prepare_invalid_downlink(enum network_server_invalid_downlink response,
					     uint8_t port, const uint8_t *payload,
					     size_t payload_len);

/**
 * @brief Prepare the server to acknowledge a confirmed uplink.
 *
 * @param tx_attempt Transmission attempt on which to return the acknowledgment.
 * @param port Expected application port.
 * @param payload Expected plaintext payload.
 * @param payload_len Length of the expected payload.
 */
void network_server_prepare_ack(uint32_t tx_attempt, uint8_t port, const uint8_t *payload,
				size_t payload_len);

/**
 * @brief Require the next transmission to use a specific frequency.
 *
 * By default, transmissions are expected on one of the EU868 default
 * channels.
 *
 * @param frequency Expected transmission frequency in Hz.
 */
void network_server_expect_tx_frequency(uint32_t frequency);

/**
 * @brief Require the next application uplink to use a specific datarate.
 *
 * @param datarate Expected LoRaWAN datarate.
 */
void network_server_expect_uplink_datarate(enum lorawan_datarate datarate);

/** @brief Require RX1 to open at the specified delay after transmission. */
void network_server_expect_rx_delay(uint8_t delay_seconds);

#endif /* TESTS_SUBSYS_LORAWAN_NATIVE_NETWORK_SERVER_H_ */
