/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Code coverage shell command.
 */

#include <zephyr/debug/gcov.h>
#include <zephyr/shell/shell.h>

SHELL_STATIC_SUBCMD_SET_CREATE(sub_coverage,
			       SHELL_CMD_ARG(dump, NULL,
					     "Dumps the code coverage data",
					     gcov_coverage_dump, 0, 0),
			       SHELL_SUBCMD_SET_END /* Array terminated */
);

SHELL_CMD_REGISTER(coverage, &sub_coverage, "Code coverage", NULL);
