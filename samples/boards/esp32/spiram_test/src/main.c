/*
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <soc/soc_memory_layout.h>

static int check_allocated_memory(int *m1, size_t size)
{
	int ret = 0;

	if (size < CONFIG_ESP_HEAP_MIN_EXTRAM_THRESHOLD) {
		if (!esp_ptr_internal(m1)) {
			printk("Memory allocation is not within"
					" specified bounds\n");
			ret = -1;
		}
	} else {
		if (!esp_ptr_external_ram(m1)) {
			printk("Memory allocation is not within"
					" specified bounds\n");
			ret = -1;
		}
	}
	return ret;
}

static int test_heap_caps(size_t size)
{
	int *m1, *m2;
	int cnt = 0, err = 0;

	m2 = NULL;
	/* Allocated as much as we can, and create linked list */
	while ((m1 = k_malloc(size))) {
		if (check_allocated_memory(m1, size) == -1) {
			err = -1;
			goto ret;
		}
		*(int **) m1 = m2;
		m2 = m1;
		cnt++;
	}

	/* Free all allocated memory */
	while (m2) {
		m1 = *(int **) m2;
		k_free(m2);
		m2 = m1;
	}
	/* Confirm that allocation can succeed now */
	m1 = k_malloc(size);
	if (check_allocated_memory(m1, size) == -1) {
		err = -1;
		goto ret;
	}
	if (!m1) {
		err = -1;
	} else {
		k_free(m1);
		printk("mem test ok! %d\n", cnt);
	}
ret:
	return err;
}

void main(void)
{
	int err = test_heap_caps(10001);

	if (err == -1) {
		printk("SPIRAM mem test failed\n");
	} else {
		printk("SPIRAM mem test pass\n");
	}

	err = test_heap_caps(1001);
	if (err == -1) {
		printk("Internal mem test failed\n");
	} else {
		printk("Internal mem test pass\n");
	}
}
