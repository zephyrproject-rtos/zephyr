/*
 * Copyright (c) 2020-2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/device.h>
#include <zephyr/shell/shell.h>

#include <zephyr/drivers/edac.h>
#include "ibecc.h"

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
 * Physical memory access interface using devmem shell module
 *
 * devmem [width [value]]       Physical memory read / write
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

static int ecc_error_show(const struct shell *sh, const struct device *dev)
{
	uint64_t error;
	int err;

	err = edac_ecc_error_log_get(dev, &error);
	if (err != 0 && err != -ENODATA) {
		shell_error(sh, "Error getting error log (err %d)", err);
		return err;
	}

	shell_fprintf(sh, SHELL_NORMAL, "ECC Error: 0x%llx\n", error);

	if (error != 0) {
		decode_ecc_error(sh, error);
	}

	return 0;
}

static int parity_error_show(const struct shell *sh, const struct device *dev)
{
	uint64_t error;
	int err;

	err = edac_parity_error_log_get(dev, &error);
	if (err != 0 && err != -ENODATA) {
		shell_error(sh, "Error getting parity error log (err %d)", err);
		return err;
	}

	shell_fprintf(sh, SHELL_NORMAL, "Parity Error: 0x%llx\n", error);

	return 0;
}

static int cmd_edac_info(const struct shell *shell, size_t argc, char **argv)
{
	const struct device *dev;
	int err;

	dev = DEVICE_DT_GET(DT_NODELABEL(ibecc));
	if (!device_is_ready(dev)) {
		shell_error(shell, "IBECC device not ready");
		return -ENODEV;
	}

	shell_fprintf(shell, SHELL_NORMAL, "Show EDAC status\n");

	err = ecc_error_show(shell, dev);
	if (err != 0) {
		return err;
	}

	err = parity_error_show(shell, dev);
	if (err != 0) {
		return err;
	}

	shell_fprintf(shell, SHELL_NORMAL,
		      "Errors correctable: %d Errors uncorrectable %d\n",
		      edac_errors_cor_get(dev), edac_errors_uc_get(dev));

	return err;
}

#if defined(CONFIG_EDAC_ERROR_INJECT)
static int cmd_inject_addr(const struct shell *shell, size_t argc, char **argv)
{
	const struct device *dev;
	int err;

	dev = DEVICE_DT_GET(DT_NODELABEL(ibecc));
	if (!device_is_ready(dev)) {
		shell_error(shell, "IBECC device not ready");
		return -ENODEV;
	}

	if (argc > 2) {
		/* Usage */
		shell_fprintf(shell, SHELL_NORMAL,
			      "Usage: edac inject %s [addr]\n", argv[0]);
		return -ENOTSUP;
	}

	if (argc == 1) {
		uint64_t addr;

		err = edac_inject_get_param1(dev, &addr);
		if (err != 0) {
			shell_error(shell, "Error getting address (err %d)",
				    err);
			return err;
		}

		shell_fprintf(shell, SHELL_NORMAL,
			      "Injection address base: 0x%llx\n", addr);
	} else {
		unsigned long value = strtoul(argv[1], NULL, 16);

		shell_fprintf(shell, SHELL_NORMAL,
			      "Set injection address base to: %s\n", argv[1]);

		err = edac_inject_set_param1(dev, value);
		if (err != 0) {
			shell_error(shell, "Error setting address (err %d)",
				    err);
			return err;
		}
	}

	return err;
}

static int cmd_inject_mask(const struct shell *shell, size_t argc, char **argv)
{
	const struct device *dev;
	int err;

	dev = DEVICE_DT_GET(DT_NODELABEL(ibecc));
	if (!device_is_ready(dev)) {
		shell_error(shell, "IBECC device not ready");
		return -ENODEV;
	}

	if (argc > 2) {
		/* Usage */
		shell_fprintf(shell, SHELL_NORMAL,
			      "Usage: edac inject %s [mask]\n", argv[0]);
		return -ENOTSUP;
	}

	if (argc == 1) {
		uint64_t mask;

		err = edac_inject_get_param2(dev, &mask);
		if (err != 0) {
			shell_error(shell, "Error getting mask (err %d)", err);
			return err;
		}

		shell_fprintf(shell, SHELL_NORMAL,
			      "Injection address mask: 0x%llx\n", mask);
	} else {
		uint64_t value = strtoul(argv[1], NULL, 16);

		shell_fprintf(shell, SHELL_NORMAL,
			      "Set injection address mask to %llx\n", value);

		err = edac_inject_set_param2(dev, value);
		if (err != 0) {
			shell_error(shell, "Error setting mask (err %d)", err);
			return err;
		}
	}

	return err;
}

static int cmd_inject_trigger(const struct shell *shell, size_t argc,
			      char **argv)
{
	const struct device *dev;

	dev = DEVICE_DT_GET(DT_NODELABEL(ibecc));
	if (!device_is_ready(dev)) {
		shell_error(shell, "IBECC device not ready");
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
	int err;

	dev = DEVICE_DT_GET(DT_NODELABEL(ibecc));
	if (!device_is_ready(dev)) {
		shell_error(shell, "IBECC device not ready");
		return -ENODEV;
	}

	err = edac_inject_get_error_type(dev, &error_type);
	if (err != 0) {
		shell_error(shell, "Error getting error type (err %d)", err);
		return err;
	}

	shell_fprintf(shell, SHELL_NORMAL, "Injection error type: %s\n",
		      get_error_type(error_type));

	return err;
}

static int set_error_type(const struct shell *shell, uint32_t error_type)
{
	const struct device *dev;

	dev = DEVICE_DT_GET(DT_NODELABEL(ibecc));
	if (!device_is_ready(dev)) {
		shell_error(shell, "IBECC device not ready");
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

	dev = DEVICE_DT_GET(DT_NODELABEL(ibecc));
	if (!device_is_ready(dev)) {
		shell_error(shell, "IBECC device not ready");
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

	dev = DEVICE_DT_GET(DT_NODELABEL(ibecc));
	if (!device_is_ready(dev)) {
		shell_error(shell, "IBECC device not ready");
		return -ENODEV;
	}

	return ecc_error_show(shell, dev);
}

static int cmd_ecc_error_clear(const struct shell *shell, size_t argc,
			       char **argv)
{
	const struct device *dev;
	int err;

	dev = DEVICE_DT_GET(DT_NODELABEL(ibecc));
	if (!device_is_ready(dev)) {
		shell_error(shell, "IBECC device not ready");
		return -ENODEV;
	}

	err = edac_ecc_error_log_clear(dev);
	if (err != 0) {
		shell_error(shell, "Error clear ecc error log (err %d)",
			    err);
		return err;
	}

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

	dev = DEVICE_DT_GET(DT_NODELABEL(ibecc));
	if (!device_is_ready(dev)) {
		shell_error(shell, "IBECC device not ready");
		return -ENODEV;
	}

	return parity_error_show(shell, dev);
}

static int cmd_parity_error_clear(const struct shell *shell, size_t argc,
				  char **argv)
{
	const struct device *dev;
	int err;

	dev = DEVICE_DT_GET(DT_NODELABEL(ibecc));
	if (!device_is_ready(dev)) {
		shell_error(shell, "IBECC device not ready");
		return -ENODEV;
	}

	err = edac_parity_error_log_clear(dev);
	if (err != 0) {
		shell_error(shell, "Error clear parity error log (err %d)",
			    err);
		return err;
	}

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

SHELL_STATIC_SUBCMD_SET_CREATE(sub_edac_cmds,
	SHELL_CMD(info, &sub_info_cmds,
		  "Show EDAC information\n"
		  "edac info <subcommands>", cmd_edac_info),
#if defined(CONFIG_EDAC_ERROR_INJECT)
	/* This does not work with SHELL_COND_CMD */
	SHELL_CMD(inject, &sub_inject_cmds,
		  "Inject ECC error commands\n"
		  "edac inject <subcommands>", NULL),
#endif
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(edac, &sub_edac_cmds, "EDAC information", cmd_edac_info);
