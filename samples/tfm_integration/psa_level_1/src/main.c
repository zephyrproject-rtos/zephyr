/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <logging/log_ctrl.h>
#include <logging/log.h>

#include "psa/crypto.h"

#include "psa_attestation.h"
#include "util_app_cfg.h"
#include "util_app_log.h"
#include "util_sformat.h"

/** Declare a reference to the application logging interface. */
LOG_MODULE_DECLARE(app, CONFIG_LOG_DEFAULT_LEVEL);

/* Create an instance of the system config struct for the application. */
static struct cfg_data cfg;

/**
 * @brief Generates random values using the TF-M crypto service.
 */
void crypto_test(void)
{
	psa_status_t status;
	u8_t outbuf[256] = { 0 };
	struct sf_hex_tbl_fmt fmt = {
		.ascii = true,
		.addr_label = true,
		.addr = 0
	};

	status = al_psa_status(psa_generate_random(outbuf, 256), __func__);
	LOG_INF("Generating 256 bytes of random data.");
	al_dump_log();
	sf_hex_tabulate_16(&fmt, outbuf, 256);
}

void main(void)
{
	/* Initialise the logger subsys and dump the current buffer. */
	log_init();

	/* Load app config struct from secure storage (create if missing). */
	if (cfg_load_data(&cfg)) {
		LOG_ERR("Error loading/generating app config data in SS.");
	}

	/* Get the entity attestation token (requires ~1kB stack memory!). */
	att_test();

        /* Dump any queued log messages, and wait for system events. */
	while (1) {
		al_dump_log();
	}
}
