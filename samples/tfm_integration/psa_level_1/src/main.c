/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr.h>
#include <misc/printk.h>
#include <logging/log_ctrl.h>
#include <logging/log.h>

#include "psa/crypto.h"
#include "config.h"
#include "applog.h"

static struct cfg_data cfg;

/** Declare a reference to the application logging interface. */
LOG_MODULE_DECLARE(app, CONFIG_LOG_DEFAULT_LEVEL);

/**
 * @brief Generates random values using the TF-M crypto service.
 */
void crypto_test(void)
{
	psa_status_t status;
	u8_t outbuf[256] = { 0 };

	status = al_psa_status(psa_generate_random(outbuf, 256), __func__);
	for (int i = 0; i < 256; i++) {
		printk("%02X ", outbuf[i]);
	}
	printk("\n");
}

void cfg_display(void)
{
	printk("Magic:   0x%X\n", cfg.magic);
	printk("Version: %d\n", cfg.version);
	printk("Scratch: %d bytes\n", sizeof(cfg.scratch));
}

void main(void)
{
	psa_status_t status;

	/* Initialise the logger subsys. */
	log_init();

	/* Call a crypto function using the PSA FF APIs. */
	/* crypto_test(); */

	/* Try to load config data from secure storage. */
	status = cfg_load_data(&cfg);
	cfg_display();

	while (1) {
		/* Process queued logger message if present. */
		while (log_process(false)) {
		}
	}
}
