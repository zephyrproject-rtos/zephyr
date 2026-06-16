/*
 * Copyright (c) 2026 Siratul Islam <email@sirat.me>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/authentication/fido2/fido2_types.h>
#include <zcbor_encode.h>
#include <zcbor_decode.h>

#include "fido2_cbor.h"
#include "fido2_crypto.h"
#include "zcbor_common.h"
#include <stdint.h>
#include <string.h>

LOG_MODULE_DECLARE(fido2, CONFIG_FIDO2_LOG_LEVEL);

/* authenticatorMakeCredential 0x01 */
#define MAKE_CREDENTIAL_KEY_CLIENT_DATA_HASH               0x01
#define MAKE_CREDENTIAL_KEY_RP                             0x02
#define MAKE_CREDENTIAL_KEY_USER                           0x03
#define MAKE_CREDENTIAL_KEY_PUB_KEY_CRED_PARAMS            0x04
#define MAKE_CREDENTIAL_KEY_EXCLUDE_LIST                   0x05
#define MAKE_CREDENTIAL_KEY_EXTENSIONS                     0x06
#define MAKE_CREDENTIAL_KEY_OPTIONS                        0x07
#define MAKE_CREDENTIAL_KEY_PIN_UV_AUTH_PARAM              0x08
#define MAKE_CREDENTIAL_KEY_PIN_UV_AUTH_PROTOCOL           0x09
#define MAKE_CREDENTIAL_KEY_ENTERPRISE_ATTESTATION         0x0A
#define MAKE_CREDENTIAL_KEY_ATTESTATION_FORMATS_PREFERENCE 0x0B
/* authenticatorMakeCredential response */
#define MAKE_CREDENTIAL_RESP_FMT                           0x01
#define MAKE_CREDENTIAL_RESP_AUTH_DATA                     0x02
#define MAKE_CREDENTIAL_RESP_ATT_STMT                      0x03
#define MAKE_CREDENTIAL_RESP_EP_ATT                        0x04
#define MAKE_CREDENTIAL_RESP_LARGE_BLOB_KEY                0x05
#define MAKE_CREDENTIAL_RESP_UNSIGNED_EXT_OUTPUTS          0x06

/* authenticatorGetAssertion 0x02 */
#define GET_ASSERTION_KEY_RP_ID                          0x01
#define GET_ASSERTION_KEY_CLIENT_DATA_HASH               0x02
#define GET_ASSERTION_KEY_ALLOW_LIST                     0x03
#define GET_ASSERTION_KEY_EXTENSIONS                     0x04
#define GET_ASSERTION_KEY_OPTIONS                        0x05
#define GET_ASSERTION_KEY_PIN_UV_AUTH_PARAM              0x06
#define GET_ASSERTION_KEY_PIN_UV_AUTH_PROTOCOL           0x07
#define GET_ASSERTION_KEY_ENTERPRISE_ATTESTATION         0x08
#define GET_ASSERTION_KEY_ATTESTATION_FORMATS_PREFERENCE 0x09
/* authenticatorGetAssertion response */
#define GET_ASSERTION_RESP_CREDENTIAL                    0x01
#define GET_ASSERTION_RESP_AUTH_DATA                     0x02
#define GET_ASSERTION_RESP_SIGNATURE                     0x03
#define GET_ASSERTION_RESP_USER                          0x04
#define GET_ASSERTION_RESP_NUMBER_OF_CREDENTIALS         0x05
#define GET_ASSERTION_RESP_USER_SELECTED                 0x06
#define GET_ASSERTION_RESP_LARGE_BLOB_KEY                0x07
#define GET_ASSERTION_RESP_UNSIGNED_EXT_OUTPUTS          0x08
#define GET_ASSERTION_RESP_EP_ATT                        0x09
#define GET_ASSERTION_RESP_ATT_STMT                      0x0A

/* authenticatorGetInfo 0x04 */
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

static int decode_tstr_field(zcbor_state_t *zs, char *out, size_t out_cap)
{
	struct zcbor_string str;

	if (!zcbor_tstr_decode(zs, &str)) {
		return -EBADMSG;
	}
	if (str.len >= out_cap) {
		return -EBADMSG;
	}

	memcpy(out, str.value, str.len);
	out[str.len] = '\0';

	return 0;
}

static int mc_decode_rp(zcbor_state_t *zs, struct fido2_make_credential_params *params)
{
	struct zcbor_string str;
	bool has_rp_id = false;

	if (!zcbor_map_start_decode(zs)) {
		return -EBADMSG;
	}
	while (!zcbor_array_at_end(zs)) {
		if (!zcbor_tstr_decode(zs, &str)) {
			return -EBADMSG;
		}
		if (str.len == 2 && memcmp(str.value, "id", 2) == 0) {
			if (decode_tstr_field(zs, params->rp_id, sizeof(params->rp_id))) {
				return -EBADMSG;
			}
			has_rp_id = true;
		} else if (str.len == 4 && memcmp(str.value, "name", 4) == 0) {
			if (decode_tstr_field(zs, params->rp_name, sizeof(params->rp_name))) {
				return -EBADMSG;
			}
		} else {
			if (!zcbor_any_skip(zs, NULL)) {
				return -EBADMSG;
			}
		}
	}
	if (!zcbor_map_end_decode(zs)) {
		return -EBADMSG;
	}
	if (!has_rp_id) {
		return -EBADMSG;
	}

	return 0;
}

static int mc_decode_user(zcbor_state_t *zs, struct fido2_make_credential_params *params)
{
	struct zcbor_string str;
	bool has_user_id = false;

	if (!zcbor_map_start_decode(zs)) {
		return -EBADMSG;
	}
	while (!zcbor_array_at_end(zs)) {
		if (!zcbor_tstr_decode(zs, &str)) {
			return -EBADMSG;
		}
		if (str.len == 2 && memcmp(str.value, "id", 2) == 0) {
			if (!zcbor_bstr_decode(zs, &str)) {
				return -EBADMSG;
			}
			if (str.len > sizeof(params->user_id)) {
				return -EBADMSG;
			}
			params->user_id_len = str.len;
			memcpy(params->user_id, str.value, str.len);
			has_user_id = true;
		} else if (str.len == 4 && memcmp(str.value, "name", 4) == 0) {
			if (decode_tstr_field(zs, params->user_name, sizeof(params->user_name))) {
				return -EBADMSG;
			}

		} else if (str.len == 11 && memcmp(str.value, "displayName", 11) == 0) {
			if (decode_tstr_field(zs, params->user_display_name,
					      sizeof(params->user_display_name))) {
				return -EBADMSG;
			}
		} else {
			if (!zcbor_any_skip(zs, NULL)) {
				return -EBADMSG;
			}
		}
	}
	if (!zcbor_map_end_decode(zs)) {
		return -EBADMSG;
	}
	if (!has_user_id) {
		return -EBADMSG;
	}

	return 0;
}

static int mc_decode_pub_key_cred_params(zcbor_state_t *zs,
					 struct fido2_make_credential_params *params)
{
	struct zcbor_string str;

	if (!zcbor_list_start_decode(zs)) {
		return -EBADMSG;
	}
	while (!zcbor_array_at_end(zs)) {
		int32_t alg = 0;
		bool has_alg = false;
		bool has_type = false;
		bool valid_type = false;

		if (!zcbor_map_start_decode(zs)) {
			return -EBADMSG;
		}
		while (!zcbor_array_at_end(zs)) {
			if (!zcbor_tstr_decode(zs, &str)) {
				return -EBADMSG;
			}

			if (str.len == 3 && memcmp(str.value, "alg", 3) == 0) {
				if (!zcbor_int32_decode(zs, &alg)) {
					return -EBADMSG;
				}
				has_alg = true;
			} else if (str.len == 4 && memcmp(str.value, "type", 4) == 0) {
				struct zcbor_string type_str;

				if (!zcbor_tstr_decode(zs, &type_str)) {
					return -EBADMSG;
				}
				has_type = true;
				if (type_str.len == 10 &&
				    memcmp(type_str.value, "public-key", 10) == 0) {
					valid_type = true;
				}
			} else {
				if (!zcbor_any_skip(zs, NULL)) {
					return -EBADMSG;
				}
			}
		}
		if (!zcbor_map_end_decode(zs)) {
			return -EBADMSG;
		}

		if (!has_alg || !has_type) {
			return -EBADMSG;
		}

		if (valid_type) {
			if (params->num_algorithms >= FIDO2_MAX_ALGORITHMS) {
				return -EBADMSG;
			}
			params->algorithms[params->num_algorithms++] = alg;
		}
	}
	if (!zcbor_list_end_decode(zs)) {
		return -EBADMSG;
	}

	return 0;
}

static int mc_decode_exclude_list(zcbor_state_t *zs, struct fido2_make_credential_params *params)
{
	struct zcbor_string str;

	if (!zcbor_list_start_decode(zs)) {
		return -EBADMSG;
	}
	while (!zcbor_array_at_end(zs)) {
		struct zcbor_string cred_id;
		bool valid_type = false;
		bool has_id = false;

		if (!zcbor_map_start_decode(zs)) {
			return -EBADMSG;
		}
		while (!zcbor_array_at_end(zs)) {
			if (!zcbor_tstr_decode(zs, &str)) {
				return -EBADMSG;
			}
			if (str.len == 2 && memcmp(str.value, "id", 2) == 0) {
				if (!zcbor_bstr_decode(zs, &cred_id)) {
					return -EBADMSG;
				}
				has_id = true;
			} else if (str.len == 4 && memcmp(str.value, "type", 4) == 0) {
				struct zcbor_string type_str;

				if (!zcbor_tstr_decode(zs, &type_str)) {
					return -EBADMSG;
				}
				if (type_str.len == 10 &&
				    memcmp(type_str.value, "public-key", 10) == 0) {
					valid_type = true;
				}
			} else {
				if (!zcbor_any_skip(zs, NULL)) {
					return -EBADMSG;
				}
			}
		}
		if (!zcbor_map_end_decode(zs)) {
			return -EBADMSG;
		}

		if (!has_id || (cred_id.len == 0)) {
			return -EBADMSG;
		}

		if (valid_type) {
			if (params->num_exclude >= CONFIG_FIDO2_MAX_CREDENTIALS) {
				return -ENOBUFS;
			}
			if (cred_id.len > FIDO2_CREDENTIAL_ID_MAX_SIZE) {
				return -EBADMSG;
			}
			memcpy(params->exclude_ids[params->num_exclude], cred_id.value,
			       cred_id.len);
			params->exclude_id_lens[params->num_exclude] = cred_id.len;
			params->num_exclude++;
		}
	}
	if (!zcbor_list_end_decode(zs)) {
		return -EBADMSG;
	}

	return 0;
}

static int mc_decode_extensions(zcbor_state_t *zs, struct fido2_make_credential_params *params)
{
	struct zcbor_string str;

	if (!zcbor_map_start_decode(zs)) {
		return -EBADMSG;
	}
	while (!zcbor_array_at_end(zs)) {
		if (!zcbor_tstr_decode(zs, &str)) {
			return -EBADMSG;
		}
		if (str.len == 11 && memcmp(str.value, "hmac-secret", 11) == 0) {
			bool val;

			if (!zcbor_bool_decode(zs, &val)) {
				return -EBADMSG;
			}
			params->ext_hmac_secret = val;
		} else if (str.len == 11 && memcmp(str.value, "credProtect", 11) == 0) {
			uint32_t level;

			if (!zcbor_uint32_decode(zs, &level)) {
				return -EBADMSG;
			}
			if (level < 1 || level > 3) {
				return -EBADMSG;
			}
			params->ext_cred_protect_level = level;
		} else {
			if (!zcbor_any_skip(zs, NULL)) {
				return -EBADMSG;
			}
		}
	}
	if (!zcbor_map_end_decode(zs)) {
		return -EBADMSG;
	}

	return 0;
}

static int mc_decode_options(zcbor_state_t *zs, struct fido2_make_credential_params *params)
{
	struct zcbor_string str;

	if (!zcbor_map_start_decode(zs)) {
		return -EBADMSG;
	}
	while (!zcbor_array_at_end(zs)) {
		if (!zcbor_tstr_decode(zs, &str)) {
			return -EBADMSG;
		}
		if (str.len == 2 && memcmp(str.value, "rk", 2) == 0) {
			bool rk;

			if (!zcbor_bool_decode(zs, &rk)) {
				return -EBADMSG;
			}
			params->resident_key = rk;
		} else if (str.len == 2 && memcmp(str.value, "up", 2) == 0) {
			bool up;

			if (!zcbor_bool_decode(zs, &up)) {
				return -EBADMSG;
			}
			params->up = up;
			params->has_up_option = true;
		} else if (str.len == 2 && memcmp(str.value, "uv", 2) == 0) {
			bool uv;

			if (!zcbor_bool_decode(zs, &uv)) {
				return -EBADMSG;
			}
			params->uv = uv;
			params->has_uv_option = true;
		} else {
			if (!zcbor_any_skip(zs, NULL)) {
				return -EBADMSG;
			}
		}
	}
	if (!zcbor_map_end_decode(zs)) {
		return -EBADMSG;
	}

	return 0;
}

static int mc_decode_attestation_formats(zcbor_state_t *zs,
					 struct fido2_make_credential_params *params)
{
	struct zcbor_string str;

	if (!zcbor_list_start_decode(zs)) {
		return -EBADMSG;
	}
	while (!zcbor_array_at_end(zs)) {
		if (!zcbor_tstr_decode(zs, &str)) {
			return -EBADMSG;
		}
		if (str.len > FIDO2_CBOR_ATTESTATION_FMT_MAX_LEN) {
			return -EBADMSG;
		}

		if (params->num_attestation_formats < FIDO2_CBOR_MAX_ATTESTATION_FORMATS) {
			memcpy(params->attestation_formats[params->num_attestation_formats],
			       str.value, str.len);
			params->attestation_formats[params->num_attestation_formats][str.len] =
				'\0';
			params->num_attestation_formats++;
		}
	}
	if (!zcbor_list_end_decode(zs)) {
		return -EBADMSG;
	}

	return 0;
}

int fido2_cbor_decode_make_credential(const uint8_t *cbor_in, size_t cbor_in_len,
				      struct fido2_make_credential_params *params)
{
	ZCBOR_STATE_D(zs, 5, cbor_in, cbor_in_len, 1, 0);
	struct zcbor_string str;
	uint32_t key;

	memset(params, 0, sizeof(*params));
	params->up = true;

	/* To see if these were seen at all*/
	bool has_cdh = false;
	bool has_rp = false;
	bool has_user = false;
	bool has_pkcp = false;

	if (!zcbor_map_start_decode(zs)) {
		return -EBADMSG;
	}

	while (!zcbor_array_at_end(zs)) {
		if (!zcbor_uint32_decode(zs, &key)) {
			return -EBADMSG;
		}

		switch (key) {
		case MAKE_CREDENTIAL_KEY_CLIENT_DATA_HASH:
			if (!zcbor_bstr_decode(zs, &str) || str.len != FIDO2_SHA256_SIZE) {
				return -EBADMSG;
			}
			memcpy(params->client_data_hash, str.value, FIDO2_SHA256_SIZE);
			has_cdh = true;
			break;
		case MAKE_CREDENTIAL_KEY_RP:
			if (mc_decode_rp(zs, params)) {
				return -EBADMSG;
			}
			has_rp = true;
			break;
		case MAKE_CREDENTIAL_KEY_USER:
			if (mc_decode_user(zs, params)) {
				return -EBADMSG;
			}
			has_user = true;
			break;
		case MAKE_CREDENTIAL_KEY_PUB_KEY_CRED_PARAMS:
			if (mc_decode_pub_key_cred_params(zs, params)) {
				return -EBADMSG;
			}
			has_pkcp = true;
			break;
		case MAKE_CREDENTIAL_KEY_EXCLUDE_LIST:
			if (mc_decode_exclude_list(zs, params)) {
				return -EBADMSG;
			}
			break;
		case MAKE_CREDENTIAL_KEY_EXTENSIONS:
			if (mc_decode_extensions(zs, params)) {
				return -EBADMSG;
			}
			break;
		case MAKE_CREDENTIAL_KEY_OPTIONS:
			if (mc_decode_options(zs, params)) {
				return -EBADMSG;
			}
			break;
		case MAKE_CREDENTIAL_KEY_PIN_UV_AUTH_PARAM:
			if (!zcbor_bstr_decode(zs, &str)) {
				return -EBADMSG;
			}
			if (str.len > sizeof(params->pin_uv_auth_param)) {
				return -EBADMSG;
			}
			memcpy(params->pin_uv_auth_param, str.value, str.len);
			params->pin_uv_auth_param_len = str.len;
			params->has_pin_uv_auth_param = true;
			break;
		case MAKE_CREDENTIAL_KEY_PIN_UV_AUTH_PROTOCOL: {
			uint32_t proto;

			if (!zcbor_uint32_decode(zs, &proto)) {
				return -EBADMSG;
			}
			params->pin_uv_auth_protocol = proto;
			params->has_pin_uv_auth_protocol = true;
			break;
		}
		case MAKE_CREDENTIAL_KEY_ENTERPRISE_ATTESTATION: {
			uint32_t ea;

			if (!zcbor_uint32_decode(zs, &ea)) {
				return -EBADMSG;
			}
			params->enterprise_attestation = ea;
			params->has_enterprise_attestation = true;
			break;
		}
		case MAKE_CREDENTIAL_KEY_ATTESTATION_FORMATS_PREFERENCE:
			if (mc_decode_attestation_formats(zs, params)) {
				return -EBADMSG;
			}
			break;
		default:
			if (!zcbor_any_skip(zs, NULL)) {
				return -EBADMSG;
			}
			break;
		}
	}

	if (!zcbor_map_end_decode(zs)) {
		return -EBADMSG;
	}

	if (!has_cdh || !has_rp || !has_user || !has_pkcp) {
		return -EINVAL;
	}

	return 0;
}

static int ga_decode_allow_list(zcbor_state_t *zs, struct fido2_get_assertion_params *params)
{
	struct zcbor_string str;

	if (!zcbor_list_start_decode(zs)) {
		return -EBADMSG;
	}
	while (!zcbor_array_at_end(zs)) {
		struct zcbor_string cred_id;
		bool valid_type = false;
		bool has_id = false;

		if (!zcbor_map_start_decode(zs)) {
			return -EBADMSG;
		}
		while (!zcbor_array_at_end(zs)) {
			if (!zcbor_tstr_decode(zs, &str)) {
				return -EBADMSG;
			}
			if (str.len == 2 && memcmp(str.value, "id", 2) == 0) {
				if (!zcbor_bstr_decode(zs, &cred_id)) {
					return -EBADMSG;
				}
				has_id = true;
			} else if (str.len == 4 && memcmp(str.value, "type", 4) == 0) {
				struct zcbor_string type_str;

				if (!zcbor_tstr_decode(zs, &type_str)) {
					return -EBADMSG;
				}
				if (type_str.len == 10 &&
				    memcmp(type_str.value, "public-key", 10) == 0) {
					valid_type = true;
				}
			} else {
				if (!zcbor_any_skip(zs, NULL)) {
					return -EBADMSG;
				}
			}
		}
		if (!zcbor_map_end_decode(zs)) {
			return -EBADMSG;
		}

		if (!has_id || (cred_id.len == 0)) {
			return -EBADMSG;
		}

		if (valid_type) {
			if (params->num_allow >= CONFIG_FIDO2_MAX_CREDENTIALS) {
				return -ENOBUFS;
			}
			if (cred_id.len > FIDO2_CREDENTIAL_ID_MAX_SIZE) {
				return -EBADMSG;
			}
			memcpy(params->allow_ids[params->num_allow], cred_id.value, cred_id.len);
			params->allow_id_lens[params->num_allow] = cred_id.len;
			params->num_allow++;
		}
	}
	if (!zcbor_list_end_decode(zs)) {
		return -EBADMSG;
	}

	return 0;
}

static int ga_decode_options(zcbor_state_t *zs, struct fido2_get_assertion_params *params)
{
	struct zcbor_string str;

	if (!zcbor_map_start_decode(zs)) {
		return -EBADMSG;
	}
	while (!zcbor_array_at_end(zs)) {
		if (!zcbor_tstr_decode(zs, &str)) {
			return -EBADMSG;
		}
		if (str.len == 2 && memcmp(str.value, "up", 2) == 0) {
			bool up;

			if (!zcbor_bool_decode(zs, &up)) {
				return -EBADMSG;
			}
			params->up = up;
			params->has_up_option = true;
		} else if (str.len == 2 && memcmp(str.value, "uv", 2) == 0) {
			bool uv;

			if (!zcbor_bool_decode(zs, &uv)) {
				return -EBADMSG;
			}
			params->uv = uv;
			params->has_uv_option = true;
		} else {
			if (!zcbor_any_skip(zs, NULL)) {
				return -EBADMSG;
			}
		}
	}
	if (!zcbor_map_end_decode(zs)) {
		return -EBADMSG;
	}

	return 0;
}

static int ga_decode_attestation_formats(zcbor_state_t *zs,
					 struct fido2_get_assertion_params *params)
{
	struct zcbor_string str;

	if (!zcbor_list_start_decode(zs)) {
		return -EBADMSG;
	}
	while (!zcbor_array_at_end(zs)) {
		if (!zcbor_tstr_decode(zs, &str)) {
			return -EBADMSG;
		}
		if (str.len > FIDO2_CBOR_ATTESTATION_FMT_MAX_LEN) {
			return -EBADMSG;
		}

		if (params->num_attestation_formats < FIDO2_CBOR_MAX_ATTESTATION_FORMATS) {
			memcpy(params->attestation_formats[params->num_attestation_formats],
			       str.value, str.len);
			params->attestation_formats[params->num_attestation_formats][str.len] =
				'\0';
			params->num_attestation_formats++;
		}
	}
	if (!zcbor_list_end_decode(zs)) {
		return -EBADMSG;
	}

	return 0;
}

int fido2_cbor_decode_get_assertion(const uint8_t *cbor_in, size_t cbor_in_len,
				    struct fido2_get_assertion_params *params)
{
	ZCBOR_STATE_D(zs, 5, cbor_in, cbor_in_len, 1, 0);
	uint32_t key;
	struct zcbor_string str;

	/* To see if these were seen at all*/
	bool has_rp_id = false;
	bool has_cdh = false;

	memset(params, 0, sizeof(*params));
	params->up = true;

	if (!zcbor_map_start_decode(zs)) {
		return -EBADMSG;
	}

	while (!zcbor_array_at_end(zs)) {
		if (!zcbor_uint32_decode(zs, &key)) {
			return -EBADMSG;
		}

		switch (key) {
		case GET_ASSERTION_KEY_RP_ID:
			if (decode_tstr_field(zs, params->rp_id, sizeof(params->rp_id))) {
				return -EBADMSG;
			}
			has_rp_id = true;
			break;
		case GET_ASSERTION_KEY_CLIENT_DATA_HASH:
			if (!zcbor_bstr_decode(zs, &str) || str.len != FIDO2_SHA256_SIZE) {
				return -EBADMSG;
			}
			memcpy(params->client_data_hash, str.value, str.len);
			has_cdh = true;
			break;
		case GET_ASSERTION_KEY_ALLOW_LIST:
			if (ga_decode_allow_list(zs, params)) {
				return -EBADMSG;
			}
			break;
		case GET_ASSERTION_KEY_EXTENSIONS:
			/* TODO: Optional feature - later*/
			if (!zcbor_any_skip(zs, NULL)) {
				return -EBADMSG;
			}
			break;
		case GET_ASSERTION_KEY_OPTIONS:
			if (ga_decode_options(zs, params)) {
				return -EBADMSG;
			}
			break;
		case GET_ASSERTION_KEY_PIN_UV_AUTH_PARAM:
			if (!zcbor_bstr_decode(zs, &str)) {
				return -EBADMSG;
			}
			if (str.len > sizeof(params->pin_uv_auth_param)) {
				return -EBADMSG;
			}
			memcpy(params->pin_uv_auth_param, str.value, str.len);
			params->pin_uv_auth_param_len = str.len;
			params->has_pin_uv_auth_param = true;
			break;
		case GET_ASSERTION_KEY_PIN_UV_AUTH_PROTOCOL: {
			uint32_t proto;

			if (!zcbor_uint32_decode(zs, &proto)) {
				return -EBADMSG;
			}
			params->pin_uv_auth_protocol = (uint8_t)proto;
			params->has_pin_uv_auth_protocol = true;
			break;
		}
		case GET_ASSERTION_KEY_ENTERPRISE_ATTESTATION: {
			uint32_t ea;

			if (!zcbor_uint32_decode(zs, &ea)) {
				return -EBADMSG;
			}
			params->enterprise_attestation = (uint8_t)ea;
			params->has_enterprise_attestation = true;
			break;
		}
		case GET_ASSERTION_KEY_ATTESTATION_FORMATS_PREFERENCE:
			if (ga_decode_attestation_formats(zs, params)) {
				return -EBADMSG;
			}
			break;
		default:
			if (!zcbor_any_skip(zs, NULL)) {
				return -EBADMSG;
			}
			break;
		}
	}

	if (!zcbor_map_end_decode(zs)) {
		return -EBADMSG;
	}

	if (!has_rp_id || !has_cdh) {
		return -EINVAL;
	}

	return 0;
}

int fido2_cbor_encode_cose_key(const uint8_t *pub_key, size_t pub_key_len, uint8_t *cbor_out,
			       size_t cbor_out_cap, size_t *cbor_out_len)
{
	ZCBOR_STATE_E(zs, 5, cbor_out, cbor_out_cap, 1);
	const uint8_t *x;
	const uint8_t *y;

	if (pub_key_len < FIDO2_P256_UNCOMPRESSED_KEY_SIZE ||
	    pub_key[0] != FIDO2_EC_POINT_UNCOMPRESSED) {
		return -EINVAL;
	}

	x = pub_key + 1;
	y = pub_key + 1 + FIDO2_P256_COORD_SIZE;

	if (!zcbor_map_start_encode(zs, 5)) {
		return -ENOMEM;
	}

	/* 1 (kty) = 2 (EC2) */
	if (!zcbor_int32_put(zs, 1) || !zcbor_int32_put(zs, 2)) {
		return -ENOMEM;
	}
	/* 3 (alg) = -7 (ES256) */
	if (!zcbor_int32_put(zs, 3) || !zcbor_int32_put(zs, FIDO2_COSE_ES256)) {
		return -ENOMEM;
	}
	/* -1 (crv) = 1 (P-256) */
	if (!zcbor_int32_put(zs, -1) || !zcbor_int32_put(zs, 1)) {
		return -ENOMEM;
	}
	/* -2 (x) = x coordinate */
	if (!zcbor_int32_put(zs, -2) || !zcbor_bstr_encode_ptr(zs, x, FIDO2_P256_COORD_SIZE)) {
		return -ENOMEM;
	}
	/* -3 (x) = x coordinate */
	if (!zcbor_int32_put(zs, -3) || !zcbor_bstr_encode_ptr(zs, y, FIDO2_P256_COORD_SIZE)) {
		return -ENOMEM;
	}

	if (!zcbor_map_end_encode(zs, 5)) {
		return -ENOMEM;
	}

	*cbor_out_len = zs->payload - cbor_out;

	return 0;
}

int fido2_cbor_encode_make_credential_resp(const uint8_t *auth_data, size_t auth_data_len,
					   const struct fido2_attestation_result *att_result,
					   uint8_t *cbor_out, size_t cbor_out_cap,
					   size_t *cbor_out_len)
{
	ZCBOR_STATE_E(zs, 5, cbor_out, cbor_out_cap, 1);

	uint8_t att_stmt_entries = 0;

	if (att_result->sig != NULL && att_result->sig_len > 0) {
		att_stmt_entries = 2; /* alg + sig */
		if (att_result->x5c != NULL && att_result->x5c_len > 0) {
			att_stmt_entries = 3; /* alg + sig + x5c */
		}
	}

	if (!zcbor_map_start_encode(zs, 3)) {
		return -ENOMEM;
	}

	/* 0x01: fmt */
	if (!zcbor_uint32_put(zs, MAKE_CREDENTIAL_RESP_FMT) ||
	    !zcbor_tstr_put_term(zs, att_result->fmt, FIDO2_ATTESTATION_FMT_MAX_LEN)) {
		return -ENOMEM;
	}
	/* 0x02: authData */
	if (!zcbor_uint32_put(zs, MAKE_CREDENTIAL_RESP_AUTH_DATA) ||
	    !zcbor_bstr_encode_ptr(zs, auth_data, auth_data_len)) {
		return -ENOMEM;
	}
	/* 0x03: attStmt */
	if (!zcbor_uint32_put(zs, MAKE_CREDENTIAL_RESP_ATT_STMT) ||
	    !zcbor_map_start_encode(zs, att_stmt_entries)) {
		return -ENOMEM;
	}
	if (att_stmt_entries >= 2) {
		if (!zcbor_tstr_put_lit(zs, "alg") || !zcbor_int32_put(zs, att_result->alg)) {
			return -ENOMEM;
		}
		if (!zcbor_tstr_put_lit(zs, "sig") ||
		    !zcbor_bstr_encode_ptr(zs, att_result->sig, att_result->sig_len)) {
			return -ENOMEM;
		}
	}
	if (att_stmt_entries >= 3) {
		if (!zcbor_tstr_put_lit(zs, "x5c") || !zcbor_list_start_encode(zs, 1)) {
			return -ENOMEM;
		}
		if (!zcbor_bstr_encode_ptr(zs, att_result->x5c, att_result->x5c_len)) {
			return -ENOMEM;
		}
		if (!zcbor_list_end_encode(zs, 1)) {
			return -ENOMEM;
		}
	}
	if (!zcbor_map_end_encode(zs, att_stmt_entries)) {
		return -ENOMEM;
	}

	if (!zcbor_map_end_encode(zs, 3)) {
		return -ENOMEM;
	}

	*cbor_out_len = zs->payload - cbor_out;

	return 0;
}

int fido2_cbor_encode_get_assertion_resp(const uint8_t *cred_id, size_t cred_id_len,
					 const uint8_t *auth_data, size_t auth_data_len,
					 const uint8_t *sig, size_t sig_len, const uint8_t *user_id,
					 size_t user_id_len, uint16_t num_credentials,
					 uint8_t *cbor_out, size_t cbor_out_cap,
					 size_t *cbor_out_len)
{
	ZCBOR_STATE_E(zs, 5, cbor_out, cbor_out_cap, 1);
	uint8_t num_entries = 3; /* credential, authData, sig always present */

	if (user_id != NULL && user_id_len > 0) {
		++num_entries;
	}
	if (num_credentials > 1) {
		++num_entries;
	}

	if (!zcbor_map_start_encode(zs, num_entries)) {
		return -ENOMEM;
	}

	/* 0x01: credential */
	if (!zcbor_uint32_put(zs, GET_ASSERTION_RESP_CREDENTIAL) ||
	    !zcbor_map_start_encode(zs, 2)) {
		return -ENOMEM;
	}
	if (!zcbor_tstr_put_lit(zs, "id") || !zcbor_bstr_encode_ptr(zs, cred_id, cred_id_len)) {
		return -ENOMEM;
	}
	if (!zcbor_tstr_put_lit(zs, "type") || !zcbor_tstr_put_lit(zs, "public-key")) {
		return -ENOMEM;
	}
	if (!zcbor_map_end_encode(zs, 2)) {
		return -ENOMEM;
	}

	/* 0x02: authData */
	if (!zcbor_uint32_put(zs, GET_ASSERTION_RESP_AUTH_DATA) ||
	    !zcbor_bstr_encode_ptr(zs, auth_data, auth_data_len)) {
		return -ENOMEM;
	}

	/* 0x03: signature */
	if (!zcbor_uint32_put(zs, GET_ASSERTION_RESP_SIGNATURE) ||
	    !zcbor_bstr_encode_ptr(zs, sig, sig_len)) {
		return -ENOMEM;
	}

	/* 0x04: user */
	if (user_id != NULL && user_id_len > 0) {
		if (!zcbor_uint32_put(zs, GET_ASSERTION_RESP_USER) ||
		    !zcbor_map_start_encode(zs, 1)) {
			return -ENOMEM;
		}
		if (!zcbor_tstr_put_lit(zs, "id") ||
		    !zcbor_bstr_encode_ptr(zs, user_id, user_id_len)) {
			return -ENOMEM;
		}
		if (!zcbor_map_end_encode(zs, 1)) {
			return -ENOMEM;
		}
	}

	/* 0x05: numberOfCredentials */
	if (num_credentials > 1) {
		if (!zcbor_uint32_put(zs, GET_ASSERTION_RESP_NUMBER_OF_CREDENTIALS) ||
		    !zcbor_uint32_put(zs, num_credentials)) {
			return -ENOMEM;
		}
	}

	if (!zcbor_map_end_encode(zs, num_entries)) {
		return -ENOMEM;
	}

	*cbor_out_len = zs->payload - cbor_out;

	return 0;
}
