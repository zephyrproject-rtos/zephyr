/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/util.h>
#include <string.h>
#include <stdio.h>
#include <mgmt/mcumgr/buf.h>
#include "mgmt/mgmt.h"
#include <zcbor_common.h>
#include <zcbor_encode.h>
#include "cborattr/cborattr.h"
#include "shell_mgmt/shell_mgmt.h"
#include "shell_mgmt/shell_mgmt_config.h"
#include <shell/shell_dummy.h>

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
	char line[SHELL_MGMT_MAX_LINE_LEN + 1];
	zcbor_state_t *zse = ctxt->cnbe->zs;
	CborError err;
	int rc;
	char *argv[SHELL_MGMT_MAX_ARGC];
	int argc;
	bool ok;
	struct zcbor_string cmd_out;

	const struct cbor_attr_t attrs[] = {
		{
			.attribute = "argv",
			.type = CborAttrArrayType,
			.addr.array = {
				.element_type = CborAttrTextStringType,
				.arr.strings.ptrs = argv,
				.arr.strings.store = line,
				.arr.strings.storelen = sizeof(line),
				.count = &argc,
				.maxlen = ARRAY_SIZE(argv),
			},
		},
		{ 0 },
	};

	line[0] = 0;

	err = cbor_read_object(&ctxt->it, attrs);
	if (err != 0) {
		return MGMT_ERR_EINVAL;
	}

	line[ARRAY_SIZE(line) - 1] = 0;

	rc = shell_exec(line);
	cmd_out.value = shell_get_output(&cmd_out.len);

	/* Key="o"; value=<command-output> */
	/* Key="rc"; value=<status> */
	ok = zcbor_tstr_put_lit(zse, "o")		&&
	     zcbor_tstr_encode(zse, &cmd_out)		&&
	     zcbor_tstr_put_lit(zse, "rc")		&&
	     zcbor_int32_put(zse, rc);

	return ok ? 0 : MGMT_ERR_ENOMEM;
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
