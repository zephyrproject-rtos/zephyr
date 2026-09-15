/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_MGMT_BMC_CONSOLE_H_
#define ZEPHYR_INCLUDE_MGMT_BMC_CONSOLE_H_

/**
 * @file
 * @brief Access to the captured host console.
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief BMC host console
 * @defgroup bmc_console BMC host console
 * @ingroup bmc_api
 * @{
 *
 * When CONFIG_BMC_CONSOLE_LOGGER is enabled the BMC continuously captures the
 * UART identified by the `host-console-uart` devicetree alias into a circular
 * log. The BMC publishes that log over TCP and over a websocket, and the API
 * below lets an application add further transports of its own.
 */

/** @brief Signalled when new host console output has been captured. */
#define BMC_CONSOLE_EVENT_DATA BIT(0)

/**
 * @brief Event bits reserved for application-provided console transports.
 *
 * Bits 0 to 15 belong to the BMC core. A transport added by an application
 * signals its own client arrivals with these so that it can wait for either
 * new console output or a new client in a single k_event_wait().
 *
 * @param _n Bit index from 0 to 15.
 */
#define BMC_CONSOLE_EVENT_USER(_n) BIT(16 + (_n))

/**
 * @brief Event object signalled on host console activity.
 *
 * Wait on @ref BMC_CONSOLE_EVENT_DATA to be woken when bmc_console_read() has
 * more data to return.
 */
extern struct k_event bmc_console_events;

/**
 * @brief Read captured host console output.
 *
 * Reading never blocks. A return value of 0 means the caller has consumed
 * everything captured so far.
 *
 * @param buf Destination buffer.
 * @param size Size of @p buf in bytes.
 * @param ppos Read position, updated on return. Characters that have already
 *             been overwritten in the circular log are skipped.
 *
 * @return Number of bytes copied, or a negative errno.
 */
ssize_t bmc_console_read(uint8_t *buf, size_t size, uint64_t *ppos);

/**
 * @brief Position a read cursor at the end of the captured output.
 *
 * Used by transports that should only forward output produced after the client
 * connected.
 *
 * @param ppos Read position to update.
 *
 * @return 0 on success, negative errno otherwise.
 */
int bmc_console_seek_end(uint64_t *ppos);

/**
 * @brief Send data to the host console.
 *
 * @param buf Bytes to send.
 * @param size Number of bytes.
 *
 * @return Number of bytes written, or a negative errno.
 */
ssize_t bmc_console_write(const uint8_t *buf, size_t size);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_MGMT_BMC_CONSOLE_H_ */
