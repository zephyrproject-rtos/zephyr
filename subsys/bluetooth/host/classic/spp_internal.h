/** @file
 *  @brief Bluetooth SPP Protocol handling.
 */

/*
 * Copyright 2025 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/classic/rfcomm.h>

enum __packed spp_state {
	/* SPP_STATE_DISCONNECTED:
	 * No active SPP connection. Resources are released or not allocated.
	 */
	SPP_STATE_DISCONNECTED,
	/* SPP_STATE_CONNECTING:
	 * Connection is in progress (e.g., SDP discovery or RFCOMM setup).
	 * Not yet available for data transfer.
	 */
	SPP_STATE_CONNECTING,
	/* SPP_STATE_CONNECTED:
	 * RFCOMM DLC is established and SPP is ready for data transfer.
	 */
	SPP_STATE_CONNECTED,
	/* SPP_STATE_DISCONNECTING:
	 * Disconnection is in progress; cleanup and resource release pending.
	 */
	SPP_STATE_DISCONNECTING,
};
