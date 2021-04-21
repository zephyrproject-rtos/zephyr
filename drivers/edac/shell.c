/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <device.h>
#include <shell/shell.h>

#include <drivers/edac.h>
#include "ibecc.h"

#define DEVICE_NAME DT_LABEL(DT_NODELABEL(ibecc))

/**
 * EDAC Error Injection interface
 *
 * edac inject addr [value]               Physical memory address base
 * edac inject mask [value]               Physical memory address mask
 * edac inject error_type                 Show / Set EDAC error type
 * edac inject trigger                    Trigger injection
 *
 * edac inject test_default               Set default injection parameters
 *
 * edac disable_nmi                       Experimental disable NMI
 * edac enable_nmi                        Experimental enable NMI
 *
 * EDAC Report interface
 *
 * edac info                              Show EDAC ECC / Parity error info
 * edac info ecc_error [show|clear]       Show ECC Errors
 * edac info parity_error [show|clear]    Show Parity Errors
 *
 * Physical memory access interface
 *
 * edac mem address [width [value]]       Physical memory read / write
 */

static void decode_ecc_error(const struct shell *shell, uint64_t ecc_error)
{
	uint64_t erradd = ECC_ERROR_ERRADD(ecc_error);
	unsigned long errsynd = ECC_ERROR_ERRSYND(ecc_error);

	shell_fprintf(shell, SHELL_NORMAL,
		      "CMI Error address: 0x%llx\n", erradd);
	shell_fprintf(shell, SHELL_NORMAL,
		      "Error Syndrome: 0x%lx\n", errsynd);

	if (ecc_error & ECC_ERROR_MERRSTS) {
		shell_fprintf(shell, SHELL_NORMAL,
			      "Uncorrectable Error (UE)\n");
	}

	if (ecc_error & ECC_ERROR_CERRSTS) {
		shell_fprintf(shell, SHELL_NORMAL,
			      "Correctable Error (CE)\n");
	}
}

static int cmd_edac_info(const struct shell *shell, size_t argc, char **argv)
{
	const struct device *dev;
	uint64_t error;

	dev = device_get_binding(DEVICE_NAME);
	if (!dev) {
		shell_error(shell, "IBECC device not found");
		return -ENODEV;
	}

	shell_fprintf(shell, SHELL_NORMAL, "Show EDAC status\n");

	error = edac_ecc_error_log_get(dev);
	shell_fprintf(shell, SHELL_NORMAL, "ECC Error Log 0x%llx\n", error);

	if (error) {
		decode_ecc_error(shell, error);
	}

	error = edac_parity_error_log_get(dev);
	shell_fprintf(shell, SHELL_NORMAL, "Parity Error Log 0x%llx\n", error);

	shell_fprintf(shell, SHELL_NORMAL,
		      "Errors correctable: %d Errors uncorrectable %d\n",
		      edac_errors_cor_get(dev), edac_errors_uc_get(dev));

	return 0;
}

#if defined(CONFIG_EDAC_ERROR_INJECT)
static int cmd_inject_addr(const struct shell *shell, size_t argc, char **argv)
{
	const struct device *dev;

	dev = device_get_binding(DEVICE_NAME);
	if (!dev) {
		shell_error(shell, "IBECC device not found");
		return -ENODEV;
	}

	if (argc > 2) {
		/* Usage */
		shell_fprintf(shell, SHELL_NORMAL,
			      "Usage: edac inject %s [addr]\n", argv[0]);
		return -ENOTSUP;
	}

	if (argc == 1) {
		uint64_t addr = edac_inject_get_param1(dev);

		shell_fprintf(shell, SHELL_NORMAL,
			      "Injection address base: 0x%lx\n", addr);
	} else {
		unsigned long value = strtoul(argv[1], NULL, 16);

		shell_fprintf(shell, SHELL_NORMAL,
			      "Set injection address base to: %s\n", argv[1]);
		edac_inject_set_param1(dev, value);
	}

	return 0;
}

static int cmd_inject_mask(const struct shell *shell, size_t argc, char **argv)
{
	const struct device *dev;

	dev = device_get_binding(DEVICE_NAME);
	if (!dev) {
		shell_error(shell, "IBECC device not found");
		return -ENODEV;
	}

	if (argc > 2) {
		/* Usage */
		shell_fprintf(shell, SHELL_NORMAL,
			      "Usage: edac inject %s [mask]\n", argv[0]);
		return -ENOTSUP;
	}

	if (argc == 1) {
		uint64_t mask = edac_inject_get_param2(dev);

		shell_fprintf(shell, SHELL_NORMAL,
			      "Injection address mask: 0x%lx\n", mask);
	} else {
		uint64_t value = strtoul(argv[1], NULL, 16);

		shell_fprintf(shell, SHELL_NORMAL,
			      "Set injection address mask to %lx\n", value);

		return edac_inject_set_param2(dev, value);
	}

	return 0;
}

static int cmd_inject_trigger(const struct shell *shell, size_t argc,
			      char **argv)
{
	const struct device *dev;

	dev = device_get_binding(DEVICE_NAME);
	if (!dev) {
		shell_error(shell, "IBECC device not found");
		return -ENODEV;
	}

	shell_fprintf(shell, SHELL_NORMAL, "Triggering injection\n");

	edac_inject_error_trigger(dev);

	return 0;
}

static int cmd_inject_disable_nmi(const struct shell *shell, size_t argc,
				  char **argv)
{
	sys_out8((sys_in8(0x70) | 0x80), 0x70);

	return 0;
}

static int cmd_inject_enable_nmi(const struct shell *shell, size_t argc,
				 char **argv)
{
	sys_out8((sys_in8(0x70) & 0x7F), 0x70);

	return 0;
}

static const char *get_error_type(uint32_t type)
{
	switch (type) {
	case EDAC_ERROR_TYPE_DRAM_COR:
		return "correctable";
	case EDAC_ERROR_TYPE_DRAM_UC:
		return "uncorrectable";
	default:
		return "unknown";
	}
}

static int cmd_inject_error_type_show(const struct shell *shell, size_t argc,
				      char **argv)
{
	const struct device *dev;
	uint32_t error_type;

	dev = device_get_binding(DEVICE_NAME);
	if (!dev) {
		shell_error(shell, "IBECC device not found");
		return -ENODEV;
	}

	error_type = edac_inject_get_error_type(dev);

	shell_fprintf(shell, SHELL_NORMAL, "Injection error type: %s\n",
		      get_error_type(error_type));

	return 0;
}

static int set_error_type(const struct shell *shell, uint32_t error_type)
{
	const struct device *dev;

	dev = device_get_binding(DEVICE_NAME);
	if (!dev) {
		shell_error(shell, "IBECC device not found");
		return -ENODEV;
	}

	shell_fprintf(shell, SHELL_NORMAL, "Set injection error type: %s\n",
		      get_error_type(error_type));

	return edac_inject_set_error_type(dev, error_type);
}

static int cmd_inject_error_type_cor(const struct shell *shell, size_t argc,
				     char **argv)
{
	return set_error_type(shell, EDAC_ERROR_TYPE_DRAM_COR);
}

static int cmd_inject_error_type_uc(const struct shell *shell, size_t argc,
				    char **argv)
{
	return set_error_type(shell, EDAC_ERROR_TYPE_DRAM_UC);
}

static int cmd_inject_test(const struct shell *shell, size_t argc, char **argv)
{
	const struct device *dev;

	dev = device_get_binding(DEVICE_NAME);
	if (!dev) {
		shell_error(shell, "IBECC device not found");
		return -ENODEV;
	}
	edac_inject_set_param1(dev, 0x1000);
	edac_inject_set_param2(dev, INJ_ADDR_BASE_MASK_MASK);
	edac_inject_set_error_type(dev, EDAC_ERROR_TYPE_DRAM_COR);
	edac_inject_error_trigger(dev);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_inject_error_type_cmds,
	SHELL_CMD(correctable, NULL, "Set correctable error type",
		  cmd_inject_error_type_cor),
	SHELL_CMD(uncorrectable, NULL, "Set uncorrectable error type",
		  cmd_inject_error_type_uc),
	SHELL_SUBCMD_SET_END /* Array terminated */
);

/* EDAC Error Injection shell commands */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_inject_cmds,
	SHELL_CMD(addr, NULL, "Get / Set physical address", cmd_inject_addr),
	SHELL_CMD(mask, NULL, "Get / Set address mask", cmd_inject_mask),
	SHELL_CMD_ARG(trigger, NULL, "Trigger injection", cmd_inject_trigger,
		      1, 0),
	SHELL_CMD(error_type, &sub_inject_error_type_cmds,
		  "Get / Set injection error type",
		  cmd_inject_error_type_show),
	SHELL_CMD(disable_nmi, NULL, "Disable NMI", cmd_inject_disable_nmi),
	SHELL_CMD(enable_nmi, NULL, "Enable NMI", cmd_inject_enable_nmi),
	SHELL_CMD_ARG(test_default, NULL, "Test default injection parameters",
		      cmd_inject_test, 1, 0),
	SHELL_SUBCMD_SET_END /* Array terminated */
);
#endif /* CONFIG_EDAC_ERROR_INJECT */

static int cmd_ecc_error_show(const struct shell *shell, size_t argc,
			      char **argv)
{
	const struct device *dev;
	uint64_t error;

	dev = device_get_binding(DEVICE_NAME);
	if (!dev) {
		shell_error(shell, "IBECC device not found");
		return -ENODEV;
	}

	error = edac_ecc_error_log_get(dev);
	shell_fprintf(shell, SHELL_NORMAL, "ECC Error: 0x%lx\n", error);

	if (error) {
		decode_ecc_error(shell, error);
	}

	return 0;
}

static int cmd_ecc_error_clear(const struct shell *shell, size_t argc,
			       char **argv)
{
	const struct device *dev;

	dev = device_get_binding(DEVICE_NAME);
	if (!dev) {
		shell_error(shell, "IBECC device not found");
		return -ENODEV;
	}

	edac_ecc_error_log_clear(dev);

	shell_fprintf(shell, SHELL_NORMAL, "ECC Error Log cleared\n");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_ecc_error_cmds,
	SHELL_CMD(show, NULL, "Show ECC errors", cmd_ecc_error_show),
	SHELL_CMD(clear, NULL, "Clear ECC errors", cmd_ecc_error_clear),
	SHELL_SUBCMD_SET_END /* Array terminated */
);

static int cmd_parity_error_show(const struct shell *shell, size_t argc,
				 char **argv)
{
	const struct device *dev;
	uint64_t error;

	dev = device_get_binding(DEVICE_NAME);
	if (!dev) {
		shell_error(shell, "IBECC device not found");
		return -ENODEV;
	}

	error = edac_parity_error_log_get(dev);
	shell_fprintf(shell, SHELL_NORMAL, "Parity Error: 0x%lx\n", error);

	return 0;
}

static int cmd_parity_error_clear(const struct shell *shell, size_t argc,
				  char **argv)
{
	const struct device *dev;

	dev = device_get_binding(DEVICE_NAME);
	if (!dev) {
		shell_error(shell, "IBECC device not found");
		return -ENODEV;
	}

	edac_parity_error_log_clear(dev);

	shell_fprintf(shell, SHELL_NORMAL, "Parity Error Log cleared\n");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_parity_error_cmds,
	SHELL_CMD(show, NULL, "Show Parity errors", cmd_parity_error_show),
	SHELL_CMD(clear, NULL, "Clear Parity errors", cmd_parity_error_clear),
	SHELL_SUBCMD_SET_END /* Array terminated */
);

/* EDAC Info shell commands */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_info_cmds,
	SHELL_CMD(ecc_error, &sub_ecc_error_cmds,
		  "ECC Error Show / Clear commands", cmd_ecc_error_show),
	SHELL_CMD(parity_error, &sub_parity_error_cmds,
		  "Parity Error Show / Clear commands", cmd_parity_error_show),
	SHELL_SUBCMD_SET_END /* Array terminated */
);

/* Memory operation commands */

static int cmd_mem(const struct shell *shell, size_t argc, char **argv)
{
	uint64_t value = 0;
	mem_addr_t phys_addr, addr;
	uint8_t width;

	if (argc < 2 || argc > 4) {
		return -EINVAL;
	}

	phys_addr = strtol(argv[1], NULL, 16);
	device_map((mm_reg_t *)&addr, phys_addr, 0x100, K_MEM_CACHE_NONE);

	shell_fprintf(shell, SHELL_NORMAL, "Mapped 0x%lx to 0x%lx\n",
		      phys_addr, addr);

	if (argc < 3) {
		width = 32;
	} else {
		width = strtol(argv[2], NULL, 10);
	}

	shell_fprintf(shell, SHELL_NORMAL, "Using data width %d\n", width);

	if (argc <= 3) {
		switch (width) {
		case 8:
			value = sys_read8(addr);
			break;
		case 16:
			value = sys_read16(addr);
			break;
		case 32:
			value = sys_read32(addr);
			break;
		default:
			shell_fprintf(shell, SHELL_NORMAL,
				      "Incorrect data width\n");
			return -EINVAL;
		}

		shell_fprintf(shell, SHELL_NORMAL,
			      "Read value 0x%lx\n", value);
		return 0;
	}

	/* If there are more then 3 arguments, that means we are going to write
	 * this value at the address provided
	 */

	value = strtol(argv[3], NULL, 16);

	shell_fprintf(shell, SHELL_NORMAL, "Writing value 0x%lx\n", value);

	switch (width) {
	case 8:
		sys_write8(value, addr);
		break;
	case 16:
		sys_write16(value, addr);
		break;
	case 32:
		sys_write32(value, addr);
		break;
	default:
		shell_fprintf(shell, SHELL_NORMAL,
			      "Incorrect data width\n");
		return -EINVAL;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_edac_cmds,
	SHELL_CMD(info, &sub_info_cmds,
		  "Show EDAC information\n"
		  "edac info <subcommands>", cmd_edac_info),
	SHELL_CMD(mem, NULL,
		  "Read / Write physical memory\n"
		  "edac mem address [width [value]]", cmd_mem),
#if defined(CONFIG_EDAC_ERROR_INJECT)
	/* This does not work with SHELL_COND_CMD */
	SHELL_CMD(inject, &sub_inject_cmds,
		  "Inject ECC error commands\n"
		  "edac inject <subcommands>", NULL),
#endif
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(edac, &sub_edac_cmds, "EDAC information", cmd_edac_info);
