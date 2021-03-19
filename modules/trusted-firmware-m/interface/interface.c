/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <init.h>
#include <kernel.h>

#include <tfm_ns_interface.h>

#if defined(TFM_PSA_API)
#include "psa_manifest/sid.h"
#endif /* TFM_PSA_API */

static int ns_interface_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	(void)tfm_ns_interface_init();

	return 0;
}

/* Initialize the TFM NS interface */
SYS_INIT(ns_interface_init, POST_KERNEL,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
