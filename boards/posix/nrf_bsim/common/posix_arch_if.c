/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nsi_main.h"

void posix_exit(int exit_code)
{
	nsi_exit(exit_code);
}
