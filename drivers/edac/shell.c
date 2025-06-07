/*
 * Copyright (c) 2020-2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/device.h>
#include <zephyr/shell/shell.h>

#include <zephyr/drivers/edac.h>

static const struct device *const edac_device = DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_edac));

/**
 * EDAC Error Injection interface
 *
 * edac inject param1 [value]             Show / Set EDAC injection parameter 1
 * edac inject param2 [value]             Show / Set EDAC injection parameter 2
 * edac inject error_type                 Show / Set EDAC error type
 * edac inject trigger                    Trigger injection
 * *
 * edac disable_nmi                       Experimental disable NMI (X86 only)
 * edac enable_nmi                        Experimental enable NMI (X86 only)
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

#ifdef CONFIG_EDAC_IBECC

#include "ibecc.h"

static void decode_ibecc_error(const struct shell *sh, uint64_t ecc_error)
{
	uint64_t erradd = ECC_ERROR_ERRADD(ecc_error);
	unsigned long errsynd = ECC_ERROR_ERRSYND(ecc_error);

	shell_fprintf(sh, SHELL_NORMAL,
		      "CMI Error address: 0x%llx\n", erradd);
	shell_fprintf(sh, SHELL_NORMAL,
		      "Error Syndrome: 0x%lx\n", errsynd);

	if (ecc_error & ECC_ERROR_MERRSTS) {
		shell_fprintf(sh, SHELL_NORMAL,
			      "Uncorrectable Error (UE)\n");
	}

	if (ecc_error & ECC_ERROR_CERRSTS) {
		shell_fprintf(sh, SHELL_NORMAL,
			      "Correctable Error (CE)\n");
	}
}

#endif /* CONFIG_EDAC_IBECC */

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

#ifdef CONFIG_EDAC_IBECC
	if (error != 0) {
		decode_ibecc_error(sh, error);
	}
#endif /* CONFIG_EDAC_IBECC */

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

static int cmd_edac_info(const struct shell *sh, size_t argc, char **argv)
{
	if (!device_is_ready(edac_device)) {
		shell_error(sh, "EDAC device not ready");
		return -ENODEV;
	}

	shell_fprintf(sh, SHELL_NORMAL, "Show EDAC status\n");

	(void)ecc_error_show(sh, edac_device);

	(void)parity_error_show(sh, edac_device);

	shell_fprintf(sh, SHELL_NORMAL, "Errors correctable: %d Errors uncorrectable: %d\n",
		      edac_errors_cor_get(edac_device), edac_errors_uc_get(edac_device));

	return 0;
}

#if defined(CONFIG_EDAC_ERROR_INJECT)
static int cmd_inject_param1(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	if (!device_is_ready(edac_device)) {
		shell_error(sh, "EDAC device not ready");
		return -ENODEV;
	}

	if (argc > 2) {
		/* Usage */
		shell_fprintf(sh, SHELL_NORMAL, "Usage: edac inject %s [val]\n", argv[0]);
		return -ENOTSUP;
	}

	if (argc == 1) {
		uint64_t value;

		err = edac_inject_get_param1(edac_device, &value);
		if (err != 0) {
			shell_error(sh, "Error getting param1 (err %d)", err);
			return err;
		}

		shell_fprintf(sh, SHELL_NORMAL, "Injection param1: 0x%llx\n", value);
	} else {
		unsigned long value = strtoul(argv[1], NULL, 16);

		shell_fprintf(sh, SHELL_NORMAL, "Set injection param1 to: %s\n", argv[1]);

		err = edac_inject_set_param1(edac_device, value);
		if (err != 0) {
			shell_error(sh, "Error setting param1 (err %d)", err);
			return err;
		}
	}

	return err;
}

static int cmd_inject_param2(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	if (!device_is_ready(edac_device)) {
		shell_error(sh, "EDAC device not ready");
		return -ENODEV;
	}

	if (argc > 2) {
		/* Usage */
		shell_fprintf(sh, SHELL_NORMAL, "Usage: edac inject %s [val]\n", argv[0]);
		return -ENOTSUP;
	}

	if (argc == 1) {
		uint64_t value;

		err = edac_inject_get_param2(edac_device, &value);
		if (err != 0) {
			shell_error(sh, "Error getting param2 (err %d)", err);
			return err;
		}

		shell_fprintf(sh, SHELL_NORMAL, "Injection param2: 0x%llx\n", value);
	} else {
		uint64_t value = strtoul(argv[1], NULL, 16);

		shell_fprintf(sh, SHELL_NORMAL, "Set injection param2 to %llx\n", value);

		err = edac_inject_set_param2(edac_device, value);
		if (err != 0) {
			shell_error(sh, "Error setting param2 (err %d)", err);
			return err;
		}
	}

	return err;
}

static int cmd_inject_trigger(const struct shell *sh, size_t argc,
			      char **argv)
{
	if (!device_is_ready(edac_device)) {
		shell_error(sh, "EDAC device not ready");
		return -ENODEV;
	}

	shell_fprintf(sh, SHELL_NORMAL, "Triggering injection\n");

	edac_inject_error_trigger(edac_device);

	return 0;
}

#ifdef CONFIG_X86

static int cmd_inject_disable_nmi(const struct shell *sh, size_t argc,
				  char **argv)
{
	sys_out8((sys_in8(0x70) | 0x80), 0x70);

	return 0;
}

static int cmd_inject_enable_nmi(const struct shell *sh, size_t argc,
				 char **argv)
{
	sys_out8((sys_in8(0x70) & 0x7F), 0x70);

	return 0;
}

#endif /* CONFIG_X86 */

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

static int cmd_inject_error_type_show(const struct shell *sh, size_t argc,
				      char **argv)
{
	uint32_t error_type;
	int err;

	if (!device_is_ready(edac_device)) {
		shell_error(sh, "EDAC device not ready");
		return -ENODEV;
	}

	err = edac_inject_get_error_type(edac_device, &error_type);
	if (err != 0) {
		shell_error(sh, "Error getting error type (err %d)", err);
		return err;
	}

	shell_fprintf(sh, SHELL_NORMAL, "Injection error type: %s\n",
		      get_error_type(error_type));

	return err;
}

static int set_error_type(const struct shell *sh, uint32_t error_type)
{
	if (!device_is_ready(edac_device)) {
		shell_error(sh, "EDAC device not ready");
		return -ENODEV;
	}

	shell_fprintf(sh, SHELL_NORMAL, "Set injection error type: %s\n",
		      get_error_type(error_type));

	return edac_inject_set_error_type(edac_device, error_type);
}

static int cmd_inject_error_type_cor(const struct shell *sh, size_t argc,
				     char **argv)
{
	return set_error_type(sh, EDAC_ERROR_TYPE_DRAM_COR);
}

static int cmd_inject_error_type_uc(const struct shell *sh, size_t argc,
				    char **argv)
{
	return set_error_type(sh, EDAC_ERROR_TYPE_DRAM_UC);
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_inject_error_type_cmds,
	SHELL_CMD(correctable, NULL, "Set correctable error type",
		  cmd_inject_error_type_cor),
	SHELL_CMD(uncorrectable, NULL, "Set uncorrectable error type",
		  cmd_inject_error_type_uc),
	SHELL_SUBCMD_SET_END /* Array terminated */
);

/* EDAC Error Injection shell commands */
SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_inject_cmds, SHELL_CMD(param1, NULL, "Get / Set injection param 1", cmd_inject_param1),
	SHELL_CMD(param2, NULL, "Get / Set injection param 2", cmd_inject_param2),
	SHELL_CMD_ARG(trigger, NULL, "Trigger injection", cmd_inject_trigger, 1, 0),
	SHELL_CMD(error_type, &sub_inject_error_type_cmds, "Get / Set injection error type",
		  cmd_inject_error_type_show),
#ifdef CONFIG_X86
	SHELL_CMD(disable_nmi, NULL, "Disable NMI", cmd_inject_disable_nmi),
	SHELL_CMD(enable_nmi, NULL, "Enable NMI", cmd_inject_enable_nmi),
#endif                       /* CONFIG_X86 */
	SHELL_SUBCMD_SET_END /* Array terminated */
);
#endif /* CONFIG_EDAC_ERROR_INJECT */

static int cmd_ecc_error_show(const struct shell *sh, size_t argc,
			      char **argv)
{
	if (!device_is_ready(edac_device)) {
		shell_error(sh, "EDAC device not ready");
		return -ENODEV;
	}

	return ecc_error_show(sh, edac_device);
}

static int cmd_ecc_error_clear(const struct shell *sh, size_t argc,
			       char **argv)
{
	int err;

	if (!device_is_ready(edac_device)) {
		shell_error(sh, "EDAC device not ready");
		return -ENODEV;
	}

	err = edac_ecc_error_log_clear(edac_device);
	if (err != 0) {
		shell_error(sh, "Error clear ecc error log (err %d)",
			    err);
		return err;
	}

	shell_fprintf(sh, SHELL_NORMAL, "ECC Error Log cleared\n");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_ecc_error_cmds,
	SHELL_CMD(show, NULL, "Show ECC errors", cmd_ecc_error_show),
	SHELL_CMD(clear, NULL, "Clear ECC errors", cmd_ecc_error_clear),
	SHELL_SUBCMD_SET_END /* Array terminated */
);

static int cmd_parity_error_show(const struct shell *sh, size_t argc,
				 char **argv)
{
	if (!device_is_ready(edac_device)) {
		shell_error(sh, "EDAC device not ready");
		return -ENODEV;
	}

	return parity_error_show(sh, edac_device);
}

static int cmd_parity_error_clear(const struct shell *sh, size_t argc,
				  char **argv)
{
	int err;

	if (!device_is_ready(edac_device)) {
		shell_error(sh, "EDAC device not ready");
		return -ENODEV;
	}

	err = edac_parity_error_log_clear(edac_device);
	if (err != 0) {
		shell_error(sh, "Error clear parity error log (err %d)",
			    err);
		return err;
	}

	shell_fprintf(sh, SHELL_NORMAL, "Parity Error Log cleared\n");

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
