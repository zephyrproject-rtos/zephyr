/*
 * Copyright (c) 2026 Siratul Islam <email@sirat.me>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/fido2/fido2_types.h>
#include <zcbor_encode.h>

#include "fido2_cbor.h"

LOG_MODULE_DECLARE(fido2, CONFIG_FIDO2_LOG_LEVEL);

#define GETINFO_KEY_VERSIONS                     0x01
#define GETINFO_KEY_EXTENSIONS                   0x02
#define GETINFO_KEY_AAGUID                       0x03
#define GETINFO_KEY_OPTIONS                      0x04
#define GETINFO_KEY_MAX_MSG_SIZE                 0x05
#define GETINFO_KEY_PIN_UV_AUTH_PROTOCOLS        0x06
#define GETINFO_KEY_MAX_CREDENTIAL_COUNT_IN_LIST 0x07
#define GETINFO_KEY_MAX_CREDENTIAL_ID_LENGTH     0x08
#define GETINFO_KEY_TRANSPORTS                   0x09
#define GETINFO_KEY_FIRMWARE_VERSION             0x0E

int fido2_cbor_encode_get_info(const struct fido2_device_info *info, uint8_t *cbor_out,
			       size_t cbor_out_cap, size_t *cbor_out_len)
{
	ZCBOR_STATE_E(zs, 5, cbor_out, cbor_out_cap, 1);

	if (!zcbor_map_start_encode(zs, 10)) {
		return -ENOMEM;
	}

	/* 0x01: versions */
	if (!zcbor_uint32_put(zs, GETINFO_KEY_VERSIONS) ||
	    !zcbor_list_start_encode(zs, info->num_versions)) {
		return -ENOMEM;
	}
	for (int i = 0; i < info->num_versions; ++i) {
		if (!zcbor_tstr_put_term(zs, info->versions[i], 32)) {
			return -ENOMEM;
		}
	}
	if (!zcbor_list_end_encode(zs, info->num_versions)) {
		return -ENOMEM;
	}

	/* 0x02: extensions */
	if (info->num_extensions > 0) {
		if (!zcbor_uint32_put(zs, GETINFO_KEY_EXTENSIONS) ||
		    !zcbor_list_start_encode(zs, info->num_extensions)) {
			return -ENOMEM;
		}
		for (int i = 0; i < info->num_extensions; ++i) {
			if (!zcbor_tstr_put_term(zs, info->extensions[i], 64)) {
				return -ENOMEM;
			}
		}
		if (!zcbor_list_end_encode(zs, info->num_extensions)) {
			return -ENOMEM;
		}
	}

	/* 0x03: aaguid */
	if (!zcbor_uint32_put(zs, GETINFO_KEY_AAGUID) ||
	    !zcbor_bstr_encode_ptr(zs, info->aaguid, FIDO2_AAGUID_SIZE)) {
		return -ENOMEM;
	}

	/* 0x04: options */
	uint8_t opt_count = 3; /* rk, up, plat always present */

	if (info->options.uv) {
		++opt_count;
	}
	if (info->options.always_uv) {
		++opt_count;
	}
	if (info->options.cred_mgmt) {
		++opt_count;
	}
	if (info->num_pin_uv_auth_protocols > 0) {
		++opt_count;
	}
	if (info->options.pin_uv_auth_token) {
		++opt_count;
	}
	if (info->options.make_cred_uv_not_rqd) {
		++opt_count;
	}
	if (info->options.no_mc_ga_permissions_with_client_pin) {
		++opt_count;
	}

	if (!zcbor_uint32_put(zs, GETINFO_KEY_OPTIONS) || !zcbor_map_start_encode(zs, opt_count)) {
		return -ENOMEM;
	}
	if (!zcbor_tstr_put_lit(zs, "rk") || !zcbor_bool_put(zs, info->options.rk)) {
		return -ENOMEM;
	}
	if (!zcbor_tstr_put_lit(zs, "up") || !zcbor_bool_put(zs, info->options.up)) {
		return -ENOMEM;
	}
	if (info->options.uv) {
		if (!zcbor_tstr_put_lit(zs, "uv") || !zcbor_bool_put(zs, info->options.uv)) {
			return -ENOMEM;
		}
	}
	if (!zcbor_tstr_put_lit(zs, "plat") || !zcbor_bool_put(zs, info->options.plat)) {
		return -ENOMEM;
	}
	if (info->options.always_uv) {
		if (!zcbor_tstr_put_lit(zs, "alwaysUv") || !zcbor_bool_put(zs, true)) {
			return -ENOMEM;
		}
	}
	if (info->options.cred_mgmt) {
		if (!zcbor_tstr_put_lit(zs, "credMgmt") ||
		    !zcbor_bool_put(zs, info->options.cred_mgmt)) {
			return -ENOMEM;
		}
	}
	/*
	 * clientPin: false = supported but not set, true = supported and set,
	 * absent = unsupported. So we check with num_pin_uv_auth_protocols
	 */
	if (info->num_pin_uv_auth_protocols > 0) {
		if (!zcbor_tstr_put_lit(zs, "clientPin") ||
		    !zcbor_bool_put(zs, info->options.client_pin)) {
			return -ENOMEM;
		}
	}
	if (info->options.pin_uv_auth_token) {
		if (!zcbor_tstr_put_lit(zs, "pinUvAuthToken") || !zcbor_bool_put(zs, true)) {
			return -ENOMEM;
		}
	}
	if (info->options.make_cred_uv_not_rqd) {
		if (!zcbor_tstr_put_lit(zs, "makeCredUvNotRqd") || !zcbor_bool_put(zs, true)) {
			return -ENOMEM;
		}
	}
	if (info->options.no_mc_ga_permissions_with_client_pin) {
		if (!zcbor_tstr_put_lit(zs, "noMcGaPermissionsWithClientPin") ||
		    !zcbor_bool_put(zs, true)) {
			return -ENOMEM;
		}
	}
	if (!zcbor_map_end_encode(zs, opt_count)) {
		return -ENOMEM;
	}

	/* 0x05: maxMsgSize */
	if (!zcbor_uint32_put(zs, GETINFO_KEY_MAX_MSG_SIZE) ||
	    !zcbor_uint32_put(zs, info->max_msg_size)) {
		return -ENOMEM;
	}

	/* 0x06: pinUvAuthProtocols */
	if (info->num_pin_uv_auth_protocols > 0) {
		if (!zcbor_uint32_put(zs, GETINFO_KEY_PIN_UV_AUTH_PROTOCOLS) ||
		    !zcbor_list_start_encode(zs, info->num_pin_uv_auth_protocols)) {
			return -ENOMEM;
		}
		for (int i = 0; i < info->num_pin_uv_auth_protocols; ++i) {
			if (!zcbor_uint32_put(zs, info->pin_uv_auth_protocols[i])) {
				return -ENOMEM;
			}
		}
		if (!zcbor_list_end_encode(zs, info->num_pin_uv_auth_protocols)) {
			return -ENOMEM;
		}
	}

	/* 0x07: maxCredentialCountInList */
	if (!zcbor_uint32_put(zs, GETINFO_KEY_MAX_CREDENTIAL_COUNT_IN_LIST) ||
	    !zcbor_uint32_put(zs, info->max_credential_count)) {
		return -ENOMEM;
	}

	/* 0x08: maxCredentialIdLength */
	if (!zcbor_uint32_put(zs, GETINFO_KEY_MAX_CREDENTIAL_ID_LENGTH) ||
	    !zcbor_uint32_put(zs, info->max_credential_id_length)) {
		return -ENOMEM;
	}

	/* 0x09: transports */
	if (info->transports != 0) {
		uint8_t transport_count = 0;

		if (info->transports & FIDO2_TRANSPORT_USB) {
			++transport_count;
		}
		if (info->transports & FIDO2_TRANSPORT_BLE) {
			++transport_count;
		}
		if (info->transports & FIDO2_TRANSPORT_NFC) {
			++transport_count;
		}
		if (!zcbor_uint32_put(zs, GETINFO_KEY_TRANSPORTS) ||
		    !zcbor_list_start_encode(zs, transport_count)) {
			return -ENOMEM;
		}
		if ((info->transports & FIDO2_TRANSPORT_BLE) && !zcbor_tstr_put_lit(zs, "ble")) {
			return -ENOMEM;
		}
		if ((info->transports & FIDO2_TRANSPORT_NFC) && !zcbor_tstr_put_lit(zs, "nfc")) {
			return -ENOMEM;
		}
		if ((info->transports & FIDO2_TRANSPORT_USB) && !zcbor_tstr_put_lit(zs, "usb")) {
			return -ENOMEM;
		}
		if (!zcbor_list_end_encode(zs, transport_count)) {
			return -ENOMEM;
		}
	}

	/* 0x0E: firmwareVersion */
	if (!zcbor_uint32_put(zs, GETINFO_KEY_FIRMWARE_VERSION) ||
	    !zcbor_uint32_put(zs, info->firmware_version)) {
		return -ENOMEM;
	}

	if (!zcbor_map_end_encode(zs, 10)) {
		return -ENOMEM;
	}

	*cbor_out_len = zs->payload - cbor_out;
	return 0;
}
