/*
 * Copyright (c) 2022-2023, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * A mailbox client command shell on sip_svc service to communicate with SDM.
 */

#include <zephyr/kernel.h>
#include <zephyr/sip_svc/sip_svc.h>
#include <zephyr/drivers/sip_svc/sip_svc_agilex_mailbox.h>
#include <zephyr/drivers/sip_svc/sip_svc_agilex_smc.h>
#include <zephyr/shell/shell.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define MAX_TIMEOUT_MSECS (1 * 1000UL)

struct private_data {
	struct k_sem semaphore;
	const struct shell *sh;
};

static void *mb_smc_ctrl;
static uint32_t mb_c_token = SIP_SVC_ID_INVALID;

static int cmd_reg(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	if (mb_smc_ctrl) {
		shell_print(sh, "Mailbox client already registered");
		return 0;
	}

	mb_smc_ctrl = sip_svc_get_controller(argv[1]);
	if (!mb_smc_ctrl) {
		shell_error(sh, "Arm SiP service %s not found", argv[1]);
		return -ENODEV;
	}

	mb_c_token = sip_svc_register(mb_smc_ctrl, NULL);
	if (mb_c_token == SIP_SVC_ID_INVALID) {
		mb_smc_ctrl = NULL;
		shell_error(sh, "Mailbox client register fail");
		err = -1;
	} else {
		shell_print(sh, "Mailbox client register success (token %08x)", mb_c_token);
		err = 0;
	}

	return err;
}

static int cmd_unreg(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	if (!mb_smc_ctrl) {
		shell_print(sh, "Mailbox client is not registered");
		return 0;
	}

	err = sip_svc_unregister(mb_smc_ctrl, mb_c_token);
	if (err) {
		shell_error(sh, "Mailbox client unregister fail (%d)", err);
	} else {
		shell_print(sh, "Mailbox client unregister success");
		mb_c_token = SIP_SVC_ID_INVALID;
		mb_smc_ctrl = NULL;
	}

	return err;
}

static int cmd_open(const struct shell *sh, size_t argc, char **argv)
{
	unsigned long mseconds = 0;
	int err;
	char *endptr;
	k_timeout_t timeout = K_MSEC(MAX_TIMEOUT_MSECS);

	if (!mb_smc_ctrl) {
		shell_print(sh, "Mailbox client is not registered");
		return 0;
	}

	if (argc > 1) {
		errno = 0;
		mseconds = strtoul(argv[1], &endptr, 10);
		if (errno == ERANGE) {
			shell_error(sh, "out of range value");
			return -ERANGE;
		} else if (errno || endptr == argv[1] || *endptr) {
			return -errno;
		} else if (mseconds <= MAX_TIMEOUT_MSECS) {
			timeout = K_MSEC(mseconds);
		} else {
			timeout = K_MSEC(MAX_TIMEOUT_MSECS);
			shell_error(sh, "Setting timeout value to %lu milliseconds",
				    MAX_TIMEOUT_MSECS);
		}
	}

	err = sip_svc_open(mb_smc_ctrl, mb_c_token, timeout);
	if (err) {
		shell_error(sh, "Mailbox client open fail (%d)", err);
	} else {
		shell_print(sh, "Mailbox client open success");
	}

	return err;
}

static int cmd_close(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	uint32_t cmd_size = sizeof(uint32_t);
	struct sip_svc_request request;

	if (!mb_smc_ctrl) {
		shell_print(sh, "Mailbox client is not registered");
		return 0;
	}

	uint32_t *cmd_addr = k_malloc(cmd_size);

	if (!cmd_addr) {
		return -ENOMEM;
	}

	*cmd_addr = MAILBOX_CANCEL_COMMAND;

	request.header = SIP_SVC_PROTO_HEADER(SIP_SVC_PROTO_CMD_ASYNC, 0);
	request.a0 = SMC_FUNC_ID_MAILBOX_SEND_COMMAND;
	request.a1 = 0;
	request.a2 = (uint64_t)cmd_addr;
	request.a3 = (uint64_t)cmd_size;
	request.a4 = 0;
	request.a5 = 0;
	request.a6 = 0;
	request.a7 = 0;
	request.resp_data_addr = (uint64_t)NULL;
	request.resp_data_size = 0;
	request.priv_data = NULL;

	err = sip_svc_close(mb_smc_ctrl, mb_c_token, &request);
	if (err) {
		k_free(cmd_addr);
		shell_error(sh, "Mailbox client close fail (%d)", err);
	} else {
		shell_print(sh, "Mailbox client close success");
	}

	return err;
}

static void cmd_send_callback(uint32_t c_token, struct sip_svc_response *response)
{
	if (response == NULL) {
		return;
	}

	struct private_data *priv = (struct private_data *)response->priv_data;
	const struct shell *sh = priv->sh;

	uint32_t *resp_data;
	uint32_t resp_len;
	uint32_t i;

	shell_print(sh, "\n\rsip_svc send command callback\n");
	shell_print(sh, "\theader=%08x\n", response->header);
	shell_print(sh, "\ta0=%016lx\n", response->a0);
	shell_print(sh, "\ta1=%016lx\n", response->a1);
	shell_print(sh, "\ta2=%016lx\n", response->a2);
	shell_print(sh, "\ta3=%016lx\n", response->a3);
	shell_print(sh, "\tresponse data=\n");

	resp_data = (uint32_t *)response->resp_data_addr;
	resp_len = response->resp_data_size / 4;
	if (resp_data && resp_len) {
		for (i = 0; i < resp_len; i++) {
			shell_print(sh, "\t\t[%4d] %08x\n", i, resp_data[i]);
		}
	} else {
		shell_error(sh, "\t\tInvalid addr (%p) or len (%d)\n", resp_data, resp_len);
	}

	/* Client only responsible to free the response data memory space,
	 * the command data memory space had been freed by sip_svc service.
	 */
	if (resp_data) {
		shell_print(sh, "response data %p is freed\n", resp_data);
		k_free((char *)resp_data);
	}

	k_sem_give(&(priv->semaphore));
}

static int parse_mb_data(const struct shell *sh, char *hex_list, char **cmd_addr,
			 uint32_t *cmd_size)
{
	char *hex_str = hex_list;
	uint32_t hex_val;
	uint32_t *buffer;
	uint32_t i = 0;
	char *state;
	char *endptr;

	if (!hex_list || !cmd_addr || !cmd_size) {
		return -EINVAL;
	}

	*cmd_addr = k_malloc(SIP_SVP_MB_MAX_WORD_SIZE * 4);
	if (!*cmd_addr) {
		shell_error(sh, "Fail to allocate command memory");
		return -ENOMEM;
	}

	buffer = (uint32_t *)*cmd_addr;
	hex_str = strtok_r(hex_str, " ", &state);

	while (hex_str) {
		if (i >= SIP_SVP_MB_MAX_WORD_SIZE) {
			k_free(*cmd_addr);
			shell_error(sh, "Mailbox length too long");
			return -EOVERFLOW;
		}
		errno = 0;
		hex_val = strtoul(hex_str, &endptr, 16);
		if (errno == ERANGE) {
			shell_error(sh, " Value is out of range value");
			k_free(*cmd_addr);
			return -ERANGE;
		} else if (errno || endptr == hex_str || *endptr) {
			k_free(*cmd_addr);
			shell_error(sh, " Invalid argument");
			return -EINVAL;
		}

		buffer[i] = hex_val;
		i++;

		hex_str = strtok_r(NULL, " ", &state);
	}

	*cmd_size = i * 4;

	return 0;
}

static int cmd_send(const struct shell *sh, size_t argc, char **argv)
{
	struct sip_svc_request request;
	int trans_id;
	uint32_t cmd_size = 0;
	struct private_data priv;
	char *cmd_addr;
	char *resp_addr, *endptr;
	int err;
	k_timeout_t timeout = K_FOREVER;
	unsigned long msecond = 0;

	if (!mb_smc_ctrl) {
		shell_print(sh, "Mailbox client is not registered");
		return 0;
	}

	err = parse_mb_data(sh, argv[1], &cmd_addr, &cmd_size);
	if (err < 0) {
		return err;
	}

	if (argc > 2) {
		errno = 0;
		msecond = strtoul(argv[2], &endptr, 10);
		if (errno == ERANGE) {
			shell_error(sh, "Out of range value");
			return -ERANGE;
		} else if (errno || endptr == argv[2] || *endptr) {
			shell_error(sh, "Invalid argument");
			return -EINVAL;
		} else if (msecond <= (MSEC_PER_SEC * MAX_TIMEOUT_MSECS)) {
			timeout = K_MSEC(msecond);
		} else {
			timeout = K_SECONDS(MAX_TIMEOUT_MSECS);
			shell_error(sh, "Setting timeout value to %lu seconds", MAX_TIMEOUT_MSECS);
		}
	}

	resp_addr = k_malloc(SIP_SVP_MB_MAX_WORD_SIZE * 4);
	if (!resp_addr) {
		k_free(cmd_addr);
		shell_error(sh, "Fail to allocate response memory");
		return -ENOMEM;
	}
	shell_print(sh, "\tResponse memory %p\n", (char *)resp_addr);

	k_sem_init(&(priv.semaphore), 0, 1);
	priv.sh = sh;

	request.header = SIP_SVC_PROTO_HEADER(SIP_SVC_PROTO_CMD_ASYNC, 0);
	request.a0 = SMC_FUNC_ID_MAILBOX_SEND_COMMAND;
	request.a1 = 0;
	request.a2 = (uint64_t)cmd_addr;
	request.a3 = (uint64_t)cmd_size;
	request.a4 = 0;
	request.a5 = 0;
	request.a6 = 0;
	request.a7 = 0;
	request.resp_data_addr = (uint64_t)resp_addr;
	request.resp_data_size = SIP_SVP_MB_MAX_WORD_SIZE * 4;
	request.priv_data = (void *)&priv;

	trans_id = sip_svc_send(mb_smc_ctrl, mb_c_token, &request, cmd_send_callback);

	if (trans_id < 0) {
		shell_error(sh, "Mailbox send fail (no open or no free trans_id)");
		k_free(cmd_addr);
		k_free(resp_addr);
		err = -EBUSY;
	} else {
		/*wait for callback*/
		if (!k_sem_take(&(priv.semaphore), timeout)) {
			shell_print(sh, "Mailbox send success: trans_id %d", trans_id);
			err = 0;
		} else {
			shell_error(sh, "Mailbox send timeout: trans_id %d", trans_id);
			cmd_close(sh, 0, NULL);
			err = -ETIMEDOUT;
		}
	}
	return err;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_mailbox, SHELL_CMD_ARG(reg, NULL, "<service>", cmd_reg, 2, 0),
	SHELL_CMD_ARG(unreg, NULL, NULL, cmd_unreg, 1, 0),
	SHELL_CMD_ARG(open, NULL, "[<timeout_msec>]", cmd_open, 1, 1),
	SHELL_CMD_ARG(close, NULL, NULL, cmd_close, 1, 0),
	SHELL_CMD_ARG(send, NULL,
		      "<hex list, example (SYNC): \"2001 11223344 aabbccdd\"> [<timeout_msec>]",
		      cmd_send, 2, 1),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(mailbox, &sub_mailbox, "Intel SoC FPGA SDM mailbox client commands", NULL);
