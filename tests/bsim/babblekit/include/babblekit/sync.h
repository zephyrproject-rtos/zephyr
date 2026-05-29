/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file provides a synchronization mechanism between devices, for the
 * simple use-case when there are only two devices in the simulation.
 */


/*
 * @brief Initialize the sync library
 *
 * This initializes a simple synchronization library based on bsim backchannels.
 *
 * Calling `bk_sync_wait()` on device A will make it block until
 * `bk_sync_send()` is called on device B.
 *
 * @note Only works between two devices in a simulation, with IDs 0 and 1.
 *
 * @retval 0 Sync channel operational
 * @retval -1 Failed to open sync channel
 *
 */
int bk_sync_init(void);

/*
 * @brief Send a synchronization packet
 *
 * @note Only works between two devices in a simulation, with IDs 0 and 1.
 *
 */
void bk_sync_send(void);

/*
 * @brief Wait for a synchronization packet
 *
 * This blocks until the other device has called `bk_sync_send()`.
 *
 * @note Only works between two devices in a simulation, with IDs 0 and 1.
 *
 */
void bk_sync_wait(void);
