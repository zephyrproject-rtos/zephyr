/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
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
