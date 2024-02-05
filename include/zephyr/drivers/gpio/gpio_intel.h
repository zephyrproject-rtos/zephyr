/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct gpio_acpi_res {
	uint8_t num_pins;
	uint32_t pad_base;
	uint32_t host_owner_reg;
	uint32_t pad_owner_reg;
	uint32_t intr_stat_reg;
	uint16_t base_num;
	uintptr_t reg_base;
};
