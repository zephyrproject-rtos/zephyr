/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "backend.h"

enum z_nfc_backend_kind z_nfc_backend_kind_get(const struct device *dev, nfc_proto_t proto)
{
	const struct nfc_driver_api *api = DEVICE_API_GET(nfc, dev);
	nfc_mode_t modes = nfc_supported_modes(dev, proto);

	if (api->offload_poll_start != NULL && api->offload_exchange != NULL) {
		return Z_NFC_BACKEND_OFFLOAD;
	}

	if ((modes & NFC_MODE_INITIATOR) != 0U && api->im_transceive != NULL) {
		return Z_NFC_BACKEND_FRONTEND_INITIATOR;
	}

	if ((modes & NFC_MODE_TARGET) != 0U && api->target_start != NULL) {
		return Z_NFC_BACKEND_FRONTEND_TARGET;
	}

	return Z_NFC_BACKEND_NONE;
}
