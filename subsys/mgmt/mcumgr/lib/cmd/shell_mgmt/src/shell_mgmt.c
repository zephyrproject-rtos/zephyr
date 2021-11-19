/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/util.h>
#include <string.h>
#include <stdio.h>
#include "mgmt/mgmt.h"
#include "cborattr/cborattr.h"
#include "shell_mgmt/shell_mgmt.h"
#include "shell_mgmt/shell_mgmt_impl.h"
#include "shell_mgmt/shell_mgmt_config.h"

/**
 * Command handler: shell exec
 */
static int
shell_mgmt_exec(struct mgmt_ctxt *cb)
{
	static char line[SHELL_MGMT_MAX_LINE_LEN + 1] = {0};
	CborEncoder str_encoder;
	CborError err;
	int rc;
	char *argv[SHELL_MGMT_MAX_ARGC];
	int argc;

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

	err = cbor_read_object(&cb->it, attrs);
	if (err != 0) {
		return MGMT_ERR_EINVAL;
	}

	/* Key="o"; value=<command-output> */
	err |= cbor_encode_text_stringz(&cb->encoder, "o");
	err |= cbor_encoder_create_indef_text_string(&cb->encoder, &str_encoder);

	rc = shell_mgmt_impl_exec(line);

	err |= cbor_encode_text_stringz(&str_encoder,
		shell_mgmt_impl_get_output());

	err |= cbor_encoder_close_container(&cb->encoder, &str_encoder);

	/* Key="rc"; value=<status> */
	err |= cbor_encode_text_stringz(&cb->encoder, "rc");
	err |= cbor_encode_int(&cb->encoder, rc);

	if (err != 0) {
		return MGMT_ERR_ENOMEM;
	}

	return 0;
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
