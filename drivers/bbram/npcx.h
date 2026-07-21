/*
 * Copyright (c) 2024 Google Inc
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INCLUDE_ZEPHYR_DRIVERS_BBRAM_NPCX_H_
#define INCLUDE_ZEPHYR_DRIVERS_BBRAM_NPCX_H_

#include <stdint.h>

#include <zephyr/devicetree.h>

/** Device config */
struct bbram_npcx_config {
	/** BBRAM base address */
	uintptr_t base_addr;
	/** BBRAM size (Unit:bytes) */
	int size;
	/** Status register base address */
	uintptr_t status_reg_addr;
};

#ifdef CONFIG_BBRAM_NPCX_EMUL
#define BBRAM_NPCX_DECL_CONFIG(inst)                                                               \
	static uint8_t bbram_npcx_emul_buffer_##inst[DT_INST_REG_SIZE_BY_NAME(inst, memory)];      \
	static uint8_t bbram_npcx_emul_status_##inst;                                              \
	static const struct bbram_npcx_config bbram_cfg_##inst = {                                 \
		.base_addr = (uintptr_t)bbram_npcx_emul_buffer_##inst,                             \
		.size = DT_INST_REG_SIZE_BY_NAME(inst, memory),                                    \
		.status_reg_addr = (uintptr_t)&bbram_npcx_emul_status_##inst,                      \
	}
#else
#define BBRAM_NPCX_DECL_CONFIG(inst)                                                               \
	static const struct bbram_npcx_config bbram_cfg_##inst = {                                 \
		.base_addr = DT_INST_REG_ADDR_BY_NAME(inst, memory),                               \
		.size = DT_INST_REG_SIZE_BY_NAME(inst, memory),                                    \
		.status_reg_addr = DT_INST_REG_ADDR_BY_NAME(inst, status),                         \
	}
#endif

#endif /* INCLUDE_ZEPHYR_DRIVERS_BBRAM_NPCX_H_ */
