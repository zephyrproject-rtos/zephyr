/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * See description in header
 */

#include <stdlib.h>

long nsi_host_random(void)
{
	return random();
}

void nsi_host_srandom(unsigned int seed)
{
	srandom(seed);
}
