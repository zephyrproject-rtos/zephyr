/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/acpi/acpi.h>
#include "soc.h"
#include "soc_gpio.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(soc_gpio, CONFIG_SOC_LOG_LEVEL);

#define GET_GPIO_BASE_NUM ("\\_SB.GINF")

static int gpio_info_acpi_get(uint8_t uid, uint32_t bank_idx, uint32_t field_idx,
			      uint32_t *ret_val, bool ginf)
{
	int status;
	ACPI_OBJECT args[3];
	ACPI_OBJECT_LIST arg_list;
	ACPI_OBJECT gpio_obj;

	arg_list.Pointer = args;

	if (ginf) {
		arg_list.Count = 3;
		args[0].Type = ACPI_TYPE_INTEGER;
		/* u-id is used to indicate the order of communities in ACPI table,
		 * in GINF method with u-id parameter community 2 is not existing
		 * but ACPI table stores community 3 group values in uid index 2
		 * and same for communities 4 and 5. So following logic is used
		 * while sending u-id to GINF.
		 */
		args[0].Integer.Value = (uid > GINF_MISSING_UID) ? (--uid) : uid;

		args[1].Type = ACPI_TYPE_INTEGER;
		args[1].Integer.Value = bank_idx;

		args[2].Type = ACPI_TYPE_INTEGER;
		args[2].Integer.Value = field_idx;
	} else {
		arg_list.Count = 2;
		args[0].Type = ACPI_TYPE_INTEGER;
		args[0].Integer.Value = bank_idx;

		args[1].Type = ACPI_TYPE_INTEGER;
		args[1].Integer.Value = field_idx;
	}

	status = acpi_invoke_method(GET_GPIO_BASE_NUM, &arg_list, &gpio_obj);
	if (status) {
		return status;
	}

	if (gpio_obj.Type == ACPI_TYPE_INTEGER) {
		*ret_val = gpio_obj.Integer.Value;
	}

	return status;
}

int soc_acpi_gpio_resource_get(int bank_idx, char *hid, char *uid, struct gpio_acpi_res *res,
			       bool ginf)
{
	int ret;
	struct acpi_dev *acpi_child;
	struct acpi_irq_resource irq_res;
	struct acpi_mmio_resource mmio_res;
	uint32_t field_val[8] = {0};
	uint16_t irqs[CONFIG_ACPI_IRQ_VECTOR_MAX];
	struct acpi_reg_base reg_base[CONFIG_ACPI_MMIO_ENTRIES_MAX];
	uint8_t uid_int = 0;

	acpi_child = acpi_device_get(hid, uid);
	if (!acpi_child) {
		LOG_ERR("acpi_device_get failed");
		return -EIO;
	}

	mmio_res.mmio_max = ARRAY_SIZE(reg_base);
	mmio_res.reg_base = reg_base;
	ret = acpi_device_mmio_get(acpi_child, &mmio_res);
	if (ret) {
		LOG_ERR("acpi_device_mmio_get failed");
		return ret;
	}

	irq_res.irq_vector_max = ARRAY_SIZE(irqs);
	irq_res.irqs = irqs;
	ret = acpi_device_irq_get(acpi_child, &irq_res);
	if (ret) {
		LOG_ERR("acpi_device_irq_get failed");
		return ret;
	}

	res->irq = irq_res.irqs[0];
	res->irq_flags = irq_res.flags;

	if (ACPI_RESOURCE_TYPE_GET(&mmio_res) == ACPI_RES_TYPE_MEM) {
		int num_fields = ARRAY_SIZE(field_val);

		if (uid) {
			uid_int = atoi(uid);
		}

		for (int i = 0; i < num_fields; i++) {
			ret = gpio_info_acpi_get(uid_int, bank_idx, i, &field_val[i], ginf);
			if (ret) {
				LOG_ERR("gpio_info_acpi_get failed");
				return ret;
			}
		}

		if (ginf) {
			/* GINF method used to fetch GPIO data from ACPI table requires 3
			 * parameters for each GPIO group.
			 * In this method Reg_base stores 64-bit physical address of the GPIO group.
			 */
			res->reg_base = ACPI_MMIO_GET(&mmio_res);
			res->len = ACPI_RESOURCE_SIZE_GET(&mmio_res);
			res->num_pins = field_val[0];
			res->pad_base = field_val[1];
			res->host_owner_reg = field_val[2];
			res->pad_owner_reg = field_val[3];
			res->gp_evt_stat_reg = field_val[4];
			res->base_num = field_val[7];
		} else {
			/* GINF method used to fetch GPIO data from ACPI table requires 2
			 * parameters for each GPIO group.
			 * In this method reg_base stores 32-bit physical address of the GPIO group
			 */
			res->reg_base = ACPI_MMIO_GET(&mmio_res) & (~0x0FFFFFF);
			res->reg_base += field_val[0];
			res->len = ACPI_RESOURCE_SIZE_GET(&mmio_res);
			res->num_pins = field_val[1];
			res->pad_base = field_val[2];
			res->host_owner_reg = field_val[3];
			res->pad_owner_reg = field_val[4];
			res->gp_evt_stat_reg = field_val[5];
			res->base_num = field_val[6];
		}
	} else {
		LOG_ERR("ACPI_RES_TYPE_MEM failed");
		return -ENODEV;
	}

	return 0;
}
