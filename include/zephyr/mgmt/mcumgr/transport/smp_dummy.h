/*
 * Copyright Runtime.io 2018. All rights reserved.
 * Copyright Laird Connectivity 2021-2022. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Dummy transport for the mcumgr SMP protocol for unit testing.
 */
#ifndef ZEPHYR_INCLUDE_MGMT_MCUMGR_TRANSPORT_DUMMY_H_
#define ZEPHYR_INCLUDE_MGMT_MCUMGR_TRANSPORT_DUMMY_H_

#include <zephyr/kernel.h>
#include <zephyr/net/buf.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/transport/serial.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Clears internal dummy SMP state and resets semaphore
 */
void smp_dummy_clear_state(void);

/**
 * @brief Adds SMPC data to the internal buffer to be processed
 *
 * @param data Input data buffer
 * @param data_size Size of data (in bytes)
 */
void dummy_mcumgr_add_data(uint8_t *data, uint16_t data_size);

/**
 * @brief Processes a single line (fragment) coming from the mcumgr response to
 *        be used in tests
 *
 * @retval net buffer of processed data
 */
struct net_buf *smp_dummy_get_outgoing(void);

/**
 * @brief Waits for a period of time for outgoing SMPC data to be ready and
 *        returns either when a full message is ready or when the timeout has
 *        elapsed.
 *
 * @param wait_time_s Time to wait for data (in seconds)
 *
 * @retval true on message received successfully, false on timeout
 */
bool smp_dummy_wait_for_data(uint32_t wait_time_s);

/**
 * @brief Calls dummy_mcumgr_add_data with the internal SMPC receive buffer.
 */
void smp_dummy_add_data(void);

/**
 * @brief Gets current send buffer position
 *
 * @retval Current send buffer position (in bytes)
 */
uint16_t smp_dummy_get_send_pos(void);

/**
 * @brief Gets current receive buffer position
 *
 * @retval Current receive buffer position (in bytes)
 */
uint16_t smp_dummy_get_receive_pos(void);

/**
 * @brief Converts input data to go out through the internal SMPC buffer.
 *
 * @param data Input data buffer
 * @param len Size of data (in bytes)
 *
 * @retval 0 on success, negative on error.
 */
int smp_dummy_tx_pkt(const uint8_t *data, int len);

/**
 * @brief Enabled the dummy SMP module (will process sent/received data)
 */
void smp_dummy_enable(void);

/**
 * @brief Disables the dummy SMP module (will not process sent/received data)
 */
void smp_dummy_disable(void);

/**
 * @brief Returns status on if the dummy SMP system is active
 *
 * @retval true if dummy SMP is enabled, false otherwise
 */
bool smp_dummy_get_status(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_MGMT_MCUMGR_TRANSPORT_DUMMY_H_ */
