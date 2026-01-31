/*
 * Copyright (c) 2026 Martin Schröder <info@swedishembedded.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Weak wrappers for EDHOC/OSCORE functions for testing
 *
 * These weak symbols allow tests to override EDHOC and OSCORE operations
 * without requiring full uoscore-uedhoc library integration.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_coap, CONFIG_COAP_LOG_LEVEL);

#include <errno.h>
#include <stdint.h>
#include <string.h>

/* These are weak symbols that can be overridden in tests */

/**
 * @brief Weak wrapper for EDHOC message_3 processing
 *
 * In production, this should call the real msg3_process() from uoscore-uedhoc.
 * In tests, this can be overridden to inject test behavior.
 *
 * @param edhoc_msg3 EDHOC message_3 buffer (CBOR bstr encoding)
 * @param edhoc_msg3_len Length of EDHOC message_3
 * @param resp_ctx EDHOC responder context
 * @param runtime_ctx Runtime context
 * @return 0 on success, negative errno on error
 */
__weak int coap_edhoc_msg3_process_wrapper(const uint8_t *edhoc_msg3,
					   size_t edhoc_msg3_len,
					   void *resp_ctx,
					   void *runtime_ctx)
{
	ARG_UNUSED(edhoc_msg3);
	ARG_UNUSED(edhoc_msg3_len);
	ARG_UNUSED(resp_ctx);
	ARG_UNUSED(runtime_ctx);

	/* Default implementation: not supported without uoscore-uedhoc */
	LOG_ERR("EDHOC msg3_process not available (override in tests or link uoscore-uedhoc)");
	return -ENOTSUP;
}

/**
 * @brief Weak wrapper for EDHOC exporter (derive OSCORE master secret/salt)
 *
 * In production, this should call edhoc_exporter() from uoscore-uedhoc.
 * In tests, this can be overridden to inject test behavior.
 *
 * @param resp_ctx EDHOC responder context
 * @param label EDHOC exporter label (0 for master secret, 1 for master salt)
 * @param output Output buffer
 * @param output_len Length of output buffer
 * @return 0 on success, negative errno on error
 */
__weak int coap_edhoc_exporter_wrapper(void *resp_ctx,
					uint8_t label,
					uint8_t *output,
					size_t output_len)
{
	ARG_UNUSED(resp_ctx);
	ARG_UNUSED(label);
	ARG_UNUSED(output);
	ARG_UNUSED(output_len);

	/* Default implementation: not supported without uoscore-uedhoc */
	LOG_ERR("EDHOC exporter not available (override in tests or link uoscore-uedhoc)");
	return -ENOTSUP;
}

/**
 * @brief Weak wrapper for OSCORE context initialization
 *
 * In production, this should call oscore_context_init() from uoscore-uedhoc.
 * In tests, this can be overridden to inject test behavior.
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
					     size_t recipient_id_len)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(master_secret);
	ARG_UNUSED(master_secret_len);
	ARG_UNUSED(master_salt);
	ARG_UNUSED(master_salt_len);
	ARG_UNUSED(sender_id);
	ARG_UNUSED(sender_id_len);
	ARG_UNUSED(recipient_id);
	ARG_UNUSED(recipient_id_len);

	/* Default implementation: not supported without uoscore-uedhoc */
	LOG_ERR("OSCORE context_init not available (override in tests or link uoscore-uedhoc)");
	return -ENOTSUP;
}
