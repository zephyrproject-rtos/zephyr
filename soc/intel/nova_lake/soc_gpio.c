/*
 * Copyright (c) 2026 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Nova Lake GPIO SoC support.
 *
 * On NVL-S the BIOS exposes a GPIOv2-style \_SB.GPVD.GINF method whose
 * first argument is a pad-encoded value:
 *
 *   bits [21:17] = chiplet ID  (0x0E = SoC/PCD, 0x0F = PCH)
 *   bits [12:10] = community index inside the chiplet's GPCS table
 *
 * Two families of GPIO ACPI devices exist:
 *
 *   SoC (PCD) : HID "INTC10DD"  - communities 0, 1, 3
 *               (community 2 is absent; indices are 0, 1, 2 in GPCS)
 *   PCH       : HID "INTC10ED"  - communities 0 – 5  (no gaps)
 */

#include <errno.h>
#include <zephyr/acpi/acpi.h>
#include "soc.h"
#include "soc_gpio.h"
#include "../common/soc_gpio.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(soc_gpio, CONFIG_SOC_LOG_LEVEL);

/* NVL-S: GINF lives under \_SB.GPVD, not \_SB  */
#define GET_GPIO_BASE_NUM ("\\_SB.GPVD.GINF")

/* Chiplet IDs used in the GPIOv2 pad encoding */
#define PCD_CHIPLET_ID  0x0E   /* SoC / PCD */
#define PCH_CHIPLET_ID  0x0F   /* PCH        */

/* SoC community 2 is missing; UIDs > 2 are shifted down by 1 in the table */
#define GINF_MISSING_UID_PCD  2

/* HID strings */
#define HID_SOC_GPIO  "INTC10DD"
#define HID_PCH_GPIO  "INTC10ED"

/**
 * Build the GPIOv2 pad-encoded value that \_SB.GPVD.GINF expects as Arg0.
 *
 * @param hid   ACPI Hardware ID ("INTC10DD" or "INTC10ED")
 * @param uid   Community UID from the device-tree
 * @return      Encoded pad_id suitable for GINF(Arg0, ...)
 */
static uint32_t ginf_pad_id(const char *hid, uint8_t uid)
{
	uint32_t chiplet;
	uint32_t community_idx;

	if (!strcmp(hid, HID_SOC_GPIO)) {
		chiplet = PCD_CHIPLET_ID;
		/* Adjust for the missing community 2 in the PCD GPCS table */
		community_idx = (uid > GINF_MISSING_UID_PCD) ? (uid - 1) : uid;
	} else {
		chiplet = PCH_CHIPLET_ID;
		community_idx = uid;   /* PCH has no gaps */
	}

	return (chiplet << 17) | (community_idx << 10);
}

static int gpio_info_acpi_get(const char *hid, uint8_t uid, uint32_t bank_idx,
			      uint32_t field_idx, uint32_t *ret_val)
{
	int status;
	ACPI_OBJECT args[3];
	ACPI_OBJECT_LIST arg_list;
	ACPI_OBJECT gpio_obj;

	arg_list.Count = 3;
	arg_list.Pointer = args;

	args[0].Type = ACPI_TYPE_INTEGER;
	args[0].Integer.Value = ginf_pad_id(hid, uid);

	args[1].Type = ACPI_TYPE_INTEGER;
	args[1].Integer.Value = bank_idx;

	args[2].Type = ACPI_TYPE_INTEGER;
	args[2].Integer.Value = field_idx;

	status = acpi_invoke_method(GET_GPIO_BASE_NUM, &arg_list, &gpio_obj);
	if (status) {
		LOG_ERR("GINF(%08x, %u, %u) failed: %d",
			(uint32_t)args[0].Integer.Value, bank_idx, field_idx,
			status);
		return status;
	}

	if (gpio_obj.Type == ACPI_TYPE_INTEGER) {
		*ret_val = gpio_obj.Integer.Value;
	}

	return 0;
}

int soc_acpi_gpio_resource_get(int bank_idx, char *hid, char *uid,
			       struct gpio_acpi_res *res, bool ginf)
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
		LOG_ERR("acpi_device_get(%s, %s) failed", hid, uid);
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

	if (ACPI_RESOURCE_TYPE_GET(&mmio_res) != ACPI_RES_TYPE_MEM) {
		LOG_ERR("ACPI_RES_TYPE_MEM not found");
		return -ENODEV;
	}

	if (uid) {
		uid_int = atoi(uid);
	}

	/* NVL always uses the 3-parameter GINF variant */
	for (int i = 0; i < (int)ARRAY_SIZE(field_val); i++) {
		ret = gpio_info_acpi_get(hid, uid_int, bank_idx, i,
					 &field_val[i]);
		if (ret) {
			LOG_ERR("gpio_info_acpi_get field %d failed", i);
			return ret;
		}
	}

	res->reg_base = ACPI_MMIO_GET(&mmio_res);
	res->len = ACPI_RESOURCE_SIZE_GET(&mmio_res);
	res->num_pins = field_val[0];
	res->pad_base = field_val[1];
	res->host_owner_reg = field_val[2];
	res->pad_owner_reg = field_val[3];
	res->gp_evt_stat_reg = field_val[4];
	res->base_num = field_val[7];

	return 0;
}
