/*
 * Copyright (c) 2021, Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Arm SiP services driver shell command 'sip_svc'.
 */

#include <sys/printk.h>
#include <shell/shell.h>
#include <stdlib.h>
#include <device.h>
#include <drivers/sip_svc/sip_svc.h>

#define DT_DRV_COMPAT arm_sip_svc

static int parse_common_args(const struct shell *sh, char **argv,
			     const struct device **dev)
{
	*dev = device_get_binding(argv[1]);
	if (!*dev) {
		shell_error(sh, "Arm SiP services device %s not found", argv[1]);
		return -ENODEV;
	}
	return 0;
}

static int hex_str_to_uint(const struct shell *sh, const char *hex_str,
			     uint32_t size, uint64_t *hex_val)
{
	uint32_t offset = 0;
	uint64_t half;
	char c;
	int len;
	int i;

	if (!hex_str || !hex_val) {
		shell_error(sh, "Invalid pointer when parsing hex");
		return -EINVAL;
	}

	if (size & 0xF) {
		shell_error(sh, "Hex size must be 4 bits aligned");
		return -EINVAL;
	}

	len = (int)(strlen(hex_str));

	if (len > (size / 4) || len > 16) {
		shell_error(sh, "Hex %s too long, expected length is %d",
			    hex_str, size / 4);
		return -EOVERFLOW;
	}

	*hex_val = 0;
	for (i = len - 1; i >= 0; i--) {
		c = hex_str[i];

		if (c >= '0' && c <= '9')
			half = (uint8_t)(c - '0');
		else if (c == 'A' || c == 'a')
			half = 0xa;
		else if (c == 'B' || c == 'b')
			half = 0xb;
		else if (c == 'C' || c == 'c')
			half = 0xc;
		else if (c == 'D' || c == 'd')
			half = 0xd;
		else if (c == 'E' || c == 'e')
			half = 0xe;
		else if (c == 'F' || c == 'f')
			half = 0xf;
		else {
			shell_error(sh, "Found unrecognized hex character '%c'", c);
			return -EFAULT;
		}

		*hex_val |= (half & 0xF) << offset;
		offset += 4;
	}

	return 0;
}

static int cmd_reg(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	uint32_t c_token;
	int err;

	err = parse_common_args(sh, argv, &dev);
	if (err < 0) {
		return err;
	}

	c_token = sip_svc_register(dev, NULL);
	if (c_token == SIP_SVC_ID_INVALID) {
		shell_error(sh, "%s: register fail", dev->name);
		err = -1;
	} else {
		shell_print(sh, "%s: register success: client token %08x",
			    dev->name, c_token);
		err = 0;
	}

	return err;
}

static int cmd_unreg(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	uint64_t c_token;
	int err;

	err = parse_common_args(sh, argv, &dev);
	if (err < 0) {
		return err;
	}

	err = hex_str_to_uint(sh, argv[2], 32, &c_token);
	if (err < 0) {
		return err;
	}

	err = sip_svc_unregister(dev, (uint32_t)c_token);
	if (err) {
		shell_error(sh, "%s: unregister fail (%d): client token %08x",
			    dev->name, err, (uint32_t)c_token);
	} else {
		shell_print(sh, "%s: unregister success: client token %08x",
			    dev->name, (uint32_t)c_token);
	}

	return err;
}

static int cmd_open(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	uint64_t c_token;
	uint32_t usecond = 0;
	int err;

	err = parse_common_args(sh, argv, &dev);
	if (err < 0) {
		return err;
	}

	err = hex_str_to_uint(sh, argv[2], 32, &c_token);
	if (err < 0) {
		return err;
	}

	if (argc > 3) {
		usecond = (uint32_t)atoi(argv[3]) * 1000000;
	}

	err = sip_svc_open(dev, (uint32_t)c_token, usecond);
	if (err) {
		shell_error(sh, "%s: open fail (%d): client token %08x",
			    dev->name, err, (uint32_t)c_token);
	} else {
		shell_print(sh, "%s: open success: client token %08x",
			    dev->name, (uint32_t)c_token);
	}

	return err;
}

static int cmd_close(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	uint64_t c_token;
	int err;

	err = parse_common_args(sh, argv, &dev);
	if (err < 0) {
		return err;
	}

	err = hex_str_to_uint(sh, argv[2], 32, &c_token);
	if (err < 0) {
		return err;
	}

	err = sip_svc_close(dev, (uint32_t)c_token);
	if (err) {
		shell_error(sh, "%s: close fail (%d): client token %08x",
			    dev->name, err, (uint32_t)c_token);
	} else {
		shell_print(sh, "%s: close success: client token %08x",
			    dev->name, (uint32_t)c_token);
	}

	return err;
}

static void cmd_send_callback(uint32_t c_token, char *res, int size)
{
	struct sip_svc_response *response =
		(struct sip_svc_response *) res;

	printk("sip_svc send command callback\n");

	if ((size_t)size != sizeof(struct sip_svc_response)) {
		printk("Invalid response size (%d), expects (%ld)",
			size, sizeof(struct sip_svc_response));
		return;
	}

	printk("\theader=%08x\n", response->header);
	printk("\ta0=%016lx\n", response->a0);
	printk("\ta1=%016lx\n", response->a1);
	printk("\ta2=%016lx\n", response->a2);
	printk("\ta3=%016lx\n", response->a3);
}

static int cmd_send(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	uint64_t c_token;
	uint32_t trans_id;
	struct sip_svc_request request;
	int err;

	err = parse_common_args(sh, argv, &dev);
	if (err < 0)
		return err;

	err = hex_str_to_uint(sh, argv[2], 32, &c_token);
	if (err < 0) {
		return err;
	}

	request.header = SIP_SVC_PROTO_HEADER(SIP_SVC_PROTO_CMD_SYNC, 0);

	err = hex_str_to_uint(sh, argv[3], 64, (uint64_t *)&request.a0);
	if (err < 0) {
		return err;
	}

	if (argc > 4) {
		err = hex_str_to_uint(sh, argv[4], 64, (uint64_t *)&request.a1);
		if (err < 0) {
			return err;
		}
	}

	if (argc > 5) {
		err = hex_str_to_uint(sh, argv[5], 64, (uint64_t *)&request.a2);
		if (err < 0) {
			return err;
		}
	}

	if (argc > 6) {
		err = hex_str_to_uint(sh, argv[6], 64, (uint64_t *)&request.a3);
		if (err < 0) {
			return err;
		}
	}

	if (argc > 7) {
		err = hex_str_to_uint(sh, argv[7], 64, (uint64_t *)&request.a4);
		if (err < 0) {
			return err;
		}
	}

	if (argc > 8) {
		err = hex_str_to_uint(sh, argv[8], 64, (uint64_t *)&request.a5);
		if (err < 0) {
			return err;
		}
	}

	if (argc > 9) {
		err = hex_str_to_uint(sh, argv[9], 64, (uint64_t *)&request.a6);
		if (err < 0) {
			return err;
		}
	}

	if (argc > 10) {
		err = hex_str_to_uint(sh, argv[10], 64, (uint64_t *)&request.a7);
		if (err < 0) {
			return err;
		}
	}

	request.resp_data_addr = 0;
	request.resp_data_size = 0;
	request.priv_data = NULL;

	trans_id = sip_svc_send(dev, (uint32_t)c_token, (uint8_t *)&request,
				sizeof(struct sip_svc_request),
				cmd_send_callback);

	if (trans_id == SIP_SVC_ID_INVALID) {
		shell_error(sh, "%s: send fail: client token %08x",
			    dev->name, (uint32_t)c_token);
		err = -EBUSY;
	} else {
		shell_print(sh, "%s: send success: client token %08x, trans_id %d",
			    dev->name, (uint32_t)c_token, trans_id);
		err = 0;
	}

	return err;
}

static int cmd_info(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int err;

	err = parse_common_args(sh, argv, &dev);
	if (err < 0) {
		return err;
	}

	sip_svc_print_info(dev);

	return err;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_sip_svc,
	SHELL_CMD_ARG(reg, NULL, "<device>",
		      cmd_reg, 2, 0),
	SHELL_CMD_ARG(unreg, NULL, "<device> <token>",
		      cmd_unreg, 3, 0),
	SHELL_CMD_ARG(open, NULL, "<device> <token> <[timeout_sec]>",
		      cmd_open, 3, 1),
	SHELL_CMD_ARG(close, NULL, "<device> <token>",
		      cmd_close, 3, 0),
	SHELL_CMD_ARG(send, NULL, "<device> <token> <a0> [<a1> <a2> ... <a7>]",
		      cmd_send, 4, 7),
	SHELL_CMD_ARG(info, NULL, "<device>",
		      cmd_info, 2, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(sip_svc, &sub_sip_svc, "Arm SiP services driver commands",
		   NULL);
