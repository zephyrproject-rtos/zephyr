/*
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <soc/soc_memory_layout.h>
#include <zephyr/multi_heap/shared_multi_heap.h>

#define BUF_SIZE (4096*8)
#define BYTE_PAT 0xa5
#define WORD_PAT 0xaa55
#define DWORD_PAT 0xaaaa5555

int main(void)
{
	uint32_t *m_ext, *m_int;
	uint16_t *m_ext_w;
	uint8_t *m_ext_b;
	int err_b, err_w, err_dw;
	int i;

	m_ext = shared_multi_heap_aligned_alloc(SMH_REG_ATTR_EXTERNAL, 32, BUF_SIZE);
	m_ext_b = (uint8_t *) m_ext;
	m_ext_w = (uint16_t *) m_ext;
	err_b = err_w = err_dw = 0;

	/* Byte access */
	for (i = 0; i < BUF_SIZE; i++) {
		m_ext_b[i] = BYTE_PAT;
	}

	for (i = 0; i < BUF_SIZE; i++) {
		if (m_ext_b[i] != BYTE_PAT) {
			err_b++;
		}
	}

	if (err_b) {
		printk("Byte access errors: %d\n", err_b);
	}

	/* Word access */
	for (i = 0; i < BUF_SIZE / 2; i++) {
		m_ext_w[i] = WORD_PAT;
	}

	for (i = 0; i < BUF_SIZE / 2; i++) {
		if (m_ext_w[i] != WORD_PAT) {
			err_w++;
		}
	}

	if (err_w) {
		printk("Word access errors: %d\n", err_w);
	}

	/* DWord access */
	for (i = 0; i < BUF_SIZE / 4; i++) {
		m_ext[i] = DWORD_PAT;
	}

	for (i = 0; i < BUF_SIZE / 4; i++) {
		if (m_ext[i] != DWORD_PAT) {
			err_dw++;
		}
	}

	if (err_dw) {
		printk("DWord access errors: %d\n", err_dw);
	}

	if (!esp_ptr_external_ram(m_ext) && !err_b && !err_w && !err_dw) {
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
