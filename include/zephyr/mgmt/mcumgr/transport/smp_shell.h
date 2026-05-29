/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Shell transport for the mcumgr SMP protocol.
 * @ingroup mcumgr_transport_shell
 */

#ifndef ZEPHYR_INCLUDE_MGMT_SMP_SHELL_H_
#define ZEPHYR_INCLUDE_MGMT_SMP_SHELL_H_

/**
 * @brief This allows to use the MCUmgr SMP protocol over Zephyr shell.
 * @defgroup mcumgr_transport_shell Shell transport
 * @ingroup mcumgr_transport
 * @{
 */

#include <zephyr/kernel.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SMP_SHELL_RX_BUF_SIZE	127

/** @brief Data used by SMP shell */
struct smp_shell_data {
	struct net_buf_pool *buf_pool;
	struct k_fifo buf_ready;
	struct net_buf *buf;
	atomic_t esc_state;
};

/**
 * @brief Attempt to process received bytes as part of an SMP frame.
 *
 * Called to scan buffer from the beginning and consume all bytes that are
 * part of SMP frame until frame or buffer ends.
 *
 * @param data SMP shell transfer data.
 * @param bytes Buffer with bytes to process
 * @param size Number of bytes to process
 *
 * @return number of bytes consumed by the SMP
 */
size_t smp_shell_rx_bytes(struct smp_shell_data *data, const uint8_t *bytes,
			  size_t size);

/**
 * @brief Processes SMP data and executes command if full frame was received.
 *
 * This function should be called from thread context.
 *
 * @param data SMP shell transfer data.
 */
void smp_shell_process(struct smp_shell_data *data);

/**
 * @brief Initializes SMP transport over shell.
 *
 * This function should be called before feeding SMP transport with received
 * data.
 *
 * @return 0 on success
 */
int smp_shell_init(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif
