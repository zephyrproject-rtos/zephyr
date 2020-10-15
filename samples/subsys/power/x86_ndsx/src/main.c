/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <power/x86_non_dsx.h>

/* Application main Thread */
void main(void)
{
	pwrseq_thread(NULL, NULL, NULL);
}
