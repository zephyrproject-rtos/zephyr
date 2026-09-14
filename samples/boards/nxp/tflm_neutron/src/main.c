/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 *
 * MobileNet Image Classification Sample
 */

#include "main_functions.h"
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "NeutronDriver.h"

/* Number of inference runs */
#define NUM_INFERENCE_RUNS 3

int main(void)
{
	NeutronSdkVersion sdkVersion;

	printk("\n\n=== TFLM NXP Neutron Starting ===\n");
	/* Print Neutron driver version once the NPU is powered up */
	sdkVersion = neutronGetSdkVersion();
	printk("=== Neutron Software version: %u.%u.%u-%s ===\n\n", (unsigned int)sdkVersion.major,
	       (unsigned int)sdkVersion.minor, (unsigned int)sdkVersion.patch,
	       sdkVersion.hashString);

	/* Initialize the model and interpreter */
	setup();

	/* Run inference */
	for (int i = 0; i < NUM_INFERENCE_RUNS; i++) {
		loop();
		k_msleep(100);
	}

	printk("Inference complete!\n");
	return 0;
}
