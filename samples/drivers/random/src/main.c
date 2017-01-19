/*
 * Copyright (c) 2016 ARM Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <random.h>
#include <stdio.h>

void main(void)
{
	struct device *dev;

	printf("Random Example! %s\n", CONFIG_ARCH);

	dev = device_get_binding(CONFIG_RANDOM_NAME);
	if (!dev) {
		printf("error: no random device\n");
		return;
	}

	printf("random device is %p, name is %s\n",
	       dev, dev->config->name);

	while (1) {
#define BUFFER_LENGTH 10
		uint8_t buffer[BUFFER_LENGTH];
		int r;

		r = random_get_entropy(dev, buffer, BUFFER_LENGTH);
		if (r) {
			printf("random_get_entropy failed: %d\n", r);
			break;
		};

		for (int i = 0; i < BUFFER_LENGTH; i++) {
			printf("  0x%x", buffer[i]);
		};

		printf("\n");

		k_sleep(1000);
	}
}
