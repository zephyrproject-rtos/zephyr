/*
 * Copyright (c) 2022 Intel Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef _ADSP_MEMORY_WINDOW_H_
#define _ADSP_MEMORY_WINDOW_H_

struct mem_win_config {
	uint32_t base_addr;
	uint32_t size;
	uint32_t offset;
	uint32_t mem_base;
	bool initialize;
	bool read_only;
};

#endif
