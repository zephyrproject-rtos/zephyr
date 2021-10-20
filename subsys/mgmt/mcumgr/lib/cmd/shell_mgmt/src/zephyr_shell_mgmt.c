/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/util.h>
#include <shell/shell.h>
#include <mgmt/mgmt.h>
#include <shell_mgmt/shell_mgmt.h>
#include <shell/shell_dummy.h>

int
shell_mgmt_impl_exec(const char *line)
{
	const struct shell *shell = shell_backend_dummy_get_ptr();

	shell_backend_dummy_clear_output(shell);
	return shell_execute_cmd(shell, line);
}

const char *
shell_mgmt_impl_get_output()
{
	size_t len;

	return shell_backend_dummy_get_output(
		shell_backend_dummy_get_ptr(),
		&len
	);
}
