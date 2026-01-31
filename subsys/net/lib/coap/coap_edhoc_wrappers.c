/*
 * Copyright (c) 2026 Martin Schröder <info@swedishembedded.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief EDHOC/OSCORE integration wrappers
 *
 * Provides integration with uoscore-uedhoc library when CONFIG_UEDHOC=y.
 * Falls back to weak stubs for testing when CONFIG_UEDHOC=n.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_coap, CONFIG_COAP_LOG_LEVEL);

#include <errno.h>
#include <stdint.h>
#include <string.h>

#if defined(CONFIG_UEDHOC)
#include <edhoc.h>
#include <edhoc_internal.h>
#include <oscore.h>
#include <common/byte_array.h>
#include <common/oscore_edhoc_error.h>
#endif

/* These are weak symbols that can be overridden in tests */

/**
 * @brief Wrapper for EDHOC message_3 processing
 *
 * Processes EDHOC message_3 and derives PRK_out per RFC 9528 Section 5.4.3.
 * Extracts C_I from the EDHOC runtime context for RFC 9528 Table 14 ID mapping.
 * When CONFIG_UEDHOC=y, uses real uoscore-uedhoc implementation.
 * When CONFIG_UEDHOC=n, provides weak stub for testing.
 *
 * @param edhoc_msg3 EDHOC message_3 buffer (CBOR bstr encoding)
 * @param edhoc_msg3_len Length of EDHOC message_3
 * @param resp_ctx EDHOC responder context
 * @param runtime_ctx Runtime context
 * @param cred_i_array Array of trusted initiator credentials
 * @param prk_out Output buffer for PRK_out (derived shared secret)
 * @param prk_out_len Length of prk_out buffer (input: buffer size, output: actual length)
 * @param initiator_pk Output buffer for initiator public key
 * @param initiator_pk_len Length of initiator_pk buffer (input: buffer size, output: actual length)
 * @param c_i Output buffer for C_I (connection identifier for initiator)
 * @param c_i_len Length of c_i buffer (input: buffer size, output: actual length)
 * @return 0 on success, negative errno on error
 */
__weak int coap_edhoc_msg3_process_wrapper(const uint8_t *edhoc_msg3,
					   size_t edhoc_msg3_len,
					   void *resp_ctx,
					   void *runtime_ctx,
					   void *cred_i_array,
					   uint8_t *prk_out,
					   size_t *prk_out_len,
					   uint8_t *initiator_pk,
					   size_t *initiator_pk_len,
					   uint8_t *c_i,
					   size_t *c_i_len)
{
#if defined(CONFIG_UEDHOC)
	struct edhoc_responder_context *c = (struct edhoc_responder_context *)resp_ctx;
	struct runtime_context *rc = (struct runtime_context *)runtime_ctx;
	struct cred_array *creds = (struct cred_array *)cred_i_array;
	struct byte_array prk_out_ba = { .ptr = prk_out, .len = *prk_out_len };
	struct byte_array initiator_pk_ba = { .ptr = initiator_pk, .len = *initiator_pk_len };
	enum err result;

	if (c == NULL || rc == NULL || creds == NULL) {
		return -EINVAL;
	}

	/* RFC 9528 Section 5.4.3: Process message_3 and derive PRK_out */
	result = msg3_process(c, rc, creds, &prk_out_ba, &initiator_pk_ba);
	if (result != ok) {
		LOG_ERR("msg3_process failed: %d", result);
		return -EACCES;
	}

	*prk_out_len = prk_out_ba.len;
	*initiator_pk_len = initiator_pk_ba.len;

	/* RFC 9528 Table 14: Extract C_I from runtime context for OSCORE Sender ID mapping
	 * After msg3_process, C_I is available in rc->c_i
	 */
	if (rc->c_i.len > 0 && rc->c_i.len <= *c_i_len) {
		memcpy(c_i, rc->c_i.ptr, rc->c_i.len);
		*c_i_len = rc->c_i.len;
	} else {
		LOG_ERR("C_I not available or buffer too small (need %zu, have %zu)",
			rc->c_i.len, *c_i_len);
		return -EINVAL;
	}

	return 0;
#else
	ARG_UNUSED(edhoc_msg3);
	ARG_UNUSED(edhoc_msg3_len);
	ARG_UNUSED(resp_ctx);
	ARG_UNUSED(runtime_ctx);
	ARG_UNUSED(cred_i_array);
	ARG_UNUSED(prk_out);
	ARG_UNUSED(prk_out_len);
	ARG_UNUSED(initiator_pk);
	ARG_UNUSED(initiator_pk_len);
	ARG_UNUSED(c_i);
	ARG_UNUSED(c_i_len);

	/* Default implementation: not supported without uoscore-uedhoc */
	LOG_ERR("EDHOC msg3_process not available (override in tests or link uoscore-uedhoc)");
	return -ENOTSUP;
#endif
}

/**
 * @brief Wrapper for EDHOC exporter (derive OSCORE master secret/salt)
 *
 * Derives OSCORE keying material from EDHOC PRK_out per RFC 9528 Appendix A.1.
 * Uses EDHOC_Exporter with labels 0 (master secret) and 1 (master salt).
 * When CONFIG_UEDHOC=y, uses real uoscore-uedhoc implementation.
 * When CONFIG_UEDHOC=n, provides weak stub for testing.
 *
 * @param prk_out PRK_out from EDHOC (derived shared secret)
 * @param prk_out_len Length of prk_out
 * @param app_hash_alg Application hash algorithm from EDHOC suite
 * @param label EDHOC exporter label (0 for master secret, 1 for master salt)
 * @param output Output buffer
 * @param output_len Length of output buffer (input: buffer size, output: actual length)
 * @return 0 on success, negative errno on error
 */
__weak int coap_edhoc_exporter_wrapper(const uint8_t *prk_out,
					size_t prk_out_len,
					int app_hash_alg,
					uint8_t label,
					uint8_t *output,
					size_t *output_len)
{
#if defined(CONFIG_UEDHOC)
	struct byte_array prk_out_ba = { .ptr = (uint8_t *)prk_out, .len = prk_out_len };
	struct byte_array prk_exporter_ba;
	uint8_t prk_exporter_buf[64]; /* Max hash size */
	struct byte_array output_ba = { .ptr = output, .len = *output_len };
	enum err result;
	enum hash_alg hash_alg = (enum hash_alg)app_hash_alg;
	enum export_label export_label = (enum export_label)label;

	if (prk_out == NULL || output == NULL || output_len == NULL) {
		return -EINVAL;
	}

	/* RFC 9528 Appendix A.1: Derive PRK_exporter from PRK_out */
	prk_exporter_ba.ptr = prk_exporter_buf;
	prk_exporter_ba.len = sizeof(prk_exporter_buf);

	result = prk_out2exporter(hash_alg, &prk_out_ba, &prk_exporter_ba);
	if (result != ok) {
		LOG_ERR("prk_out2exporter failed: %d", result);
		/* Zeroize intermediate secret */
		memset(prk_exporter_buf, 0, sizeof(prk_exporter_buf));
		return -EACCES;
	}

	/* RFC 9528 Appendix A.1: Derive OSCORE master secret or salt */
	result = edhoc_exporter(hash_alg, export_label, &prk_exporter_ba, &output_ba);

	/* Zeroize intermediate secret */
	memset(prk_exporter_buf, 0, sizeof(prk_exporter_buf));

	if (result != ok) {
		LOG_ERR("edhoc_exporter failed: %d", result);
		return -EACCES;
	}

	*output_len = output_ba.len;

	return 0;
#else
	ARG_UNUSED(prk_out);
	ARG_UNUSED(prk_out_len);
	ARG_UNUSED(app_hash_alg);
	ARG_UNUSED(label);
	ARG_UNUSED(output);
	ARG_UNUSED(output_len);

	/* Default implementation: not supported without uoscore-uedhoc */
	LOG_ERR("EDHOC exporter not available (override in tests or link uoscore-uedhoc)");
	return -ENOTSUP;
#endif
}

/**
 * @brief Wrapper for OSCORE context initialization
 *
 * Initializes OSCORE security context with derived keying material.
 * Per RFC 9528 Appendix A.1, uses EDHOC-selected algorithms.
 * When CONFIG_UEDHOC=y, uses real uoscore-uedhoc implementation.
 * When CONFIG_UEDHOC=n, provides weak stub for testing.
 *
 * @param ctx OSCORE context to initialize
 * @param master_secret Master secret
 * @param master_secret_len Length of master secret
 * @param master_salt Master salt
 * @param master_salt_len Length of master salt
 * @param sender_id Sender ID
 * @param sender_id_len Length of sender ID
 * @param recipient_id Recipient ID
 * @param recipient_id_len Length of recipient ID
 * @param aead_alg AEAD algorithm (from EDHOC suite)
 * @param hkdf_alg HKDF algorithm (from EDHOC suite)
 * @return 0 on success, negative errno on error
 */
__weak int coap_oscore_context_init_wrapper(void *ctx,
					     const uint8_t *master_secret,
					     size_t master_secret_len,
					     const uint8_t *master_salt,
					     size_t master_salt_len,
					     const uint8_t *sender_id,
					     size_t sender_id_len,
					     const uint8_t *recipient_id,
					     size_t recipient_id_len,
					     int aead_alg,
					     int hkdf_alg)
{
#if defined(CONFIG_UEDHOC)
	struct context *c = (struct context *)ctx;
	struct oscore_init_params params;
	struct byte_array master_secret_ba = {
		.ptr = (uint8_t *)master_secret,
		.len = master_secret_len
	};
	struct byte_array master_salt_ba = {
		.ptr = (uint8_t *)master_salt,
		.len = master_salt_len
	};
	struct byte_array sender_id_ba = {
		.ptr = (uint8_t *)sender_id,
		.len = sender_id_len
	};
	struct byte_array recipient_id_ba = {
		.ptr = (uint8_t *)recipient_id,
		.len = recipient_id_len
	};
	enum err result;

	if (ctx == NULL || master_secret == NULL || sender_id == NULL || recipient_id == NULL) {
		return -EINVAL;
	}

	/* RFC 9528 Appendix A.1: Initialize OSCORE context with EDHOC-derived keying material */
	memset(&params, 0, sizeof(params));
	params.master_secret = master_secret_ba;
	params.master_salt = master_salt_ba;
	params.sender_id = sender_id_ba;
	params.recipient_id = recipient_id_ba;
	params.aead_alg = (enum AEAD_algorithm)aead_alg;
	params.hkdf = (enum hkdf)hkdf_alg;
	params.fresh_master_secret_salt = true; /* EDHOC-derived keys are fresh */

	result = oscore_context_init(&params, c);
	if (result != ok) {
		LOG_ERR("oscore_context_init failed: %d", result);
		return -EACCES;
	}

	return 0;
#else
	ARG_UNUSED(ctx);
	ARG_UNUSED(master_secret);
	ARG_UNUSED(master_secret_len);
	ARG_UNUSED(master_salt);
	ARG_UNUSED(master_salt_len);
	ARG_UNUSED(sender_id);
	ARG_UNUSED(sender_id_len);
	ARG_UNUSED(recipient_id);
	ARG_UNUSED(recipient_id_len);
	ARG_UNUSED(aead_alg);
	ARG_UNUSED(hkdf_alg);

	/* Default implementation: not supported without uoscore-uedhoc */
	LOG_ERR("OSCORE context_init not available (override in tests or link uoscore-uedhoc)");
	return -ENOTSUP;
#endif
}
