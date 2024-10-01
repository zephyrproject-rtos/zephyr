/*
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <soc/soc_memory_layout.h>
#include <zephyr/multi_heap/shared_multi_heap.h>

int main(void)
{
	int *m_ext, *m_int;

	m_ext = shared_multi_heap_aligned_alloc(SMH_REG_ATTR_EXTERNAL, 32, 4096);

	if (!esp_ptr_external_ram(m_ext)) {
		printk("SPIRAM mem test failed\n");
	} else {
		printk("SPIRAM mem test pass\n");
	}

	shared_multi_heap_free(m_ext);

	m_int = k_malloc(4096);
	if (!esp_ptr_internal(m_int)) {
		printk("Internal mem test failed\n");
	} else {
		printk("Internal mem test pass\n");
	}

	k_free(m_int);

	return 0;
}
