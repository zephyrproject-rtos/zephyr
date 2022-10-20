/*
 * Copyright (c) 2022 Intel Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef _ADSP_MEMORY_WINDOW_H_
#define _ADSP_MEMORY_WINDOW_H_

#define WIN_SIZE(N) (CONFIG_MEMORY_WIN_##N##_SIZE)

#define MEM_WINDOW_NODE(n) DT_NODELABEL(mem_window##n)
#define WIN0_OFFSET DT_PROP(MEM_WINDOW_NODE(0), offset)
#define WIN1_OFFSET WIN0_OFFSET + WIN_SIZE(0)
#define WIN2_OFFSET WIN1_OFFSET + WIN_SIZE(1)
#define WIN3_OFFSET WIN2_OFFSET + WIN_SIZE(2)



#define WIN_OFFSET(n) (DT_PROP_OR(MEM_WINDOW_NODE(n), offset, (WIN##n##_OFFSET)))

#define HP_SRAM_WIN0_BASE L2_SRAM_BASE + WIN0_OFFSET
#define HP_SRAM_WIN0_SIZE WIN_SIZE(0)

#define HP_SRAM_WIN1_BASE L2_SRAM_BASE + WIN1_OFFSET
#define HP_SRAM_WIN1_SIZE WIN_SIZE(1)

#define HP_SRAM_WIN2_BASE L2_SRAM_BASE + WIN2_OFFSET
#define HP_SRAM_WIN2_SIZE WIN_SIZE(2)

#define HP_SRAM_WIN3_BASE L2_SRAM_BASE + WIN3_OFFSET
#define HP_SRAM_WIN3_SIZE WIN_SIZE(3)

#ifndef _LINKER
struct mem_win_config {
	uint32_t base_addr;
	uint32_t size;
	uint32_t offset;
	uint32_t mem_base;
	bool initialize;
	bool read_only;
};

#endif

#endif
