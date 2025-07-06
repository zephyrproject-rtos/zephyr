/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/acpi/acpi.h>
#include "soc.h"
#include "soc_gpio.h"

#define GET_GPIO_BASE_NUM ("\\_SB.GINF")

static int gpio_info_acpi_get(uint32_t bank_idx, uint32_t field_idx, uint32_t *ret_val)
{
	int status;
	ACPI_OBJECT args[2];
	ACPI_OBJECT_LIST arg_list;
	ACPI_OBJECT gpio_obj;

	arg_list.Count = ARRAY_SIZE(args);
	arg_list.Pointer = args;

	args[0].Type = ACPI_TYPE_INTEGER;
	args[0].Integer.Value = bank_idx;

	args[1].Type = ACPI_TYPE_INTEGER;
	args[1].Integer.Value = field_idx;

	status = acpi_invoke_method(GET_GPIO_BASE_NUM, &arg_list, &gpio_obj);
	if (status) {
		return status;
	}

	if (gpio_obj.Type == ACPI_TYPE_INTEGER) {
		*ret_val = gpio_obj.Integer.Value;
	}

	return status;
}

int soc_acpi_gpio_resource_get(int bank_idx, char *hid, char *uid, struct gpio_acpi_res *res)
{
	int ret;
	struct acpi_dev *acpi_child;
	struct acpi_irq_resource irq_res;
	struct acpi_mmio_resource mmio_res;
	uint32_t field_val[7];
	uint16_t irqs[CONFIG_ACPI_IRQ_VECTOR_MAX];
	struct acpi_reg_base reg_base[CONFIG_ACPI_MMIO_ENTRIES_MAX];

	acpi_child = acpi_device_get(hid, uid);
	if (!acpi_child) {
		printk("acpi_device_get failed\n");
		return -EIO;
	}

	mmio_res.mmio_max = ARRAY_SIZE(reg_base);
	mmio_res.reg_base = reg_base;
	ret = acpi_device_mmio_get(acpi_child, &mmio_res);
	if (ret) {
		printk("acpi_device_mmio_get failed\n");
		return ret;
	}

	irq_res.irq_vector_max = ARRAY_SIZE(irqs);
	irq_res.irqs = irqs;
	ret = acpi_device_irq_get(acpi_child, &irq_res);
	if (ret) {
		printk("acpi_device_irq_get failed\n");
		return ret;
	}

	res->irq = irq_res.irqs[0];
	res->irq_flags = irq_res.flags;

	if (ACPI_RESOURCE_TYPE_GET(&mmio_res) == ACPI_RES_TYPE_MEM) {
		for (int i = 0; i < ARRAY_SIZE(field_val); i++) {
			ret = gpio_info_acpi_get(bank_idx, i, &field_val[i]);
			if (ret) {
				printk("gpio_info_acpi_get failed\n");
				return ret;
			}
		}

		res->reg_base = ACPI_MMIO_GET(&mmio_res) & (~0x0FFFFFF);
		res->reg_base += field_val[0];
		res->len = ACPI_RESOURCE_SIZE_GET(&mmio_res);
		res->num_pins = field_val[1];
		res->pad_base = field_val[2];
		res->host_owner_reg = field_val[3];
		res->pad_owner_reg = field_val[4];
		res->intr_stat_reg = field_val[5] - 0x40;
		res->base_num = field_val[6];
	} else {
		printk("ACPI_RES_TYPE_MEM failed\n");
		return -ENODEV;
	}

	return 0;
}
