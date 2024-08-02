/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct gpio_acpi_res {
	uintptr_t reg_base;
	uint32_t len;
	uint32_t pad_base;
	uint8_t num_pins;
	uint32_t host_owner_reg;
	uint32_t pad_owner_reg;
	uint32_t intr_stat_reg;
	uint16_t base_num;
	uint16_t irq;
	uint32_t irq_flags;
};

/**
 * @brief Retrieve current resource settings of a gpio device from acpi.
 *
 * @param bank_idx band index of the gpio group (eg: gpp_a, gpp_b etc.)
 * @param hid the hardware id of the acpi device
 * @param uid the unique id of the acpi device
 * @param res the pointer to resource struct on which data return
 * @return return 0 on success or error code
 */
int soc_acpi_gpio_resource_get(int bank_idx, char *hid, char *uid, struct gpio_acpi_res *res);
