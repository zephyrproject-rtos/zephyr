/** @file
 *  @brief Bluetooth SPP Protocol handling.
 */

/*
 * Copyright 2025 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/classic/rfcomm.h>

struct bt_spp {
	/** @internal SPP channel */
	uint8_t channel;

	/** @internal Local SPP Service UUID */
	const struct bt_uuid *uuid;

	/** @internal connection handle */
	struct bt_conn *acl_conn;

	/** @internal SPP SDP record */
	struct bt_sdp_record *sdp_rec;

	/** @internal SPP RFCOMM server */
	struct bt_rfcomm_server rfcomm_server;

	/** @internal SPP RFCOMM DLC */
	struct bt_rfcomm_dlc rfcomm_dlc;
};
