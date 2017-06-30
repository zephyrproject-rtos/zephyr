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
		u8_t buffer[BUFFER_LENGTH] = {0};
		int r;

		/* The BUFFER_LENGTH-1 is used so the driver will not
		 * write the last byte of the buffer. If that last
		 * byte is not 0 on return it means the driver wrote
		 * outside the passed buffer, and that should never
		 * happen.
		 */
		r = random_get_entropy(dev, buffer, BUFFER_LENGTH-1);
		if (r) {
			printf("random_get_entropy failed: %d\n", r);
			break;
		}

		if (buffer[BUFFER_LENGTH-1] != 0) {
			printf("random_get_entropy buffer overflow\n");
		}

		for (int i = 0; i < BUFFER_LENGTH-1; i++) {
			printf("  0x%02x", buffer[i]);
		}

		printf("\n");

		k_sleep(1000);
	}
}
