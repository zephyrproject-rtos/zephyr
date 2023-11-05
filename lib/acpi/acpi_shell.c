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
		case ACPI_RESOURCE_TYPE_EXTENDED_IRQ:
			shell_print(sh, "ACPI_RESOURCE_TYPE_EXTENDED_IRQ");
			break;
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
		shell_error(sh, "invalid arugment");
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
		shell_error(sh, "invalid arugment");
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
		static ACPI_PCI_ROUTING_TABLE irq_prt_table[CONFIG_ACPI_MAX_PRT_ENTRY];
		int status, cnt;
		ACPI_PCI_ROUTING_TABLE *prt;

		if (argc < 2) {
			shell_error(sh, "invalid arugment");
			return -EINVAL;
		}

		status = acpi_get_irq_routing_table(argv[1],
						    irq_prt_table, ARRAY_SIZE(irq_prt_table));
		if (status) {
			return status;
		}

		prt = irq_prt_table;
		for (cnt = 0; prt->Length; cnt++) {
			shell_print(sh, "[%02X] PCI IRQ Routing Table Package", cnt);
			shell_print(sh, "\tDevNum: %lld Pin: %d IRQ: %d",
				    (prt->Address >> 16) & 0xFFFF, prt->Pin, prt->SourceIndex);

			prt = ACPI_ADD_PTR(ACPI_PCI_ROUTING_TABLE, prt, prt->Length);
		}
	})); /* IF_ENABLED(CONFIG_PCIE_PRT) */

	return 0;
}

static int enum_dev(const struct shell *sh, size_t argc, char **argv)
{
	struct acpi_dev *dev;

	if (argc < 2) {
		return -EINVAL;
	}

	dev = acpi_device_get(argv[1], 0);
	if (!dev || !dev->res_lst) {
		shell_error(sh, "acpi get device failed for HID: %s", argv[1]);
		return -EIO;
	}

	shell_print(sh, "Name: %s", dev->path ? dev->path : "None");
	dump_dev_res(sh, dev->res_lst);

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
	SHELL_CMD(rd_table, NULL,
		  "read acpi table (for read mad table, eg:acpi read_table APIC)",
		  read_table),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(acpi, &sub_acpi, "Demo commands", NULL);
