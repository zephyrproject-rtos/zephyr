/*
 * Copyright (c) 2024 Google Inc
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INCLUDE_ZEPHYR_DRIVERS_BBRAM_IT8XXX2_H_
#define INCLUDE_ZEPHYR_DRIVERS_BBRAM_IT8XXX2_H_

#include <stdint.h>

#include <zephyr/devicetree.h>

/** Device config */
struct bbram_it8xxx2_config {
	/** BBRAM base address */
	uintptr_t base_addr;
	/** BBRAM size (Unit:bytes) */
	int size;
};

#ifdef CONFIG_BBRAM_IT8XXX2_EMUL
#define BBRAM_IT8XXX2_DECL_CONFIG(inst)                                                            \
	static uint8_t bbram_it8xxx2_emul_buffer_##inst[DT_INST_REG_SIZE(inst)];                   \
	static const struct bbram_it8xxx2_config bbram_cfg_##inst = {                              \
		.base_addr = (uintptr_t)bbram_it8xxx2_emul_buffer_##inst,                          \
		.size = DT_INST_REG_SIZE(inst),                                                    \
	}
#else
#define BBRAM_IT8XXX2_DECL_CONFIG(inst)                                                            \
	static const struct bbram_it8xxx2_config bbram_cfg_##inst = {                              \
		.base_addr = DT_INST_REG_ADDR(inst),                                               \
		.size = DT_INST_REG_SIZE(inst),                                                    \
	}
#endif

#endif /* INCLUDE_ZEPHYR_DRIVERS_BBRAM_IT8XXX2_H_ */
