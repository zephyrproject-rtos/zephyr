/*
 * Copyright (c) 2026 Siemens AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/util.h>

#include "common/byte_array.h"
#include "common/oscore_edhoc_error.h"
#include "oscore/nvm.h"
#include "oscore/oscore_coap_defines.h"

LOG_MODULE_REGISTER(uoscore_nvm, CONFIG_LOG_DEFAULT_LEVEL);

#define OSCORE_SSN_KEY_PREFIX     "oscore_ssn/"
#define OSCORE_SSN_KEY_PREFIX_LEN (sizeof(OSCORE_SSN_KEY_PREFIX) - 1U)

/*
 * Worst-case key length (excluding NUL) for maximum-size identifiers: prefix +
 * two length-prefixed IDs + trailing ID context, all hex encoded. The literal
 * 11 is strlen(OSCORE_SSN_KEY_PREFIX). Warn at build time if the settings name
 * limit cannot hold it, in which case such contexts fail with buffer_to_small.
 */
#define OSCORE_SSN_KEY_WORST_LEN                                                                   \
	(11U + (2U + (2U * MAX_KID_LEN)) + (2U + (2U * MAX_KID_LEN)) + (2U * MAX_KID_CONTEXT_LEN))

#if SETTINGS_MAX_NAME_LEN < OSCORE_SSN_KEY_WORST_LEN
#warning "SETTINGS_MAX_NAME_LEN too small for OSCORE SSN keys with maximum-size (8/8/8) \
context identifiers; nvm_read_ssn()/nvm_write_ssn() fail with buffer_to_small"
#endif

/* Keep the literal 11 above in sync with the actual prefix length. */
BUILD_ASSERT(OSCORE_SSN_KEY_PREFIX_LEN == 11U);

/*
 * The settings key is the hex encoded concatenation of the public context
 * identifiers: sender ID, recipient ID and ID context. The two leading IDs are
 * preceded by a one-byte length prefix so their boundary is explicit; the ID
 * context is trailing and consumes the remainder, so it needs no prefix.
 */
static enum err key_append_field(char *key, size_t key_size, size_t *offset,
				 const struct byte_array *field)
{
	uint8_t len_byte;

	if (field->len > UINT8_MAX) {
		return wrong_parameter;
	}

	/* Length prefix (2 hex chars) + field content (2 hex chars each) + NUL. */
	if ((*offset + 2U + ((size_t)field->len * 2U) + 1U) > key_size) {
		return buffer_to_small;
	}

	len_byte = (uint8_t)field->len;
	*offset += bin2hex(&len_byte, sizeof(len_byte), key + *offset, key_size - *offset);

	if (field->len > 0U) {
		*offset += bin2hex(field->ptr, field->len, key + *offset, key_size - *offset);
	}

	return ok;
}

static enum err oscore_ssn_key(const struct nvm_key_t *nvm_key, char *key, size_t key_size)
{
	size_t offset = OSCORE_SSN_KEY_PREFIX_LEN;
	enum err r;

	if ((OSCORE_SSN_KEY_PREFIX_LEN + 1U) > key_size) {
		return buffer_to_small;
	}

	memcpy(key, OSCORE_SSN_KEY_PREFIX, OSCORE_SSN_KEY_PREFIX_LEN);

	r = key_append_field(key, key_size, &offset, &nvm_key->sender_id);
	if (r != ok) {
		return r;
	}

	r = key_append_field(key, key_size, &offset, &nvm_key->recipient_id);
	if (r != ok) {
		return r;
	}

	/* Trailing ID context: no length prefix needed, just the hex content. */
	if ((offset + ((size_t)nvm_key->id_context.len * 2U) + 1U) > key_size) {
		return buffer_to_small;
	}

	if (nvm_key->id_context.len > 0U) {
		(void)bin2hex(nvm_key->id_context.ptr, nvm_key->id_context.len, key + offset,
			      key_size - offset);
	}

	return ok;
}

struct ssn_load_ctx {
	uint64_t *ssn;
	bool found;
};

static int ssn_direct_load(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg,
			   void *param)
{
	struct ssn_load_ctx *ctx = param;
	const char *next;
	ssize_t rc;

	/* Only the exact leaf (no trailing key components) is of interest. */
	if (settings_name_next(name, &next) != 0) {
		return 0;
	}

	if (len != sizeof(*ctx->ssn)) {
		return -EINVAL;
	}

	rc = read_cb(cb_arg, ctx->ssn, sizeof(*ctx->ssn));
	if (rc < 0) {
		return (int)rc;
	}

	ctx->found = true;
	return 0;
}

enum err nvm_write_ssn(const struct nvm_key_t *nvm_key, uint64_t ssn)
{
	char key[SETTINGS_MAX_NAME_LEN + 1];
	enum err r;
	int ret;

	if (nvm_key == NULL) {
		return wrong_parameter;
	}

	r = oscore_ssn_key(nvm_key, key, sizeof(key));
	if (r != ok) {
		return r;
	}

	ret = settings_save_one(key, &ssn, sizeof(ssn));
	if (ret != 0) {
		LOG_ERR("Failed to write SSN to NVM: %d", ret);
		return unexpected_result_from_ext_lib;
	}

	return ok;
}

enum err nvm_read_ssn(const struct nvm_key_t *nvm_key, uint64_t *ssn)
{
	char key[SETTINGS_MAX_NAME_LEN + 1];
	struct ssn_load_ctx ctx;
	enum err r;
	int ret;

	if ((nvm_key == NULL) || (ssn == NULL)) {
		return wrong_parameter;
	}

	r = oscore_ssn_key(nvm_key, key, sizeof(key));
	if (r != ok) {
		return r;
	}

	ctx.ssn = ssn;
	ctx.found = false;

	ret = settings_load_subtree_direct(key, ssn_direct_load, &ctx);
	if (ret != 0) {
		LOG_ERR("Failed to read SSN from NVM: %d", ret);
		return unexpected_result_from_ext_lib;
	}

	if (!ctx.found) {
		LOG_ERR("No stored SSN found for the given context");
		return unexpected_result_from_ext_lib;
	}

	return ok;
}

static int oscore_nvm_init(void)
{
	int ret = settings_subsys_init();

	if (ret != 0) {
		LOG_ERR("settings_subsys_init failed: %d", ret);
	}

	return ret;
}

SYS_INIT(oscore_nvm_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
