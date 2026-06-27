/*
 * Copyright (c) 2026 Siratul Islam <email@sirat.me>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include <zephyr/authentication/fido2/fido2.h>
#include <zephyr/authentication/fido2/fido2_attestation.h>
#include <zephyr/authentication/fido2/fido2_storage.h>
#include <zephyr/authentication/fido2/fido2_transport.h>
#include <zephyr/authentication/fido2/fido2_types.h>
#include <zephyr/authentication/fido2/fido2_up.h>

#include "fido2_cbor.h"
#include "fido2_crypto.h"

LOG_MODULE_REGISTER(fido2, CONFIG_FIDO2_LOG_LEVEL);

struct fido2_msg {
	const struct fido2_transport *transport;
	uint32_t cid;
	size_t len;
	/* data[0] = cmd, data[1..n] = CBOR */
	uint8_t data[CONFIG_FIDO2_CBOR_MAX_SIZE + 1];
};

/*
 * The transport callback may run on the system workqueue where stack can
 * be tight. Keep the queue item out.
 */
K_MUTEX_DEFINE(rx_enqueue_mutex);
static struct fido2_msg rx_enqueue_msg;
/* Reused to minimize thread stack usage. */
static struct fido2_msg rx_dequeue_msg;

K_MSGQ_DEFINE(fido2_msgq, sizeof(struct fido2_msg), 2, 4);

static K_THREAD_STACK_DEFINE(fido2_stack, CONFIG_FIDO2_THREAD_STACK_SIZE);
static struct k_thread fido2_thread;

/* Kept these out to keep thread stack light */
/* makeCredential */
static struct fido2_make_credential_params mc_params;
static struct fido2_credential mc_credential;
/* getAssertion */
static struct fido2_get_assertion_params ga_params;
static struct fido2_credential ga_credentials[CONFIG_FIDO2_MAX_CREDENTIALS];
/* getNextAssertion */
static size_t ga_next_count;
static size_t ga_next_index;
static uint8_t ga_next_client_data_hash[FIDO2_SHA256_SIZE];
static uint8_t ga_next_rp_id_hash[FIDO2_SHA256_SIZE];
static uint8_t ga_next_flags;
static k_timepoint_t ga_next_deadline;
static bool ga_next_pending;

static uint8_t ctap_tx_frame[CONFIG_FIDO2_CBOR_MAX_SIZE];

static atomic_t fido2_running;

static k_timepoint_t reset_timeout;

static struct k_spinlock state_lock;
static enum fido2_runtime_state runtime_state = FIDO2_RUNTIME_STATE_STOPPED;
static fido2_state_callback_t runtime_state_cb;
static void *runtime_state_cb_user_data;

static void set_runtime_state(enum fido2_runtime_state state)
{
	fido2_state_callback_t cb;
	void *cb_user_data;
	bool changed;
	k_spinlock_key_t key;

	key = k_spin_lock(&state_lock);
	changed = state != runtime_state;
	runtime_state = state;
	cb = runtime_state_cb;
	cb_user_data = runtime_state_cb_user_data;
	k_spin_unlock(&state_lock, key);

	if (changed && cb != NULL) {
		cb(state, cb_user_data);
	}
}

static void notify_wire(const struct fido2_transport *transport, uint32_t cid,
			enum fido2_wire_status status)
{
	if (transport && transport->api && transport->api->notify) {
		transport->api->notify(cid, status);
	}
}

static bool mc_check_exclude_list(void)
{
	for (int i = 0; i < mc_params.num_exclude; ++i) {
		if (mc_params.exclude_id_lens[i] == FIDO2_DISCOVERABLE_CRED_ID_SIZE) {
			struct fido2_credential existing_cred;

			if (fido2_storage_load(mc_params.exclude_ids[i],
					       mc_params.exclude_id_lens[i], &existing_cred) != 0) {
				continue;
			}
			/* This check is needed per spec */
			if (memcmp(existing_cred.rp_id_hash, mc_credential.rp_id_hash,
				   FIDO2_SHA256_SIZE) != 0) {
				continue;
			}
#if defined(CONFIG_FIDO2_EXT_CRED_PROTECT)
			if (existing_cred.cred_protect == FIDO2_CRED_PROTECT_UV_REQUIRED) {
				continue;
			}
#endif
			return true;
		} else if (mc_params.exclude_id_lens[i] == FIDO2_NON_DISCOVERABLE_CRED_ID_SIZE) {
			uint32_t key_id;

			if (fido2_crypto_unwrap_credential(mc_params.exclude_ids[i],
							   mc_credential.rp_id_hash,
							   &key_id) != 0) {
				continue;
			}

			return true;
		}
	}

	return false;
}

static int mc_populate_credential(void)
{
	int ret;

	strncpy(mc_credential.rp_id, mc_params.rp_id, sizeof(mc_credential.rp_id) - 1);
	strncpy(mc_credential.rp_name, mc_params.rp_name, sizeof(mc_credential.rp_name) - 1);
	memcpy(mc_credential.user_id, mc_params.user_id, mc_params.user_id_len);
	mc_credential.user_id_len = mc_params.user_id_len;
	strncpy(mc_credential.user_name, mc_params.user_name, sizeof(mc_credential.user_name) - 1);
	strncpy(mc_credential.user_display_name, mc_params.user_display_name,
		sizeof(mc_credential.user_display_name) - 1);
	mc_credential.sign_count = 0;
	mc_credential.algorithm = FIDO2_COSE_ES256;
	mc_credential.discoverable = mc_params.resident_key;

#if defined(CONFIG_FIDO2_EXT_CRED_PROTECT)
	mc_credential.cred_protect = mc_params.ext_cred_protect_level;
#endif
#if defined(CONFIG_FIDO2_EXT_HMAC_SECRET)
	if (mc_params.ext_hmac_secret) {
		mc_credential.extensions |= FIDO2_EXT_HMAC_SECRET;
	}
#endif

	if (mc_params.resident_key) {
		mc_credential.id_len = FIDO2_DISCOVERABLE_CRED_ID_SIZE;
		ret = sys_csrand_get(mc_credential.id, mc_credential.id_len);
		if (ret) {
			LOG_ERR("Credential ID csrand failed: %d", ret);
			return ret;
		}
	} else {
		mc_credential.id_len = FIDO2_NON_DISCOVERABLE_CRED_ID_SIZE;
		ret = fido2_crypto_wrap_credential(mc_credential.key_id, mc_credential.rp_id_hash,
						   mc_credential.id);
		if (ret) {
			LOG_ERR("Credential ID wrap failed: %d", ret);
			return ret;
		}
	}

	return 0;
}

static int mc_store_discoverable(void)
{
	size_t creds_found;
	int ret;

	if (fido2_storage_find_by_rp(mc_credential.rp_id_hash, ga_credentials,
				     CONFIG_FIDO2_MAX_CREDENTIALS, &creds_found) == 0) {
		for (int i = 0; i < creds_found; ++i) {
			if (mc_credential.user_id_len == ga_credentials[i].user_id_len &&
			    memcmp(mc_credential.user_id, ga_credentials[i].user_id,
				   mc_credential.user_id_len) == 0) {
				fido2_storage_remove(ga_credentials[i].id,
						     ga_credentials[i].id_len);
				break;
			}
		}
	}

	ret = fido2_storage_store(&mc_credential);

	if (ret) {
		LOG_ERR("Discoverable credential store failed: %d", ret);
		return ret;
	}

	return 0;
}

static int mc_build_auth_data(const uint8_t rp_id_hash[FIDO2_SHA256_SIZE], const uint8_t *cred_id,
			      size_t cred_id_len, const uint8_t *pub_key, size_t pub_key_len,
			      uint32_t sign_count, uint8_t flags, uint8_t *auth_data,
			      size_t auth_data_size, size_t *auth_data_len)
{
	uint8_t cose_key[FIDO2_COSE_KEY_MAX_SIZE];
	size_t cose_key_len;
	size_t total;
	size_t offset = 0;
	size_t written;
	int ret;

	ret = fido2_cbor_encode_cose_key(pub_key, pub_key_len, cose_key, sizeof(cose_key),
					 &cose_key_len);
	if (ret) {
		return ret;
	}

	total = FIDO2_AUTH_DATA_HEADER_SIZE + FIDO2_AAGUID_SIZE + 2 + cred_id_len + cose_key_len;

	if (total > auth_data_size) {
		return -ENOMEM;
	}

	memcpy(auth_data + offset, rp_id_hash, FIDO2_SHA256_SIZE);
	offset += FIDO2_SHA256_SIZE;

	auth_data[offset] = flags;
	offset += sizeof(uint8_t);

	sys_put_be32(sign_count, auth_data + offset);
	offset += sizeof(uint32_t);

	written = hex2bin(CONFIG_FIDO2_AAGUID, strlen(CONFIG_FIDO2_AAGUID), auth_data + offset,
			  FIDO2_AAGUID_SIZE);
	if (written != FIDO2_AAGUID_SIZE) {
		LOG_ERR("AAGUID parse failed, check CONFIG_FIDO2_AAGUID");
		return -EINVAL;
	}
	offset += FIDO2_AAGUID_SIZE;

	sys_put_be16(cred_id_len, auth_data + offset);
	offset += sizeof(uint16_t);

	memcpy(auth_data + offset, cred_id, cred_id_len);
	offset += cred_id_len;

	memcpy(auth_data + offset, cose_key, cose_key_len);
	offset += cose_key_len;

	*auth_data_len = offset;

	return 0;
}

static enum fido2_status
handle_make_credential(uint8_t *cbor_in, size_t cbor_in_len, uint8_t *cbor_out, size_t cbor_out_cap,
		       size_t *cbor_out_len, const struct fido2_transport *transport, uint32_t cid)
{
	bool supported_alg = false;
	uint8_t pub_key[FIDO2_P256_UNCOMPRESSED_KEY_SIZE];
	size_t pub_key_len;
	uint8_t auth_data[FIDO2_AUTH_DATA_MAX_SIZE];
	size_t auth_data_len;
	bool user_verified = false;
	uint8_t flags;
	struct fido2_attestation_result att_result;
	enum fido2_status status = FIDO2_OK;
	int ret;

	memset(&mc_credential, 0, sizeof(mc_credential));

	ret = fido2_cbor_decode_make_credential(cbor_in, cbor_in_len, &mc_params);
	if (ret) {
		LOG_WRN("MakeCredential CBOR decode failed: %d (len=%zu)", ret, cbor_in_len);
		return FIDO2_ERR_INVALID_CBOR;
	}

	/* UP must not be false. */
	if (mc_params.has_up_option && !mc_params.up) {
		LOG_WRN("Rejected MakeCredential: options.up cannot be false");
		return FIDO2_ERR_INVALID_OPTION;
	}

	/* pinUvAuthParam and uv are mutually exclusive, pinUvAuthParam takes precedence. */
	if (mc_params.has_pin_uv_auth_param) {
		mc_params.uv = false;
	}

	/* "Gracefully" reject unsupported feature */
	if (mc_params.has_enterprise_attestation) {
		LOG_WRN("Rejected MakeCredential: enterprise attestation not supported");
		return FIDO2_ERR_INVALID_PARAMETER;
	}

	/* Only ES256 (COSE algorithm -7) is supported */
	for (int i = 0; i < mc_params.num_algorithms; ++i) {
		if (mc_params.algorithms[i] == FIDO2_COSE_ES256) {
			supported_alg = true;
			break;
		}
	}
	if (!supported_alg) {
		LOG_WRN("No supported algorithm in pubKeyCredParams");
		return FIDO2_ERR_UNSUPPORTED_ALGORITHM;
	}

	ret = fido2_crypto_sha256((const uint8_t *)mc_params.rp_id, strlen(mc_params.rp_id),
				  mc_credential.rp_id_hash);
	if (ret) {
		LOG_ERR("RP ID hash failed: %d", ret);
		return FIDO2_ERR_OTHER;
	}

	if (mc_check_exclude_list()) {
		set_runtime_state(FIDO2_RUNTIME_STATE_WAITING_USER_PRESENCE);
		notify_wire(transport, cid, FIDO2_WIRE_STATUS_UP_NEEDED);

		ret = fido2_up_wait();
		if (ret) {
			if (ret == -ECANCELED) {
				return FIDO2_ERR_KEEPALIVE_CANCEL;
			}
			return FIDO2_ERR_USER_ACTION_TIMEOUT;
		}

		return FIDO2_ERR_CREDENTIAL_EXCLUDED;
	}

	ret = fido2_crypto_generate_keypair(&mc_credential.key_id);
	if (ret) {
		LOG_ERR("Keypair generation failed: %d", ret);
		return FIDO2_ERR_OTHER;
	}

	ret = fido2_crypto_export_pubkey(mc_credential.key_id, pub_key, sizeof(pub_key),
					 &pub_key_len);
	if (ret) {
		LOG_ERR("Public key export failed: %d", ret);
		status = FIDO2_ERR_OTHER;
		goto cleanup;
	}

	if (pub_key_len != FIDO2_P256_UNCOMPRESSED_KEY_SIZE ||
	    pub_key[0] != FIDO2_EC_POINT_UNCOMPRESSED) {
		LOG_ERR("Unsupported public key format len=%zu first=0x%02x", pub_key_len,
			pub_key_len > 0 ? pub_key[0] : 0);
		status = FIDO2_ERR_OTHER;
		goto cleanup;
	}

	ret = mc_populate_credential();
	if (ret) {
		status = FIDO2_ERR_OTHER;
		goto cleanup;
	}

	set_runtime_state(FIDO2_RUNTIME_STATE_WAITING_USER_PRESENCE);
	notify_wire(transport, cid, FIDO2_WIRE_STATUS_UP_NEEDED);

	ret = fido2_up_wait();
	if (ret) {
		status = (ret == -ECANCELED) ? FIDO2_ERR_KEEPALIVE_CANCEL
					     : FIDO2_ERR_USER_ACTION_TIMEOUT;
		goto cleanup;
	}

	set_runtime_state(FIDO2_RUNTIME_STATE_PROCESSING);
	notify_wire(transport, cid, FIDO2_WIRE_STATUS_PROCESSING);

	flags = AUTH_DATA_FLAG_AT | AUTH_DATA_FLAG_UP;

	if (user_verified) {
		flags |= AUTH_DATA_FLAG_UV;
	}

	ret = mc_build_auth_data(mc_credential.rp_id_hash, mc_credential.id, mc_credential.id_len,
				 pub_key, pub_key_len, 0, flags, auth_data, sizeof(auth_data),
				 &auth_data_len);
	if (ret) {
		LOG_ERR("authData build failed: %d", ret);
		status = FIDO2_ERR_OTHER;
		goto cleanup;
	}

	ret = fido2_attestation_sign(auth_data, auth_data_len, mc_params.client_data_hash,
				     mc_credential.key_id, &att_result);
	if (ret) {
		status = FIDO2_ERR_OTHER;
		goto cleanup;
	}

	ret = fido2_cbor_encode_make_credential_resp(auth_data, auth_data_len, &att_result,
						     cbor_out, cbor_out_cap, cbor_out_len);
	if (ret) {
		LOG_ERR("MakeCredential response encode failed: %d", ret);
		status = FIDO2_ERR_OTHER;
		goto cleanup;
	}

	if (mc_credential.discoverable) {
		ret = mc_store_discoverable();
		if (ret) {
			status = (ret == -ENOSPC) ? FIDO2_ERR_KEY_STORE_FULL : FIDO2_ERR_OTHER;
			goto cleanup;
		}
	}

cleanup:
	if (status != FIDO2_OK) {
		fido2_crypto_destroy_key(mc_credential.key_id);
		return status;
	}

	LOG_INF("MakeCredential succeeded for RP: %s", mc_params.rp_id);

	return FIDO2_OK;
}

static int ga_check_allow_list(uint8_t rp_id_hash[FIDO2_SHA256_SIZE], size_t *found_count)
{
	for (int i = 0; i < ga_params.num_allow; ++i) {
		struct fido2_credential cred;
		int ret;

		if (ga_params.allow_id_lens[i] == FIDO2_DISCOVERABLE_CRED_ID_SIZE) {
			ret = fido2_storage_load(ga_params.allow_ids[i], ga_params.allow_id_lens[i],
						 &cred);
			if (!ret && !memcmp(cred.rp_id_hash, rp_id_hash, FIDO2_SHA256_SIZE) &&
			    *found_count < CONFIG_FIDO2_MAX_CREDENTIALS) {
				memcpy(&ga_credentials[(*found_count)++], &cred, sizeof(cred));
			}
		} else if (ga_params.allow_id_lens[i] == FIDO2_NON_DISCOVERABLE_CRED_ID_SIZE) {
			uint32_t key_id;

			ret = fido2_crypto_unwrap_credential(ga_params.allow_ids[i], rp_id_hash,
							     &key_id);
			if (!ret && *found_count < CONFIG_FIDO2_MAX_CREDENTIALS) {
				memset(&cred, 0, sizeof(cred));
				memcpy(cred.rp_id_hash, rp_id_hash, FIDO2_SHA256_SIZE);
				memcpy(cred.id, ga_params.allow_ids[i],
				       FIDO2_NON_DISCOVERABLE_CRED_ID_SIZE);
				cred.id_len = FIDO2_NON_DISCOVERABLE_CRED_ID_SIZE;
				cred.key_id = key_id;
				cred.algorithm = FIDO2_COSE_ES256;
				cred.discoverable = false;
				memcpy(&ga_credentials[(*found_count)++], &cred, sizeof(cred));
			}
		}
	}

	return 0;
}

static int build_assertion_response(const struct fido2_credential *cred,
				    const uint8_t rp_id_hash[FIDO2_SHA256_SIZE],
				    const uint8_t client_data_hash[FIDO2_SHA256_SIZE],
				    uint8_t flags, uint16_t num_credentials, uint8_t *cbor_out,
				    size_t cbor_out_cap, size_t *cbor_out_len)
{
	uint8_t auth_data[FIDO2_AUTH_DATA_HEADER_SIZE];
	size_t auth_data_len;
	uint32_t sign_count;
	uint8_t sig_input_hash[FIDO2_SHA256_SIZE];
	uint8_t sig[FIDO2_ECDSA_SIG_MAX_SIZE];
	size_t sig_len;
	int ret;

	if (cred->discoverable) {
		ret = fido2_storage_sign_count_increment(cred->id, cred->id_len, &sign_count);
		if (ret) {
			LOG_ERR("Sign count increment failed: %d", ret);
			return ret;
		}
	} else {
		sign_count = 0;
	}

	memcpy(auth_data, rp_id_hash, FIDO2_SHA256_SIZE);
	auth_data[FIDO2_SHA256_SIZE] = flags;
	sys_put_be32(sign_count, auth_data + FIDO2_SHA256_SIZE + 1);
	auth_data_len = FIDO2_AUTH_DATA_HEADER_SIZE;

	ret = fido2_crypto_hash_authdata(auth_data, auth_data_len, client_data_hash,
					 sig_input_hash);
	if (ret) {
		return ret;
	}

	ret = fido2_crypto_sign(cred->key_id, sig_input_hash, sig, sizeof(sig), &sig_len);
	if (ret) {
		LOG_ERR("Assertion sign failed key_id=0x%08x: %d", (unsigned int)cred->key_id, ret);
		return ret;
	}

	ret = fido2_cbor_encode_get_assertion_resp(
		cred->id, cred->id_len, auth_data, auth_data_len, sig, sig_len, cred->user_id,
		cred->user_id_len, num_credentials, cbor_out, cbor_out_cap, cbor_out_len);
	if (ret) {
		return ret;
	}

	return 0;
}

static enum fido2_status handle_get_assertion(uint8_t *cbor_in, size_t cbor_in_len,
					      uint8_t *cbor_out, size_t cbor_out_cap,
					      size_t *cbor_out_len,
					      const struct fido2_transport *transport, uint32_t cid)
{
	uint8_t rp_id_hash[FIDO2_SHA256_SIZE];
	size_t found_creds_count = 0;
	uint8_t flags = 0;
	uint16_t num_credentials;
	int ret;

	ret = fido2_cbor_decode_get_assertion(cbor_in, cbor_in_len, &ga_params);
	if (ret) {
		LOG_WRN("GetAssertion CBOR decode failed: %d (len=%zu)", ret, cbor_in_len);
		return FIDO2_ERR_INVALID_CBOR;
	}

	/* Accept absent up as true, per spec */
	if (!ga_params.has_up_option) {
		ga_params.up = true;
	}
	if (!ga_params.has_uv_option) {
		ga_params.uv = false;
	}
	/* pinUvAuthParam and uv are mutually exclusive, pinUvAuthParam takes precedence. */
	if (ga_params.has_pin_uv_auth_param) {
		ga_params.uv = false;
	}
	if (ga_params.uv) {
		LOG_WRN("Rejected GetAssertion: UV not supported");
		return FIDO2_ERR_OPERATION_DENIED;
	}
	if (ga_params.has_enterprise_attestation) {
		LOG_WRN("Rejected GetAssertion: enterprise attestation not supported");
		return FIDO2_ERR_INVALID_PARAMETER;
	}

	ret = fido2_crypto_sha256(ga_params.rp_id, strlen(ga_params.rp_id), rp_id_hash);
	if (ret) {
		LOG_ERR("RP ID hash failed: %d", ret);
		return FIDO2_ERR_OTHER;
	}

	if (ga_params.num_allow > 0) {
		ret = ga_check_allow_list(rp_id_hash, &found_creds_count);
		if (ret) {
			return FIDO2_ERR_OTHER;
		}
	} else {
		ret = fido2_storage_find_by_rp(rp_id_hash, ga_credentials,
					       CONFIG_FIDO2_MAX_CREDENTIALS, &found_creds_count);
		if (ret) {
			return FIDO2_ERR_OTHER;
		}
	}

	if (found_creds_count == 0) {
		return FIDO2_ERR_NO_CREDENTIALS;
	}

	if (ga_params.up) {
		set_runtime_state(FIDO2_RUNTIME_STATE_WAITING_USER_PRESENCE);
		notify_wire(transport, cid, FIDO2_WIRE_STATUS_UP_NEEDED);

		ret = fido2_up_wait();
		if (ret) {
			if (ret == -ECANCELED) {
				return FIDO2_ERR_KEEPALIVE_CANCEL;
			}
			return FIDO2_ERR_OPERATION_DENIED;
		}

		set_runtime_state(FIDO2_RUNTIME_STATE_PROCESSING);
		notify_wire(transport, cid, FIDO2_WIRE_STATUS_PROCESSING);

		flags = AUTH_DATA_FLAG_UP;
	}

	/* Exclude numberOfCredentials when allowList is pesent or found_creds_count is exactly 1*/
	num_credentials =
		(ga_params.num_allow == 0 && found_creds_count > 1) ? found_creds_count : 0;

	ret = build_assertion_response(&ga_credentials[0], rp_id_hash, ga_params.client_data_hash,
				       flags, num_credentials, cbor_out, cbor_out_cap,
				       cbor_out_len);
	if (ret) {
		return FIDO2_ERR_OTHER;
	}

	/* Store sates for getNextAssertion */
	if (ga_params.num_allow == 0 && found_creds_count > 1) {
		ga_next_count = found_creds_count;
		ga_next_index = 1;
		ga_next_flags = flags;
		memcpy(ga_next_rp_id_hash, rp_id_hash, FIDO2_SHA256_SIZE);
		memcpy(ga_next_client_data_hash, ga_params.client_data_hash, FIDO2_SHA256_SIZE);
		ga_next_pending = true;
		ga_next_deadline = sys_timepoint_calc(K_SECONDS(30));
	} else {
		ga_next_pending = false;
	}

	LOG_INF("GetAssertion succeeded for RP: %s (%zu credentials)", ga_params.rp_id,
		found_creds_count);

	return FIDO2_OK;
}

static enum fido2_status handle_get_info(uint8_t *cbor_out, size_t cbor_out_cap,
					 size_t *cbor_out_len)
{
	struct fido2_device_info info = {0};
	int ret;

	info.versions[0] = "FIDO_2_0";
	info.versions[1] = "FIDO_2_1";
	info.num_versions = 2;
	info.num_pin_uv_auth_protocols = 0;
	info.max_credential_count = CONFIG_FIDO2_MAX_CREDENTIALS;
	info.max_msg_size = CONFIG_FIDO2_CBOR_MAX_SIZE;
	info.max_credential_id_length = FIDO2_CREDENTIAL_ID_MAX_SIZE;
	info.transports = 0;

	info.firmware_version = 0x00010000;

	if (IS_ENABLED(CONFIG_FIDO2_TRANSPORT_USB_HID)) {
		info.transports |= FIDO2_TRANSPORT_USB;
	}

	info.options.rk = !IS_ENABLED(CONFIG_FIDO2_STORAGE_NONE);
	info.options.up = true;
	info.options.plat = false;
	/* These will depend on config */
	info.options.uv = false;
	/* Allow non-UV non-discoverable credential creation */
	info.options.make_cred_uv_not_rqd = !IS_ENABLED(CONFIG_FIDO2_ALWAYS_UV);
	/* Force UV even when RP sets userVerification=discouraged */
	info.options.always_uv = IS_ENABLED(CONFIG_FIDO2_ALWAYS_UV);
	info.options.no_mc_ga_permissions_with_client_pin = false;

#if defined(CONFIG_FIDO2_CREDENTIAL_MANAGEMENT)
	info.options.cred_mgmt = true;
#endif
#if defined(CONFIG_FIDO2_EXT_CRED_PROTECT)
	info.extensions[info.num_extensions++] = "credProtect";
#endif
#if defined(CONFIG_FIDO2_EXT_HMAC_SECRET)
	info.extensions[info.num_extensions++] = "hmac-secret";
#endif
#if defined(CONFIG_FIDO2_EXT_LARGE_BLOB_KEY)
	info.extensions[info.num_extensions++] = "largeBlobKey";
#endif
#if defined(CONFIG_FIDO2_EXT_CRED_BLOB)
	info.extensions[info.num_extensions++] = "credBlob";
#endif
#if defined(CONFIG_FIDO2_EXT_THIRD_PARTY_PAYMENT)
	info.extensions[info.num_extensions++] = "thirdPartyPayment";
#endif

	/* Convert HEX AAGUID to binary and load it in info */
	size_t written = hex2bin(CONFIG_FIDO2_AAGUID, strlen(CONFIG_FIDO2_AAGUID), info.aaguid,
				 FIDO2_AAGUID_SIZE);
	if (written != FIDO2_AAGUID_SIZE) {
		LOG_ERR("Invalid FIDO2_AAGUID");
		return FIDO2_ERR_OTHER;
	}

	ret = fido2_cbor_encode_get_info(&info, cbor_out, cbor_out_cap, cbor_out_len);
	if (ret) {
		return FIDO2_ERR_OTHER;
	}

	return FIDO2_OK;
}

static enum fido2_status handle_get_next_assertion(uint8_t *cbor_out, size_t cbor_out_cap,
						   size_t *cbor_out_len)
{
	int ret;

	if (!ga_next_pending || (ga_next_index >= ga_next_count)) {
		ga_next_pending = false;
		return FIDO2_ERR_NOT_ALLOWED;
	}

	if (sys_timepoint_expired(ga_next_deadline)) {
		ga_next_pending = false;
		return FIDO2_ERR_NOT_ALLOWED;
	}

	ret = build_assertion_response(&ga_credentials[ga_next_index], ga_next_rp_id_hash,
				       ga_next_client_data_hash, ga_next_flags, 0, cbor_out,
				       cbor_out_cap, cbor_out_len);
	if (ret) {
		return FIDO2_ERR_OTHER;
	}

	++ga_next_index;
	ga_next_deadline = sys_timepoint_calc(K_SECONDS(30));

	return FIDO2_OK;
}

static enum fido2_status handle_selection(const struct fido2_transport *transport, uint32_t cid)
{
	int ret;

	set_runtime_state(FIDO2_RUNTIME_STATE_WAITING_USER_PRESENCE);
	notify_wire(transport, cid, FIDO2_WIRE_STATUS_UP_NEEDED);

	ret = fido2_up_wait();

	if (ret) {
		if (ret == -ECANCELED) {
			return FIDO2_ERR_KEEPALIVE_CANCEL;
		}

		return FIDO2_ERR_USER_ACTION_TIMEOUT;
	}

	return FIDO2_OK;
}

static enum fido2_status process_command(uint8_t cmd, uint8_t *cbor_in, size_t cbor_in_len,
					 uint8_t *cbor_out, size_t cbor_out_cap,
					 size_t *cbor_out_len,
					 const struct fido2_transport *transport, uint32_t cid)
{
	switch (cmd) {
	case FIDO2_CMD_MAKE_CREDENTIAL:
		return handle_make_credential(cbor_in, cbor_in_len, cbor_out, cbor_out_cap,
					      cbor_out_len, transport, cid);
	case FIDO2_CMD_GET_ASSERTION:
		return handle_get_assertion(cbor_in, cbor_in_len, cbor_out, cbor_out_cap,
					    cbor_out_len, transport, cid);
	case FIDO2_CMD_GET_INFO:
		return handle_get_info(cbor_out, cbor_out_cap, cbor_out_len);
	case FIDO2_CMD_CLIENT_PIN:
		/* TODO: clientPin */
		*cbor_out_len = 0;
		return FIDO2_ERR_INVALID_COMMAND;
	case FIDO2_CMD_RESET:
		/* TODO: Reset */
		*cbor_out_len = 0;
		return FIDO2_OK;
	case FIDO2_CMD_GET_NEXT_ASSERTION:
		return handle_get_next_assertion(cbor_out, cbor_out_cap, cbor_out_len);
	case FIDO2_CMD_CREDENTIAL_MGMT:
		/* TODO: credMgmt */
		*cbor_out_len = 0;
		return FIDO2_ERR_INVALID_COMMAND;
	case FIDO2_CMD_SELECTION:
		*cbor_out_len = 0;
		return handle_selection(transport, cid);
	default:
		*cbor_out_len = 0;
		return FIDO2_ERR_INVALID_COMMAND;
	}
}

static void transport_recv_cb(const struct fido2_transport *transport, uint32_t cid,
			      const uint8_t *buf, size_t len)
{
	int ret;

	if (len > sizeof(rx_enqueue_msg.data)) {
		LOG_WRN("Message too large, dropping");
		return;
	}

	k_mutex_lock(&rx_enqueue_mutex, K_FOREVER);

	rx_enqueue_msg.transport = transport;
	rx_enqueue_msg.cid = cid;
	rx_enqueue_msg.len = len;
	memcpy(rx_enqueue_msg.data, buf, len);

	ret = k_msgq_put(&fido2_msgq, &rx_enqueue_msg, K_NO_WAIT);

	k_mutex_unlock(&rx_enqueue_mutex);

	if (ret) {
		LOG_WRN("Message queue full, dropping cid=0x%08x", cid);
	}
}

static inline void transport_cancel_cb(void)
{
	fido2_up_cancel();
}

static void fido2_thread_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (atomic_get(&fido2_running)) {
		if (k_msgq_get(&fido2_msgq, &rx_dequeue_msg, K_MSEC(100)) != 0) {
			continue;
		}

		if (rx_dequeue_msg.len < 1) {
			continue;
		}

		uint8_t cmd = rx_dequeue_msg.data[0];
		size_t cbor_out_len = 0;
		enum fido2_status status;
		int ret;

		status =
			process_command(cmd, rx_dequeue_msg.data + 1, rx_dequeue_msg.len - 1,
					ctap_tx_frame + 1, sizeof(ctap_tx_frame) - 1, &cbor_out_len,
					rx_dequeue_msg.transport, rx_dequeue_msg.cid);

		set_runtime_state(FIDO2_RUNTIME_STATE_IDLE);
		notify_wire(rx_dequeue_msg.transport, rx_dequeue_msg.cid, FIDO2_WIRE_STATUS_DONE);

		ctap_tx_frame[0] = (uint8_t)status;
		if (rx_dequeue_msg.transport && rx_dequeue_msg.transport->api) {
			ret = rx_dequeue_msg.transport->api->send(rx_dequeue_msg.cid, ctap_tx_frame,
								  cbor_out_len + 1);
		} else {
			LOG_ERR("No send path for cid=0x%08x", rx_dequeue_msg.cid);
			ret = -ENODEV;
		}

		if (ret) {
			LOG_WRN("Response send failed: %d", ret);
		} else {
			LOG_INF("CTAP cmd=0x%02x status=0x%02x wire_len=%zu cid=0x%08x", cmd,
				ctap_tx_frame[0], cbor_out_len + 1, rx_dequeue_msg.cid);
		}
	}
}

int fido2_init(void)
{
	int ret;

	reset_timeout = sys_timepoint_calc(K_SECONDS(10)); /* Reset allowed for 10s post-boot */

	ret = fido2_crypto_init();
	if (ret) {
		LOG_ERR("Crypto init failed: %d", ret);
		return ret;
	}

	ret = fido2_storage_init();
	if (ret) {
		LOG_ERR("Storage init failed: %d", ret);
		return ret;
	}

	ret = fido2_transport_init_all(transport_recv_cb, transport_cancel_cb);
	if (ret) {
		LOG_ERR("Transport init failed: %d", ret);
		return ret;
	}

	LOG_INF("FIDO2 subsystem initialized");

	return 0;
}

int fido2_start(void)
{
	if (atomic_get(&fido2_running)) {
		LOG_ERR("FIDO2 authenticator already started");
		return -EALREADY;
	}

	atomic_set(&fido2_running, 1);

	k_thread_create(&fido2_thread, fido2_stack, K_THREAD_STACK_SIZEOF(fido2_stack),
			fido2_thread_fn, NULL, NULL, NULL, CONFIG_FIDO2_THREAD_PRIORITY, 0,
			K_NO_WAIT);
	k_thread_name_set(&fido2_thread, "fido2");

	set_runtime_state(FIDO2_RUNTIME_STATE_IDLE);

	LOG_INF("FIDO2 authenticator started");
	return 0;
}

int fido2_stop(void)
{
	int ret;

	if (!atomic_get(&fido2_running)) {
		LOG_ERR("FIDO2 authenticator already stopped");
		return -EALREADY;
	}

	atomic_set(&fido2_running, 0);

	ret = k_thread_join(&fido2_thread, K_SECONDS(5));
	if (ret) {
		LOG_ERR("FIDO2 thread stop failed: %d", ret);
		return ret;
	}

	fido2_transport_shutdown_all();

	set_runtime_state(FIDO2_RUNTIME_STATE_STOPPED);

	LOG_INF("FIDO2 authenticator stopped");
	return 0;
}

int fido2_set_state_callback(fido2_state_callback_t cb, void *user_data)
{
	enum fido2_runtime_state state;
	k_spinlock_key_t key;

	key = k_spin_lock(&state_lock);
	runtime_state_cb = cb;
	runtime_state_cb_user_data = user_data;
	state = runtime_state;
	k_spin_unlock(&state_lock, key);

	if (cb != NULL) {
		cb(state, user_data);
	}

	return 0;
}

enum fido2_runtime_state fido2_get_state(void)
{
	enum fido2_runtime_state state;
	k_spinlock_key_t key;

	key = k_spin_lock(&state_lock);
	state = runtime_state;
	k_spin_unlock(&state_lock, key);

	return state;
}

int fido2_reset(void)
{
	if (sys_timepoint_expired(reset_timeout)) {
		return -EACCES;
	}

	/* TODO: Reset */
	return 0;
}
