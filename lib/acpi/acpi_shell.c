/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <stdbool.h>
#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/acpi/acpi.h>
#include <zephyr/shell/shell.h>

static void dump_dev_res(const struct shell *sh, ACPI_RESOURCE *res_lst)
{
	ACPI_RESOURCE *res = res_lst;

	shell_print(sh, "**** ACPI Device Resource Info ****");

	do {

		if (!res->Length) {
			shell_error(sh, "Error: zero length found!");
			break;
		}

		switch (res->Type) {
		case ACPI_RESOURCE_TYPE_IRQ:
			shell_print(sh, "ACPI_RESOURCE_TYPE_IRQ");
			ACPI_RESOURCE_IRQ *irq_res = &res->Data.Irq;

			shell_print(sh, "\tDescriptorLength: %x", irq_res->DescriptorLength);
			shell_print(sh, "\tTriggering: %x", irq_res->Triggering);
			shell_print(sh, "\tPolarity: %x", irq_res->Polarity);
			shell_print(sh, "\tShareable: %x", irq_res->Shareable);
			shell_print(sh, "\tInterruptCount: %d", irq_res->InterruptCount);
			shell_print(sh, "\tInterrupts[0]: %x", irq_res->Interrupts[0]);
			break;
		case ACPI_RESOURCE_TYPE_IO: {
			ACPI_RESOURCE_IO *io_res = &res->Data.Io;

			shell_print(sh, "ACPI_RESOURCE_TYPE_IO");
			shell_print(sh, "\tIoDecode: %x", io_res->IoDecode);
			shell_print(sh, "\tAlignment: %x", io_res->Alignment);
			shell_print(sh, "\tAddressLength: %x", io_res->AddressLength);
			shell_print(sh, "\tMinimum: %x", io_res->Minimum);
			shell_print(sh, "\tMaximum: %x", io_res->Maximum);
			break;
		}
		case ACPI_RESOURCE_TYPE_DMA:
			shell_print(sh, "ACPI_RESOURCE_TYPE_DMA");
			break;
		case ACPI_RESOURCE_TYPE_START_DEPENDENT:
			shell_print(sh, "ACPI_RESOURCE_TYPE_START_DEPENDENT");
			break;
		case ACPI_RESOURCE_TYPE_END_DEPENDENT:
			shell_print(sh, "ACPI_RESOURCE_TYPE_END_DEPENDENT");
			break;
		case ACPI_RESOURCE_TYPE_FIXED_IO:
			shell_print(sh, "ACPI_RESOURCE_TYPE_FIXED_IO");
			break;
		case ACPI_RESOURCE_TYPE_VENDOR:
			shell_print(sh, "ACPI_RESOURCE_TYPE_VENDOR");
			break;
		case ACPI_RESOURCE_TYPE_MEMORY24:
			shell_print(sh, "ACPI_RESOURCE_TYPE_MEMORY24");
			break;
		case ACPI_RESOURCE_TYPE_MEMORY32: {
			ACPI_RESOURCE_MEMORY32 *mem_res = &res->Data.Memory32;

			shell_print(sh, "ACPI_RESOURCE_TYPE_MEMORY32");
			shell_print(sh, "\tMinimum: %x", mem_res->Minimum);
			shell_print(sh, "\tMaximum: %x", mem_res->Maximum);
			break;
		}
		case ACPI_RESOURCE_TYPE_FIXED_MEMORY32: {
			ACPI_RESOURCE_FIXED_MEMORY32 *fix_mem_res = &res->Data.FixedMemory32;

			shell_print(sh, "ACPI_RESOURCE_TYPE_FIXED_MEMORY32");
			shell_print(sh, "\tAddress: %x", fix_mem_res->Address);
			break;
		}
		case ACPI_RESOURCE_TYPE_ADDRESS16:
			shell_print(sh, "ACPI_RESOURCE_TYPE_ADDRESS16");
			break;
		case ACPI_RESOURCE_TYPE_ADDRESS32: {
			ACPI_RESOURCE_ADDRESS32 *add_res = &res->Data.Address32;

			shell_print(sh, "ACPI_RESOURCE_TYPE_ADDRESS32");
			shell_print(sh, "\tMinimum: %x", add_res->Address.Minimum);
			shell_print(sh, "\tMaximum: %x", add_res->Address.Maximum);
			break;
		}
		case ACPI_RESOURCE_TYPE_ADDRESS64: {
			ACPI_RESOURCE_ADDRESS64 *add_res64 = &res->Data.Address64;

			shell_print(sh, "ACPI_RESOURCE_TYPE_ADDRESS64");
			shell_print(sh, "\tMinimum: %llx", add_res64->Address.Minimum);
			shell_print(sh, "\tMaximum: %llx", add_res64->Address.Maximum);
			break;
		}
		case ACPI_RESOURCE_TYPE_EXTENDED_ADDRESS64:
			shell_print(sh, "ACPI_RESOURCE_TYPE_EXTENDED_ADDRESS64");
			break;
		case ACPI_RESOURCE_TYPE_EXTENDED_IRQ: {
			ACPI_RESOURCE_EXTENDED_IRQ *ext_irq_res = &res->Data.ExtendedIrq;

			shell_print(sh, "ACPI_RESOURCE_TYPE_EXTENDED_IRQ");
			shell_print(sh, "\tTriggering: %x", ext_irq_res->Triggering);
			shell_print(sh, "\tPolarity: %x", ext_irq_res->Polarity);
			shell_print(sh, "\tShareable: %s", ext_irq_res->Shareable ? "YES":"NO");
			shell_print(sh, "\tInterruptCount: %d", ext_irq_res->InterruptCount);
			shell_print(sh, "\tInterrupts[0]: %d", ext_irq_res->Interrupts[0]);
			break;
		}
		case ACPI_RESOURCE_TYPE_GENERIC_REGISTER:
			shell_print(sh, "ACPI_RESOURCE_TYPE_GENERIC_REGISTER");
			break;
		case ACPI_RESOURCE_TYPE_GPIO:
			shell_print(sh, "ACPI_RESOURCE_TYPE_GPIO");
			break;
		case ACPI_RESOURCE_TYPE_FIXED_DMA:
			shell_print(sh, "ACPI_RESOURCE_TYPE_FIXED_DMA");
			break;
		case ACPI_RESOURCE_TYPE_SERIAL_BUS:
			shell_print(sh, "ACPI_RESOURCE_TYPE_SERIAL_BUS");
			break;
		case ACPI_RESOURCE_TYPE_PIN_FUNCTION:
			shell_print(sh, "ACPI_RESOURCE_TYPE_PIN_FUNCTION");
			break;
		case ACPI_RESOURCE_TYPE_PIN_CONFIG:
			shell_print(sh, "ACPI_RESOURCE_TYPE_PIN_CONFIG");
			break;
		case ACPI_RESOURCE_TYPE_PIN_GROUP:
			shell_print(sh, "ACPI_RESOURCE_TYPE_PIN_GROUP");
			break;
		case ACPI_RESOURCE_TYPE_PIN_GROUP_FUNCTION:
			shell_print(sh, "ACPI_RESOURCE_TYPE_PIN_GROUP_FUNCTION");
			break;
		case ACPI_RESOURCE_TYPE_PIN_GROUP_CONFIG:
			shell_print(sh, "ACPI_RESOURCE_TYPE_PIN_GROUP_CONFIG");
			break;
		default:
			shell_error(sh, "Unknown resource type %d", res->Type);
		}

		res = ACPI_NEXT_RESOURCE(res);

	} while (res->Type != ACPI_RESOURCE_TYPE_END_TAG);
}

static int dump_dev_crs(const struct shell *sh, size_t argc, char **argv)
{
	int status;
	ACPI_RESOURCE *res_lst;

	if (argc < 2) {
		shell_error(sh, "invalid argument");
		return -EINVAL;
	}

	status = acpi_current_resource_get(argv[1], &res_lst);
	if (status) {
		shell_error(sh, "Error on ACPI _CRS method: %d", status);
		return status;
	}

	dump_dev_res(sh, res_lst);

	acpi_current_resource_free(res_lst);

	return 0;
}

static int dump_dev_prs(const struct shell *sh, size_t argc, char **argv)
{
	int status;
	ACPI_RESOURCE *res_lst;

	if (argc < 2) {
		shell_error(sh, "invalid argument");
		return -EINVAL;
	}

	status = acpi_possible_resource_get(argv[1], &res_lst);
	if (status) {
		shell_error(sh, "Error in on ACPI _PRS method: %d", status);
		return status;
	}

	dump_dev_res(sh, res_lst);

	return 0;
}

static int dump_prt(const struct shell *sh, size_t argc, char **argv)
{
	IF_ENABLED(CONFIG_PCIE_PRT, ({
		int irq, bus, dev_num, func;
		pcie_bdf_t bdf;

		if (argc < 4) {
			shell_error(sh, "invalid arguments [Eg: acpi prt <bus> <dev> <func>]");
			return -EINVAL;
		}

		bus = atoi(argv[1]);
		dev_num = atoi(argv[2]);
		func = atoi(argv[3]);

		bdf = PCIE_BDF(bus, dev_num, func);
		irq = acpi_legacy_irq_get(bdf);
		if (irq != UINT_MAX) {
			shell_print(sh, "PCI Legacy IRQ for bus %d dev %d func %d is: %d",
				 bus, dev_num, func, irq);
		} else {
			shell_print(sh, "PCI Legacy IRQ for bus %d dev %d func %d Not found",
				 bus, dev_num, func);
		}
	})); /* IF_ENABLED(CONFIG_PCIE_PRT) */

	return 0;
}

static int enum_dev(const struct shell *sh, size_t argc, char **argv)
{
	struct acpi_dev *dev;
	ACPI_RESOURCE *res_lst;

	if (argc < 3) {
		shell_error(sh, "Invalid arguments [Eg: acpi enum PNP0103 0]");
		return -EINVAL;
	}

	dev = acpi_device_get(argv[1], argv[2]);
	if (!dev || !dev->res_lst) {
		shell_error(sh, "acpi get device failed for HID: %s", argv[1]);
		return -EIO;
	}

	shell_print(sh, "Name: %s", dev->path ? dev->path : "None");

	if (dev->path) {
		if (!acpi_current_resource_get(dev->path, &res_lst)) {
			dump_dev_res(sh, res_lst);
			acpi_current_resource_free(res_lst);
		}
	}

	return 0;
}

static int enum_all_dev(const struct shell *sh, size_t argc, char **argv)
{
	struct acpi_dev *dev;

	for (int i = 0; i < CONFIG_ACPI_DEV_MAX; i++) {
		dev = acpi_device_by_index_get(i);
		if (!dev) {
			shell_print(sh, "No more ACPI device found!");
			break;
		}

		if (!dev->dev_info) {
			continue;
		}

		shell_print(sh, "%d) Name: %s, HID: %s", i, dev->path ? dev->path : "None",
			    dev->dev_info->HardwareId.String ? dev->dev_info->HardwareId.String
							     : "None");
	}

	return 0;
}

static int get_acpi_dev_resource(const struct shell *sh, size_t argc, char **argv)
{
	struct acpi_dev *dev;
	struct acpi_irq_resource irq_res;
	struct acpi_mmio_resource mmio_res;
	uint16_t irqs[CONFIG_ACPI_IRQ_VECTOR_MAX];
	struct acpi_reg_base reg_base[CONFIG_ACPI_MMIO_ENTRIES_MAX];

	if (argc < 3) {
		return -EINVAL;
	}

	dev = acpi_device_get(argv[1], argv[2]);
	if (!dev) {
		shell_error(sh, "acpi get device failed for HID: %s", argv[1]);
		return -EIO;
	}

	if (dev->path) {
		shell_print(sh, "Device Path: %s", dev->path);

		mmio_res.mmio_max = ARRAY_SIZE(reg_base);
		mmio_res.reg_base = reg_base;
		if (!acpi_device_mmio_get(dev, &mmio_res)) {

			shell_print(sh, "Device MMIO resources");
			for (int i = 0; i < ACPI_RESOURCE_COUNT_GET(&mmio_res); i++) {
				shell_print(sh, "\tType: %x, Address: %p, Size: %d",
					ACPI_MULTI_RESOURCE_TYPE_GET(&mmio_res, i),
					(void *)ACPI_MULTI_MMIO_GET(&mmio_res, i),
						ACPI_MULTI_RESOURCE_SIZE_GET(&mmio_res, i));
			}
		}

		irq_res.irq_vector_max = ARRAY_SIZE(irqs);
		irq_res.irqs = irqs;
		if (!acpi_device_irq_get(dev, &irq_res)) {

			shell_print(sh, "Device IRQ resources");
			for (int i = 0; i < irq_res.irq_vector_max; i++) {
				shell_print(sh, "\tIRQ Num: %x, Flags: %x", irq_res.irqs[i],
					    irq_res.flags);
			}
		}
	}

	return 0;
}

static int read_table(const struct shell *sh, size_t argc, char **argv)
{
	ACPI_TABLE_HEADER *table;

	if (argc < 2) {
		return -EINVAL;
	}

	table = acpi_table_get(argv[1], 0);
	if (!table) {
		shell_error(sh, "ACPI get table %s failed", argv[1]);
		return -EIO;
	}

	shell_print(sh, "ACPI Table %s:", argv[1]);
	shell_print(sh, "\tSignature: %.4s", table->Signature);
	shell_print(sh, "\tTable Length: %d", table->Length);
	shell_print(sh, "\tRevision: %d", table->Revision);
	shell_print(sh, "\tOemId: %s", table->OemId);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_acpi,
	SHELL_CMD(crs, NULL,
		  "display device current resource settings (eg:acpi crs _SB.PC00.LPCB.RTC)",
		  dump_dev_crs),
	SHELL_CMD(prs, NULL,
		  "display device possible resource settings (eg:acpi crs _SB.PC00.LPCB.RTC)",
		  dump_dev_prs),
	SHELL_COND_CMD(CONFIG_PCIE_PRT, prt, NULL,
		       "display PRT details for a given bus (eg:acpi prt _SB.PC00)",
		       dump_prt),
	SHELL_CMD(enum, NULL,
		  "enumerate device using hid (for enum HPET timer device,eg:acpi enum PNP0103)",
		  enum_dev),
	SHELL_CMD(enum_all, NULL, "enumerate all device in acpi name space (eg:acpi enum_all)",
		  enum_all_dev),
	SHELL_CMD(dev_res, NULL, "retrieve device resource (eg: acpi dev_res PNP0501 2)",
		  get_acpi_dev_resource),
	SHELL_CMD(rd_table, NULL, "read ACPI table (eg: acpi read_table APIC)",
		  read_table),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(acpi, &sub_acpi, "Demo commands", NULL);
