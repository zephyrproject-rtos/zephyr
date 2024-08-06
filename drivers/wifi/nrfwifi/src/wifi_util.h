/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* @file
 * @brief nRF Wi-Fi radio-test mode shell module
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/init.h>
#include <ctype.h>
#include <host_rpu_sys_if.h>
#include <fmac_structs.h>
#include <queue.h>

struct nrf_wifi_ctx_zep_rt {
	struct nrf_wifi_fmac_priv *fmac_priv;
	struct rpu_conf_params conf_params;
};
