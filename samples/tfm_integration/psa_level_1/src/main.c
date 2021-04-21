/*
 * Copyright (c) 2019,2020 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <logging/log_ctrl.h>
#include <logging/log.h>

#include "tfm_ns_interface.h"
#include "psa_attestation.h"
#include "psa_crypto.h"
#include "util_app_cfg.h"
#include "util_app_log.h"
#include "util_sformat.h"

/** Declare a reference to the application logging interface. */
LOG_MODULE_DECLARE(app, CONFIG_LOG_DEFAULT_LEVEL);

/* Create an instance of the system config struct for the application. */
static struct cfg_data cfg;

void main(void)
{
	/* Initialize the TFM NS interface */
	tfm_ns_interface_init();

	/* Initialise the logger subsys and dump the current buffer. */
	log_init();

	/* Load app config struct from secure storage (create if missing). */
	if (cfg_load_data(&cfg)) {
		LOG_ERR("Error loading/generating app config data in SS.");
	}

	/* Get the entity attestation token (requires ~1kB stack memory!). */
	att_test();

	/* Crypto tests */
	crp_test_rng();
	crp_test_sha256();

	/* Dump any queued log messages, and wait for system events. */
	while (1) {
		al_dump_log();
	}

	while (1) {

	}
}
