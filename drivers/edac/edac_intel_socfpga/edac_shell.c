/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT intel_io96b

#include <stdlib.h>
#include <zephyr/device.h>
#include <zephyr/shell/shell.h>
#include "edac.h"
#include "io96b_ecc.h"
#include "hps_ecc.h"

#if CONFIG_IO96B_INTEL_SOCFPGA

#define MAX_IO96B_INSTANCES          (0x2u)
#define MAX_ECC_MODE_VALUE           (3u)
#define MAX_ECC_TYPE_VALUE           (1u)
#define IO96B_EN_SET_ECC_TYPE_OFFSET (2u)
#define UNUSED_ARG                   (0u)
static int get_io96_device(const struct shell *shell, char **argv, const struct device **dev,
			   int *inst_id)
{
	int err = 0;

	*inst_id = shell_strtoul(argv[1], 10, &err);
	*dev = NULL;

	if (err != 0) {
		shell_error(shell, "Invalid IO96B instance ID");
		return err;
	} else if (*inst_id > MAX_IO96B_INSTANCES) {
		shell_error(shell, "IO96B instance ID out of range");
		return -ERANGE;
	}

	switch (*inst_id) {
	case 0:
#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(io96b0), okay)
		*dev = DEVICE_DT_GET(DT_NODELABEL(io96b0));
#else
		shell_error(shell, "IO96B instance 0 not enabled in device tree");
		return -ENODEV;
#endif
		break;

	case 1:
#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(io96b1), okay)
		*dev = DEVICE_DT_GET(DT_NODELABEL(io96b1));
		break;
#else
		shell_error(shell, "IO96B instance 1 not enabled in device tree");
		return -ENODEV;
#endif

	default:
		shell_error(shell, "Invalid IO96B instant id");
		return -ENODEV;
	}

	if (!device_is_ready(*dev)) {
		shell_error(shell, "IO96B device not ready");
		return -ENODEV;
	}
	return 0;
}

static int cmd_io96b_info(const struct shell *shell, size_t argc, char **argv)
{
	const struct device *dev = NULL;
	int err;
	int inst_id = 0;
	struct io96b_mb_req_resp req_resp;

	err = get_io96_device(shell, argv, &dev, &inst_id);
	if (err) {
		return err;
	}

	shell_fprintf(shell, SHELL_NORMAL, "Show IO96B status\n");

	memset(&req_resp, 0, sizeof(struct io96b_mb_req_resp));

	req_resp.req.usr_cmd_type = CMD_GET_SYS_INFO;
	req_resp.req.usr_cmd_opcode = GET_MEM_INTF_INFO;

	err = io96b_mb_request(dev, &req_resp);

	if (err) {
		shell_error(shell, "IO96B mailbox get memory info failed\n");
		return err;
	}

	if (((req_resp.resp.cmd_resp_data_0 >> 29) & 0x7) > 0u) {
		shell_fprintf(shell, SHELL_NORMAL, "Memory interface 0 ID: %d , IP type: EMIF\n",
			      (req_resp.resp.cmd_resp_data_0 >> 24) & 0x1F);
	} else {
		shell_fprintf(shell, SHELL_NORMAL, "Memory interface 0 Not used\n");
	}
	if (((req_resp.resp.cmd_resp_data_1 >> 29) & 0x7) > 0u) {
		shell_fprintf(shell, SHELL_NORMAL, "Memory interface 1 ID: %d , IP type: EMIF\n",
			      (req_resp.resp.cmd_resp_data_1 >> 24) & 0x1F);
	} else {
		shell_fprintf(shell, SHELL_NORMAL, "Memory interface 1 Not used\n");
	}

	return err;
}

static int get_io96_interface_number(const struct shell *shell, char **argv,
				     struct io96b_mb_req_resp *req_resp)
{
	int err = 0;

	req_resp->req.io96b_intf_inst_num = shell_strtoul(argv[2], 10, &err);

	if (err != 0) {
		shell_error(shell, "Invalid IO96B interface ID");
		return err;
	} else if (req_resp->req.io96b_intf_inst_num > MAX_INTERFACES) {
		shell_error(shell, "IO96B interface ID out of range");
		return -ERANGE;
	}
	return 0;
}

static int cmd_io96b_ecc_en_set(const struct shell *shell, size_t argc, char **argv)
{
	const struct device *dev = NULL;
	int err;
	int inst_id = 0;
	struct io96b_mb_req_resp req_resp;
	uint32_t ecc_mode;
	uint32_t ecc_type;

	if (argc < 2) {
		shell_error(shell, "Invalid command arguments");
		return -EINVAL;
	}

	err = get_io96_device(shell, argv, &dev, &inst_id);
	if (err) {
		return err;
	}

	memset(&req_resp, 0, sizeof(struct io96b_mb_req_resp));

	req_resp.req.usr_cmd_type = CMD_TRIG_CONTROLLER_OP;
	err = get_io96_interface_number(shell, argv, &req_resp);
	if (err) {
		return err;
	}

	req_resp.req.usr_cmd_opcode = ECC_ENABLE_SET;

	ecc_mode = shell_strtoul(argv[3], 10, &err);
	if (err != 0) {
		shell_error(shell, "Invalid argument ECC mode ");
		return err;
	} else if (ecc_mode > MAX_ECC_MODE_VALUE) {
		shell_error(shell, "ECC mode value out of range");
		return -ERANGE;
	}
	ecc_type = shell_strtoul(argv[4], 10, &err);
	if (err != 0) {
		shell_error(shell, "Invalid argument ECC type");
		return err;
	} else if (ecc_type > MAX_ECC_TYPE_VALUE) {
		shell_error(shell, "ECC type value out of range");
		return -ERANGE;
	}

	req_resp.req.cmd_param_0 = (ecc_mode | (ecc_type << IO96B_EN_SET_ECC_TYPE_OFFSET));

	err = io96b_mb_request(dev, &req_resp);
	if (err) {
		shell_error(shell, "IO96B mailbox ECC enable set failed\n");
		return err;
	}
	shell_fprintf(shell, SHELL_NORMAL, "ECC enable set success\n");

	return 0;
}

static int cmd_io96b_ecc_en_status(const struct shell *shell, size_t argc, char **argv)
{
	const struct device *dev = NULL;
	int err;
	int inst_id = 0;
	struct io96b_mb_req_resp req_resp;

	if (argc < 2) {
		shell_error(shell, "Invalid command arguments");
		return -EINVAL;
	}

	err = get_io96_device(shell, argv, &dev, &inst_id);
	if (err) {
		return err;
	}

	memset(&req_resp, 0, sizeof(struct io96b_mb_req_resp));

	req_resp.req.usr_cmd_type = CMD_TRIG_CONTROLLER_OP;
	err = get_io96_interface_number(shell, argv, &req_resp);
	if (err) {
		return err;
	}

	req_resp.req.usr_cmd_opcode = ECC_ENABLE_STATUS;
	err = io96b_mb_request(dev, &req_resp);

	if (err) {
		shell_error(shell, "IO96B mailbox check ECC enable status failed\n");
		return err;
	}

	switch (IO96B_CMD_RESPONSE_DATA_SHORT(req_resp.resp.cmd_resp_status) &
		IO96B_ECC_ENABLE_RESPONSE_MODE_MASK) {
	case 1:
		shell_fprintf(shell, SHELL_NORMAL,
			      "ECC is enabled, but without detection or correction\n");
		break;
	case 2:
		shell_fprintf(shell, SHELL_NORMAL,
			      "ECC is enabled with detection, but correction is not supported\n");
		break;
	case 3:
		shell_fprintf(shell, SHELL_NORMAL,
			      "ECC is enabled with detection and correction\n");
		break;
	default:
		shell_fprintf(shell, SHELL_NORMAL, "ECC is disabled\n");
	}

	if (IO96B_CMD_RESPONSE_DATA_SHORT(req_resp.resp.cmd_resp_status) &
	    IO96B_ECC_ENABLE_RESPONSE_TYPE_MASK) {
		shell_fprintf(shell, SHELL_NORMAL, "ECC type: In-line ECC\n");
	} else {
		shell_fprintf(shell, SHELL_NORMAL, "ECC type: Out-of-Band ECC\n");
	}

	return 0;
}

static int cmd_io96b_ecc_err_inject(const struct shell *shell, size_t argc, char **argv)
{
	const struct device *dev = NULL;
	int err;
	int inst_id = 0;
	struct io96b_mb_req_resp req_resp;

	if (argc < 2) {
		shell_error(shell, "Invalid command arguments");
		return -EINVAL;
	}

	err = get_io96_device(shell, argv, &dev, &inst_id);
	if (err) {
		return err;
	}

	memset(&req_resp, 0, sizeof(struct io96b_mb_req_resp));

	req_resp.req.usr_cmd_type = CMD_TRIG_CONTROLLER_OP;
	err = get_io96_interface_number(shell, argv, &req_resp);
	if (err) {
		return err;
	}

	req_resp.req.usr_cmd_opcode = ECC_INJECT_ERROR;
	req_resp.req.cmd_param_0 = shell_strtoul(argv[3], 10, &err);
	if (err != 0) {
		shell_error(shell, "Invalid argument ecc err inject XOR bits ");
		return err;
	}
	err = io96b_mb_request(dev, &req_resp);
	if (err) {
		shell_error(shell, "IO96B mailbox inject ECC error failed\n");
		return err;
	}

	return 0;
}

static int cmd_io96b_ecc_get_sbe_cnt(const struct shell *shell, size_t argc, char **argv)
{
	const struct device *dev = NULL;
	const struct edac_ecc_driver_api *api;
	int err;
	int inst_id = 0;
	int sbe_count = 0;

	if (argc < 2) {
		shell_error(shell, "Invalid command arguments");
		return -EINVAL;
	}

	err = get_io96_device(shell, argv, &dev, &inst_id);
	if (err) {
		return err;
	}

	api = dev->api;

	sbe_count = api->get_sbe_ecc_err_cnt(dev, UNUSED_ARG);

	shell_fprintf(shell, SHELL_NORMAL, "SBE error count = %d\n", sbe_count);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	io96b_cmd_ecc,
	SHELL_CMD_ARG(en_set, NULL,
		      "ECC enable set <inst id> <interface id> <ECC mode> <ECC type>\n"
		      "ECC mode  0 - ECC disabled\n"
		      "          1 - ECC enabled without detection & correction\n"
		      "          2 - ECC enable with detection & without correction\n"
		      "          3 - ECC enabled with detection & correction\n"
		      "ECC type  0 - Out-of-Band ECC\n"
		      "          1 - In-line ECC",
		      cmd_io96b_ecc_en_set, 5, 0),
	SHELL_CMD_ARG(en_status, NULL, "ECC enable status <inst id> <interface id>\n",
		      cmd_io96b_ecc_en_status, 3, 0),
	SHELL_CMD_ARG(err_inject, NULL,
		      "Inject ECC error <inst id> <interface id> <xor check bits>\n",
		      cmd_io96b_ecc_err_inject, 4, 0),
	SHELL_CMD_ARG(get_sbe_count, NULL, "Get Single Bit Error count <inst id>\n",
		      cmd_io96b_ecc_get_sbe_cnt, 2, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_io96b_cmds,
			       SHELL_CMD_ARG(info, NULL,
					     "Show IO96B memory <interface> information",
					     cmd_io96b_info, 2, 0),
			       SHELL_CMD(ecc, &io96b_cmd_ecc,
					 "ECC related commands en_set/en_status/err_inject", NULL),
			       SHELL_SUBCMD_SET_END /* Array terminated. */
);

#endif /* CONFIG_IO96B_INTEL_SOCFPGA */

static int cmd_hps_ecc_err_inject(const struct shell *shell, size_t argc, char **argv)
{
#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(hps_ecc), okay)
	const struct device *ecc_dev;
	const struct edac_ecc_driver_api *api;
	int ecc_block_id = 0;
	int error_type = 0;
	int err;

	ecc_dev = DEVICE_DT_GET(DT_NODELABEL(hps_ecc));
	if (!device_is_ready(ecc_dev)) {
		shell_error(shell, "EDAC: System Manager ECC device is not ready");
		return -ENODEV;
	}
	api = ecc_dev->api;
	ecc_block_id = shell_strtoul(argv[1], 10, &err);
	if ((err != 0) ||
	    ((ecc_block_id < ECC_OCRAM) || (ecc_block_id > ECC_MODULE_MAX_INSTANCES) ||
	     (ecc_block_id == ECC_DMA0))) {
		shell_error(shell, "Invalid argument ecc error inject block id ");
		return err;
	}
	error_type = shell_strtoul(argv[2], 10, &err);
	if ((err != 0) || ((error_type != INJECT_DBE) && (error_type != INJECT_SBE))) {
		shell_error(shell, "Invalid argument ecc error inject error type ");
		return err;
	}
	api->inject_ecc_error(ecc_dev, ecc_block_id, error_type);

#else
	shell_error(shell, "HPS ECC device is not available");
#endif
	return 0;
}

static int cmd_hps_ecc_get_sbe_cnt(const struct shell *shell, size_t argc, char **argv)
{
#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(hps_ecc), okay)

	const struct device *ecc_dev;
	const struct edac_ecc_driver_api *api;
	int ecc_block_id = 0;
	int sbe_count = 0;
	int err;

	ecc_dev = DEVICE_DT_GET(DT_NODELABEL(hps_ecc));

	if (!device_is_ready(ecc_dev)) {
		shell_error(shell, "EDAC: System Manager ECC device is not ready");
		return -ENODEV;
	}

	api = ecc_dev->api;

	ecc_block_id = shell_strtoul(argv[1], 10, &err);
	if ((err != 0) ||
	    ((ecc_block_id < ECC_OCRAM) || (ecc_block_id > ECC_MODULE_MAX_INSTANCES) ||
	     (ecc_block_id == ECC_DMA0))) {
		shell_error(shell, "Invalid argument ecc error inject block id ");
		return err;
	}

	sbe_count = api->get_sbe_ecc_err_cnt(ecc_dev, ecc_block_id);

	shell_fprintf(shell, SHELL_NORMAL, "SBE error count = %d\n", sbe_count);
#else
	shell_error(shell, "HPS ECC device is not available");
#endif
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_hps_ecc_cmds,
			       SHELL_CMD_ARG(err_inject, NULL,
					     "Inject ECC error <Block id> <error type>\n"
					     "ECC block id  1  - ECC_OCRAM\n"
					     "              2  - ECC_USB0_RAM0\n"
					     "              3  - ECC_USB1_RAM0\n"
					     "              4  - ECC_EMAC0_RX\n"
					     "              5  - ECC_EMAC0_TX\n"
					     "              6  - ECC_EMAC1_RX\n"
					     "              7  - ECC_EMAC1_TX\n"
					     "              8  - ECC_EMAC2_RX\n"
					     "              9  - ECC_EMAC2_TX\n"
					     "              10 - ECC_DMA0\n"
					     "              11 - ECC_USB1_RAM1\n"
					     "              12 - ECC_USB1_RAM2\n"
					     "              13 - ECC_NAND\n"
					     "              14 - ECC_SDMMCA\n"
					     "              15 - ECC_SDMMCB\n"
					     "              18 - ECC_DMA1\n"
						 "              19 - ECC_QSPI\n"
					     "error type 1 - DBE\n"
					     "           2 - SBE\n",
					     cmd_hps_ecc_err_inject, 3, 0),
			       SHELL_CMD_ARG(get_sbe_count, NULL,
					     "Get Single Bit Error count <Block id>\n",
					     cmd_hps_ecc_get_sbe_cnt, 2, 0),
			       SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_edac_cmds,
#if CONFIG_IO96B_INTEL_SOCFPGA
			       SHELL_CMD(io96b, &sub_io96b_cmds, "IO96B information", NULL),
#endif /* CONFIG_IO96B_INTEL_SOCFPGA */
			       SHELL_CMD(hps_ecc, &sub_hps_ecc_cmds,
					 "HPS ECC information\n"
					 "ECC related command err_inject",
					 NULL),

			       SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(edac, &sub_edac_cmds, "EDAC information", NULL);
