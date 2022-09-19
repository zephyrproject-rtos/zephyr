/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <string.h>
#include <stdio.h>
#include <zephyr/mgmt/mcumgr/buf.h>
#include "mgmt/mgmt.h"
#include <zcbor_common.h>
#include <zcbor_encode.h>
#include <zcbor_decode.h>
#include "shell_mgmt/shell_mgmt.h"
#include "shell_mgmt/shell_mgmt_config.h"
#include <zephyr/shell/shell_dummy.h>

static int
shell_exec(const char *line)
{
	const struct shell *shell = shell_backend_dummy_get_ptr();

	shell_backend_dummy_clear_output(shell);
	return shell_execute_cmd(shell, line);
}

const char *
shell_get_output(size_t *len)
{
	return shell_backend_dummy_get_output(
		shell_backend_dummy_get_ptr(),
		len
	);
}

/**
 * Command handler: shell exec
 */
static int
shell_mgmt_exec(struct mgmt_ctxt *ctxt)
{
	int rc;
	bool ok;
	char line[SHELL_MGMT_MAX_LINE_LEN + 1];
	size_t len = 0;
	struct zcbor_string cmd_out;
	zcbor_state_t *zsd = ctxt->cnbd->zs;
	zcbor_state_t *zse = ctxt->cnbe->zs;

	if (!zcbor_map_start_decode(zsd)) {
		return MGMT_ERR_EINVAL;
	}

	/* Expecting single array named "argv" */
	do {
		struct zcbor_string key;
		static const char argv_keyword[] = "argv";

		ok = zcbor_tstr_decode(zsd, &key);

		if (ok) {
			if (key.len == (ARRAY_SIZE(argv_keyword) - 1) &&
			    memcmp(key.value, argv_keyword, ARRAY_SIZE(argv_keyword) - 1) == 0) {
				break;
			}

			ok = zcbor_any_skip(zsd, NULL);
		}
	} while (ok);

	if (!ok || !zcbor_list_start_decode(zsd)) {
		return MGMT_ERR_EINVAL;
	}

	/* Compose command line */
	do {
		struct zcbor_string value;

		ok = zcbor_tstr_decode(zsd, &value);
		if (ok) {
			/* TODO: This is original error when failed to collect command line
			 * to buffer, but should be rather MGMT_ERR_ENOMEM.
			 */
			if ((len + value.len) >= (ARRAY_SIZE(line) - 1)) {
				return MGMT_ERR_EINVAL;
			}

			memcpy(&line[len], value.value, value.len);
			len += value.len + 1;
			line[len - 1] = ' ';
		} else {
			line[len - 1] = 0;
			/* Implicit break by while condition */
		}
	} while (ok);

	zcbor_list_end_decode(zsd);

	/* Failed to compose command line? */
	if (len == 0) {
		/* We do not bother to close decoder */
		return MGMT_ERR_EINVAL;
	}

	rc = shell_exec(line);
	cmd_out.value = shell_get_output(&cmd_out.len);

	/* Key="o"; value=<command-output> */
	/* Key="ret"; value=<status>, or rc if legacy option enabled */
	ok = zcbor_tstr_put_lit(zse, "o")		&&
	     zcbor_tstr_encode(zse, &cmd_out)		&&
#ifdef CONFIG_MCUMGR_CMD_SHELL_MGMT_LEGACY_RC_RETURN_CODE
	     zcbor_tstr_put_lit(zse, "rc")		&&
#else
	     zcbor_tstr_put_lit(zse, "ret")		&&
#endif
	     zcbor_int32_put(zse, rc);

	zcbor_map_end_decode(zsd);

	return ok ? MGMT_ERR_EOK : MGMT_ERR_EMSGSIZE;
}

static struct mgmt_handler shell_mgmt_handlers[] = {
	[SHELL_MGMT_ID_EXEC] = { NULL, shell_mgmt_exec },
};

#define SHELL_MGMT_HANDLER_CNT ARRAY_SIZE(shell_mgmt_handlers)

static struct mgmt_group shell_mgmt_group = {
	.mg_handlers = shell_mgmt_handlers,
	.mg_handlers_count = SHELL_MGMT_HANDLER_CNT,
	.mg_group_id = MGMT_GROUP_ID_SHELL,
};


void
shell_mgmt_register_group(void)
{
	mgmt_register_group(&shell_mgmt_group);
}
