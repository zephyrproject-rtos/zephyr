/*
 * Copyright (c) 2022-2023, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * ARM SiP service shell command 'sip_svc'.
 */

#include <zephyr/shell/shell.h>
#include <zephyr/sip_svc/sip_svc.h>
#include <zephyr/sip_svc/sip_svc_controller.h>
#include <stdlib.h>
#include <errno.h>

#define MAX_TIMEOUT_SECS (10 * 60UL)

struct private_data {
	struct k_sem semaphore;
	const struct shell *sh;
};

static int parse_common_args(const struct shell *sh, char **argv, void **ctrl)
{
	*ctrl = sip_svc_get_controller(argv[1]);

	if (!*ctrl) {
		shell_error(sh, "service %s not found", argv[1]);
		return -ENODEV;
	}

	struct sip_svc_controller *ct = (struct sip_svc_controller *)(*ctrl);

	if (!ct->init) {
		shell_error(sh, "ARM SiP services method %s not initialized", argv[1]);
		return -ENODEV;
	}
	return 0;
}

static int cmd_reg(const struct shell *sh, size_t argc, char **argv)
{
	struct sip_svc_controller *ctrl;
	uint32_t c_token;
	int err;

	err = parse_common_args(sh, argv, (void **)&ctrl);
	if (err < 0) {
		return err;
	}

	c_token = sip_svc_register(ctrl, NULL);
	if (c_token == SIP_SVC_ID_INVALID) {
		shell_error(sh, "%s: register fail", ctrl->method);
		err = -1;
	} else {
		shell_print(sh, "%s: register success: client token %08x\n", ctrl->method, c_token);
		err = 0;
	}

	return err;
}

static int cmd_unreg(const struct shell *sh, size_t argc, char **argv)
{
	struct sip_svc_controller *ctrl;
	uint32_t c_token;
	int err;
	char *endptr;

	err = parse_common_args(sh, argv, (void **)&ctrl);
	if (err < 0) {
		return err;
	}

	errno = 0;
	c_token = strtoul(argv[2], &endptr, 16);
	if (errno == ERANGE) {
		shell_error(sh, "Out of range value");
		return -ERANGE;
	} else if (errno || endptr == argv[2] || *endptr) {
		shell_error(sh, "Invalid argument");
		return -errno;
	}

	err = sip_svc_unregister(ctrl, (uint32_t)c_token);
	if (err) {
		shell_error(sh, "%s: unregister fail (%d): client token %08x", ctrl->method, err,
			    (uint32_t)c_token);
	} else {
		shell_print(sh, "%s: unregister success: client token %08x", ctrl->method,
			    (uint32_t)c_token);
	}

	return err;
}

static int cmd_open(const struct shell *sh, size_t argc, char **argv)
{
	struct sip_svc_controller *ctrl;
	uint32_t c_token;
	unsigned long seconds = 0;
	int err;
	char *endptr;
	k_timeout_t timeout = K_FOREVER;

	err = parse_common_args(sh, argv, (void **)&ctrl);
	if (err < 0) {
		return err;
	}

	errno = 0;
	c_token = strtoul(argv[2], &endptr, 16);
	if (errno == ERANGE) {
		shell_error(sh, "Out of range value");
		return -ERANGE;
	} else if (errno || endptr == argv[2] || *endptr) {
		shell_error(sh, "Invalid argument");
		return -errno;
	}

	if (argc > 3) {
		errno = 0;
		seconds = strtoul(argv[3], &endptr, 10);
		if (errno == ERANGE) {
			shell_error(sh, "Out of range value");
			return -ERANGE;
		} else if (errno || endptr == argv[3] || *endptr) {
			shell_error(sh, "Invalid Argument");
			return -EINVAL;
		} else if (seconds <= MAX_TIMEOUT_SECS) {
			timeout = K_SECONDS(seconds);
		} else {
			timeout = K_SECONDS(MAX_TIMEOUT_SECS);
			shell_error(sh, "Setting timeout value to %lu", MAX_TIMEOUT_SECS);
		}
	}

	err = sip_svc_open(ctrl, (uint32_t)c_token, timeout);
	if (err) {
		shell_error(sh, "%s: open fail (%d): client token %08x", ctrl->method, err,
			    (uint32_t)c_token);
	} else {
		shell_print(sh, "%s: open success: client token %08x", ctrl->method,
			    (uint32_t)c_token);
	}

	return err;
}

static int cmd_close(const struct shell *sh, size_t argc, char **argv)
{
	struct sip_svc_controller *ctrl;
	uint32_t c_token;
	int err;
	char *endptr;

	err = parse_common_args(sh, argv, (void **)&ctrl);
	if (err < 0) {
		return err;
	}

	errno = 0;
	c_token = strtoul(argv[2], &endptr, 16);
	if (errno == ERANGE) {
		shell_error(sh, "Out of range value");
		return -ERANGE;
	} else if (errno || endptr == argv[2] || *endptr) {
		shell_error(sh, "Invalid argument");
		return -errno;
	}

	err = sip_svc_close(ctrl, (uint32_t)c_token, NULL);
	if (err) {
		shell_error(sh, "%s: close fail (%d): client token %08x", ctrl->method, err,
			    (uint32_t)c_token);
	} else {
		shell_print(sh, "%s: close success: client token %08x", ctrl->method,
			    (uint32_t)c_token);
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

	shell_print(sh, "\n\rsip_svc send callback response\n");
	shell_print(sh, "\theader=%08x\n", response->header);
	shell_print(sh, "\ta0=%016lx\n", response->a0);
	shell_print(sh, "\ta1=%016lx\n", response->a1);
	shell_print(sh, "\ta2=%016lx\n", response->a2);
	shell_print(sh, "\ta3=%016lx\n", response->a3);

	k_sem_give(&(priv->semaphore));
}

static int cmd_send(const struct shell *sh, size_t argc, char **argv)
{
	struct sip_svc_controller *ctrl;
	uint32_t c_token;
	int trans_id;
	struct sip_svc_request request;
	struct private_data priv;
	char *endptr;
	int err;

	err = parse_common_args(sh, argv, (void **)&ctrl);
	if (err < 0) {
		return err;
	}

	errno = 0;
	c_token = strtoul(argv[2], &endptr, 16);
	if (errno == ERANGE) {
		shell_error(sh, "Out of range value");
		return -ERANGE;
	} else if (errno || endptr == argv[2] || *endptr) {
		shell_error(sh, "Invalid argument");
		return -errno;
	}

	request.header = SIP_SVC_PROTO_HEADER(SIP_SVC_PROTO_CMD_SYNC, 0);

	request.a0 = strtoul(argv[3], &endptr, 16);
	if (errno == ERANGE) {
		shell_error(sh, "Out of range value for a0");
		return -ERANGE;
	} else if (errno || endptr == argv[3] || *endptr) {
		shell_error(sh, "Invalid argument for a0");
		return -errno;
	}

	if (argc > 4) {
		request.a1 = strtoul(argv[4], &endptr, 16);
		if (errno == ERANGE) {
			shell_error(sh, "Out of range value for a1");
			return -ERANGE;
		} else if (errno || endptr == argv[4] || *endptr) {
			shell_error(sh, "Invalid argument for a1");
			return -errno;
		}
	}

	if (argc > 5) {
		request.a2 = strtoul(argv[5], &endptr, 16);
		if (errno == ERANGE) {
			shell_error(sh, "Out of range value for a2");
			return -ERANGE;
		} else if (errno || endptr == argv[5] || *endptr) {
			shell_error(sh, "Invalid argument for a2");
			return -errno;
		}
	}

	if (argc > 6) {
		request.a3 = strtoul(argv[6], &endptr, 16);
		if (errno == ERANGE) {
			shell_error(sh, "Out of range value for a3");
			return -ERANGE;
		} else if (errno || endptr == argv[6] || *endptr) {
			shell_error(sh, "Invalid argument for a3");
			return -errno;
		}
	}

	if (argc > 7) {
		request.a4 = strtoul(argv[7], &endptr, 16);
		if (errno == ERANGE) {
			shell_error(sh, "Out of range value for a4");
			return -ERANGE;
		} else if (errno || endptr == argv[7] || *endptr) {
			shell_error(sh, "Invalid argument for a4");
			return -errno;
		}
	}

	if (argc > 8) {
		request.a5 = strtoul(argv[8], &endptr, 16);
		if (errno == ERANGE) {
			shell_error(sh, "Out of range value for a5");
			return -ERANGE;
		} else if (errno || endptr == argv[8] || *endptr) {
			shell_error(sh, "Invalid argument for a5");
			return -errno;
		}
	}

	if (argc > 9) {
		request.a6 = strtoul(argv[9], &endptr, 16);
		if (errno == ERANGE) {
			shell_error(sh, "Out of range value for a6");
			return -ERANGE;
		} else if (errno || endptr == argv[9] || *endptr) {
			shell_error(sh, "Invalid argument for a6");
			return -errno;
		}
	}

	if (argc > 10) {
		request.a7 = strtoul(argv[10], &endptr, 16);
		if (errno == ERANGE) {
			shell_error(sh, "Out of range value for a7");
			return -ERANGE;
		} else if (errno || endptr == argv[10] || *endptr) {
			shell_error(sh, "Invalid argument for a7");
			return -errno;
		}
	}

	k_sem_init(&(priv.semaphore), 0, 1);
	priv.sh = sh;

	request.resp_data_addr = 0;
	request.resp_data_size = 0;
	request.priv_data = (void *)&priv;

	trans_id = sip_svc_send(ctrl, (uint32_t)c_token, &request, cmd_send_callback);

	if (trans_id < 0) {
		shell_error(sh, "%s: send fail: client token %08x", ctrl->method,
			    (uint32_t)c_token);
		err = trans_id;
	} else {
		/*wait for callback*/
		k_sem_take(&(priv.semaphore), K_FOREVER);

		shell_print(sh, "%s: send success: client token %08x, trans_id %d", ctrl->method,
			    (uint32_t)c_token, trans_id);
		err = 0;
	}
	return err;
}

static int cmd_info(const struct shell *sh, size_t argc, char **argv)
{
	struct sip_svc_controller *ctrl;
	int err;
	uint32_t i;
	static const char *const state_str_list[] = {"INVALID", "IDLE", "OPEN", "ABORT"};
	const char *state_str = "UNKNOWN";

	err = parse_common_args(sh, argv, (void **)&ctrl);
	if (err < 0) {
		return err;
	}

	shell_print(sh, "---------------------------------------\n");
	shell_print(sh, "sip_svc service information\n");
	shell_print(sh, "---------------------------------------\n");

	shell_print(sh, "active job cnt         %d\n", ctrl->active_job_cnt);
	shell_print(sh, "active async job cnt   %d\n", ctrl->active_async_job_cnt);

	shell_print(sh, "---------------------------------------\n");
	shell_print(sh, "Client Token\tState\tTrans Cnt\n");
	shell_print(sh, "---------------------------------------\n");
	for (i = 0; i < ctrl->num_clients; i++) {
		if (ctrl->clients[i].id != SIP_SVC_ID_INVALID) {
			if (ctrl->clients[i].state <= SIP_SVC_CLIENT_ST_ABORT) {
				state_str = state_str_list[ctrl->clients[i].state];
			}

			shell_print(sh, "%08x    \t%-10s\t%-9d\n", ctrl->clients[i].token,
				    state_str, ctrl->clients[i].active_trans_cnt);
		}
	}
	return err;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_sip_svc, SHELL_CMD_ARG(reg, NULL, "<method>", cmd_reg, 2, 0),
	SHELL_CMD_ARG(unreg, NULL, "<method> <token>", cmd_unreg, 3, 0),
	SHELL_CMD_ARG(open, NULL, "<method> <token> <[timeout_sec]>", cmd_open, 3, 1),
	SHELL_CMD_ARG(close, NULL, "<method> <token>", cmd_close, 3, 0),
	SHELL_CMD_ARG(send, NULL, "<method> <token> <a0> [<a1> <a2> ... <a7>]", cmd_send, 4, 7),
	SHELL_CMD_ARG(info, NULL, "<method>", cmd_info, 2, 0), SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(sip_svc, &sub_sip_svc, "ARM SiP services commands", NULL);
