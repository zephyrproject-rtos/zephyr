/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <power/x86_non_dsx.h>

/* Application main Thread */
void main(void)
{
	int32_t t_time = 1000;

	pwrseq_thread((void *)&t_time, NULL, NULL);
}
