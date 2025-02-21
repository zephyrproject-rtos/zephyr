/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/autoconf.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/types.h>
#include <zephyr/toolchain.h>

#if defined(CONFIG_SOC_SERIES_BSIM_NRFXX)
#include "bstests.h"
#endif /* CONFIG_SOC_SERIES_BSIM_NRFXX */

#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME bttester_main
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

#include "btp/btp.h"

int main(void)
{
	tester_init();

#if defined(CONFIG_SOC_SERIES_BSIM_NRFXX)
	bst_main();
#endif /* CONFIG_SOC_SERIES_BSIM_NRFXX */

	return 0;
}
