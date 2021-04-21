/*
 * Copyright (c) 2016 ARM Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <drivers/entropy.h>
#include <stdio.h>

void main(void)
{
	const struct device *dev;

	printf("Entropy Example! %s\n", CONFIG_ARCH);

	dev = device_get_binding(DT_CHOSEN_ZEPHYR_ENTROPY_LABEL);
	if (!dev) {
		printf("error: no entropy device\n");
		return;
	}

	printf("entropy device is %p, name is %s\n",
	       dev, dev->name);

	while (1) {
#define BUFFER_LENGTH 10
		uint8_t buffer[BUFFER_LENGTH] = {0};
		int r;

		/* The BUFFER_LENGTH-1 is used so the driver will not
		 * write the last byte of the buffer. If that last
		 * byte is not 0 on return it means the driver wrote
		 * outside the passed buffer, and that should never
		 * happen.
		 */
		r = entropy_get_entropy(dev, buffer, BUFFER_LENGTH-1);
		if (r) {
			printf("entropy_get_entropy failed: %d\n", r);
			break;
		}

		if (buffer[BUFFER_LENGTH-1] != 0U) {
			printf("entropy_get_entropy buffer overflow\n");
		}

		for (int i = 0; i < BUFFER_LENGTH-1; i++) {
			printf("  0x%02x", buffer[i]);
		}

		printf("\n");

		k_sleep(K_MSEC(1000));
	}
}
