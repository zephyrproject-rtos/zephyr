/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <signal.h>

void nsi_raise_sigtrap(void)
{
	raise(SIGTRAP);
}
