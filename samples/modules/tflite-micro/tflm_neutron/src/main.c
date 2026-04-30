/*
 * Copyright 2026 NXP 
 * All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * MobileNet Image Classification Sample
 */

#include "main_functions.h"
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

/* Number of inference runs */
#define NUM_INFERENCE_RUNS 1

int main(int argc, char *argv[])
{
	printk("\n\n=== TFLM NXP Neutron Starting ===\n");

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
