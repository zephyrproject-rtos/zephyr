/*
 * Copyright (c) 2019,2020, 2021, 2023 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "tfm_api.h"
#include "tfm_ns_interface.h"
#ifdef TFM_PSA_API
#include "psa_manifest/sid.h"
#include "psa/crypto.h"
#endif

/**
 * \brief Retrieve the version of the PSA Framework API.
 *
 * \note This is a functional test only and doesn't
 *       mean to test all possible combinations of
 *       input parameters and return values.
 */
static void tfm_get_version(void)
{
	uint32_t version;

	version = psa_framework_version();
	if (version == PSA_FRAMEWORK_VERSION) {
		printk("The version of the PSA Framework API is %d.\n",
		       version);
	} else {
		printk("The version of the PSA Framework API is not valid!\n");
		return;
	}
}

#ifdef TFM_PSA_API
/**
 * \brief Retrieve the minor version of a RoT Service.
 */
static void tfm_get_sid(void)
{
	uint32_t version;

	version = psa_version(TFM_CRYPTO_SID);
	if (version == PSA_VERSION_NONE) {
		printk("RoT Service is not implemented or caller is not ");
		printk("authorized to access it!\n");
		return;
	}

	/* Valid version number */
	printk("The PSA Crypto service minor version is %d.\n", version);
}

/**
 * \brief Generates random data using the TF-M crypto service.
 */
void tfm_psa_crypto_rng(void)
{
	psa_status_t status;
	uint8_t outbuf[256] = { 0 };

	status = psa_generate_random(outbuf, 256);
	printk("Generating 256 bytes of random data:");
	for (uint16_t i = 0; i < 256; i++) {
		if (!(i % 16)) {
			printk("\n");
		}
		printk("%02X ", (uint8_t)(outbuf[i] & 0xFF));
	}
	printk("\n");
}
#endif

int main(void)
{
	printk("TF-M IPC on %s\n", CONFIG_BOARD);

	tfm_get_version();
#ifdef TFM_PSA_API
	tfm_get_sid();
	tfm_psa_crypto_rng();
#endif
	return 0;
}
