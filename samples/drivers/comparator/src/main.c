/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/comparator.h>
#include <stdio.h>

static void print_state(uint32_t state)
{
	if (state & COMPARATOR_STATE_BELOW) {
		printf("BELOW\n");
	} else if (state & COMPARATOR_STATE_ABOVE) {
		printf("ABOVE\n");
	}
}

static void comparator_callback(const struct device *dev,
				uint32_t state,
				void *user_data)
{
	printf("Callback: ");
	print_state(state);
}

int main(void)
{
	printf("Comparator driver sample on %s\n", CONFIG_BOARD);

	const struct device *comp_dev = DEVICE_DT_GET(DT_NODELABEL(comp_dev));
	uint32_t initial_state;
	int rc;

	if (!device_is_ready(comp_dev)) {
		printf("%s is not ready\n", comp_dev->name);
		return -1;
	}

	rc = comparator_set_callback(comp_dev, comparator_callback, NULL);
	if (rc < 0) {
		printf("comparator_callback_set failed: %d\n", rc);
		return -1;
	}

	rc = comparator_start(comp_dev);
	if (rc < 0) {
		printf("comparator_start failed: %d\n", rc);
		return -1;
	}

	printf("Comparator started\n");

	rc = comparator_get_state(comp_dev, &initial_state);
	if (rc == 0) {
		printf("Initial state: ");
		print_state(initial_state);
	} else if (rc == -ENOSYS) {
		printf("comparator_get_state is not implemented\n");
	} else if (rc < 0) {
		printf("comparator_get_state failed: %d\n", rc);
		return -1;
	}

	printf("Sleeping for 20 seconds\n");
	printf("Change voltages on comparator inputs to see callback notifications\n");
	k_sleep(K_SECONDS(20));

	printf("Comparator stopped\n");

	rc = comparator_stop(comp_dev);
	if (rc < 0) {
		printf("comparator_stop failed: %d\n", rc);
		return -1;
	}

	return 0;
}
