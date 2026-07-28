/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_NFC_COMMON_BACKEND_H_
#define ZEPHYR_SUBSYS_NFC_COMMON_BACKEND_H_

#include <zephyr/device.h>
#include <zephyr/drivers/nfc.h>

/*
 * Backend classification.
 *
 * The driver API does not report a backend class explicitly, so it is derived
 * from the advertised modes and the populated operations. Resolved once by
 * nfc_poller_start() and cached for the session.
 */
enum z_nfc_backend_kind {
	Z_NFC_BACKEND_NONE = 0,
	Z_NFC_BACKEND_FRONTEND_INITIATOR,
	Z_NFC_BACKEND_OFFLOAD,
	Z_NFC_BACKEND_FRONTEND_TARGET,
};

enum z_nfc_backend_kind z_nfc_backend_kind_get(const struct device *dev, nfc_proto_t proto);

#endif /* ZEPHYR_SUBSYS_NFC_COMMON_BACKEND_H_ */
