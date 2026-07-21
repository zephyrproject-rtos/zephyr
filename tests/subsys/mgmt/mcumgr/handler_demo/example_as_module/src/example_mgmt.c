/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/mgmt/handlers.h>
/* The below should be updated with the real name of the file */
#include <example_mgmt.h>
#include <zephyr/logging/log.h>
#include <assert.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include <mgmt/mcumgr/util/zcbor_bulk.h>

#if defined(CONFIG_MCUMGR_MGMT_NOTIFICATION_HOOKS)
#include <zephyr/mgmt/mcumgr/mgmt/callbacks.h>
#if defined(CONFIG_MCUMGR_GRP_EXAMPLE_OTHER_HOOK)
/* The below should be updated with the real name of the file */
#include <example_mgmt_callbacks.h>
#endif
#endif

LOG_MODULE_REGISTER(mcumgr_example_grp, CONFIG_MCUMGR_GRP_EXAMPLE_LOG_LEVEL);
/* Example function with "read" command support, requires both parameters are supplied */
static int example_mgmt_test(struct smp_streamer *ctxt)
{
	uint32_t uint_value = 0;
	zcbor_state_t *zse = ctxt->writer->zs;
	zcbor_state_t *zsd = ctxt->reader->zs;
	bool ok;
	struct zcbor_string string_value = { 0 };
	size_t decoded;
	struct zcbor_map_decode_key_val example_test_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("uint_key", zcbor_uint32_decode, &uint_value),
		ZCBOR_MAP_DECODE_KEY_DECODER("string_key", zcbor_tstr_decode, &string_value),
	};

	LOG_DBG("Example test function called");

	ok = zcbor_map_decode_bulk(zsd, example_test_decode, ARRAY_SIZE(example_test_decode),
				   &decoded) == 0;
	/* Check that both parameters were supplied and that the value of "string_key" is not
	 * empty
	 */
	if (!ok || string_value.len == 0 || !zcbor_map_decode_bulk_key_found(
			   example_test_decode, ARRAY_SIZE(example_test_decode), "uint_key")) {
		return MGMT_ERR_EINVAL;
	}

	/* If the value of "uint_key" is over 50, return an error of "not wanted" */
	if (uint_value > 50) {
		ok = smp_add_cmd_err(zse, MGMT_GROUP_ID_EXAMPLE, EXAMPLE_MGMT_ERR_NOT_WANTED);
		goto end;
	}

	/* Otherwise, return an integer value of 4691 */
	ok = zcbor_tstr_put_lit(zse, "return_int") &&
	     zcbor_int32_put(zse, 4691);

end:
	/* If "ok" is false, then there was an error processing the output cbor message, which
	 * likely indicates a lack of available memory
	 */
	return (ok ? MGMT_ERR_EOK : MGMT_ERR_EMSGSIZE);
}

/* Example function with "write" command support */
static int example_mgmt_other(struct smp_streamer *ctxt)
{
	uint32_t user_value = 0;
	zcbor_state_t *zse = ctxt->writer->zs;
	zcbor_state_t *zsd = ctxt->reader->zs;
	bool ok;
	size_t decoded;
	struct zcbor_map_decode_key_val example_other_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("user_value", zcbor_uint32_decode, &user_value),
	};

#if defined(CONFIG_MCUMGR_GRP_EXAMPLE_OTHER_HOOK)
	struct example_mgmt_other_data other_data;
	enum mgmt_cb_return status;
	int32_t err_rc;
	uint16_t err_group;
#endif

	LOG_DBG("Example other function called");

	ok = zcbor_map_decode_bulk(zsd, example_other_decode, ARRAY_SIZE(example_other_decode),
				   &decoded) == 0;

	/* The supplied value is optional, therefore do not return an error if it was not
	 * provided
	 */
	if (!ok) {
		return MGMT_ERR_EINVAL;
	}

#if defined(CONFIG_MCUMGR_GRP_EXAMPLE_OTHER_HOOK)
	/* Send request to application to check what to do */
	other_data.user_value = user_value;
	status = mgmt_callback_notify(MGMT_EVT_OP_EXAMPLE_OTHER, &other_data, sizeof(other_data),
				      &err_rc, &err_group);
	if (status != MGMT_CB_OK) {
		/* If a callback returned an RC error, exit out, if it returned a group error
		 * code, add the error code to the response and return to the calling function to
		 * have it sent back to the client
		 */
		if (status == MGMT_CB_ERROR_RC) {
			return err_rc;
		}

		ok = smp_add_cmd_err(zse, err_group, (uint16_t)err_rc);
		goto end;
	}
#endif
	/* Return some dummy data to the client */
	ok = zcbor_tstr_put_lit(zse, "return_string") &&
	     zcbor_tstr_put_lit(zse, "some dummy data!");

#if defined(CONFIG_MCUMGR_GRP_EXAMPLE_OTHER_HOOK)
end:
#endif
	/* If "ok" is false, then there was an error processing the output cbor message, which
	 * likely indicates a lack of available memory
	 */
	return (ok ? MGMT_ERR_EOK : MGMT_ERR_EMSGSIZE);
}

#ifdef CONFIG_MCUMGR_SMP_SUPPORT_ORIGINAL_PROTOCOL
/* This is a lookup function that converts from SMP version 2 group error codes to legacy
 * MCUmgr error codes, it is only included if support for the original protocol is enabled.
 * Note that in SMP version 2, MCUmgr error codes can still be returned, but are to be used
 * only for general SMP/MCUmgr errors. The success/OK error code is not used in translation
 * functions as it is automatically handled by the base SMP code.
 */
static int example_mgmt_translate_error_code(uint16_t err)
{
	int rc;

	switch (err) {
	case EXAMPLE_MGMT_ERR_NOT_WANTED:
		rc = MGMT_ERR_ENOENT;
		break;

	case EXAMPLE_MGMT_ERR_REJECTED_BY_HOOK:
		rc = MGMT_ERR_EBADSTATE;
		break;

	case EXAMPLE_MGMT_ERR_UNKNOWN:
	default:
		rc = MGMT_ERR_EUNKNOWN;
	}

	return rc;
}
#endif

static const struct mgmt_handler example_mgmt_handlers[] = {
	[EXAMPLE_MGMT_ID_TEST] = {
		.mh_read = example_mgmt_test,
		.mh_write = NULL,
	},
	[EXAMPLE_MGMT_ID_OTHER] = {
		.mh_read = NULL,
		.mh_write = example_mgmt_other,
	},
};

static struct mgmt_group example_mgmt_group = {
	.mg_handlers = example_mgmt_handlers,
	.mg_handlers_count = ARRAY_SIZE(example_mgmt_handlers),
	.mg_group_id = MGMT_GROUP_ID_EXAMPLE,
#ifdef CONFIG_MCUMGR_SMP_SUPPORT_ORIGINAL_PROTOCOL
	.mg_translate_error = example_mgmt_translate_error_code,
#endif
};

static void example_mgmt_register_group(void)
{
	/* This function is called during system init before main() is invoked, if the
	 * handler needs to set anything up before it can be used, it should do it here.
	 * This register the group so that clients can call the function handlers.
	 */
	mgmt_register_group(&example_mgmt_group);
}

MCUMGR_HANDLER_DEFINE(example_mgmt, example_mgmt_register_group);
