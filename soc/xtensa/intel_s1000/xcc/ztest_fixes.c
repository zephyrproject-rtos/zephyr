/*
 * Copyright (c) 2019, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>

#ifndef CONFIG_ZTEST_RETEST_IF_PASSED

void sys_reboot(int type)
{
	ARG_UNUSED(type);
}

#endif /* CONFIG_ZTEST_RETEST_IF_PASSED */
