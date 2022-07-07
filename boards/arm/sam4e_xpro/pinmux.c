/*
 * Copyright (c) 2019 Gerson Fernando Budke
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>

static int sam4e_xpro_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

SYS_INIT(sam4e_xpro_init, PRE_KERNEL_1, 0);
