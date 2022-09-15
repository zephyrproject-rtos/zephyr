/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define LOG_MODULE_NAME net_lwm2m_shell
#define LOG_LEVEL	CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/lwm2m.h>
#include <zephyr/shell/shell.h>

#include <lwm2m_engine.h>
#include <lwm2m_util.h>

#define LWM2M_HELP_CMD "LwM2M commands"
#define LWM2M_HELP_SEND "LwM2M SEND operation\nsend [OPTION]... [PATH]...\n" \
	"-n\t Send as non-confirmable\n" \
	"Paths are inserted without leading '/'\n" \
	"Root-level operation is unsupported"
#define LWM2M_HELP_EXEC "Execute a resource\nexec PATH\n"
#define LWM2M_HELP_READ "Read value from LwM2M resource\nread PATH [OPTIONS]\n" \
	"-s \tRead value as string(default)\n" \
	"-b \tRead value as bool (1/0)\n" \
	"-uX\tRead value as uintX_t\n" \
	"-sX\tRead value as intX_t\n" \
	"-f \tRead value as float\n"
#define LWM2M_HELP_WRITE "Write into LwM2M resource\nwrite PATH [OPTIONS] VALUE\n" \
	"-s \tValue as string(default)\n" \
	"-b \tValue as bool\n" \
	"-uX\tValue as uintX_t\n" \
	"-sX\tValue as intX_t\n" \
	"-f \tValue as float\n"
#define LWM2M_HELP_START "Start the LwM2M RD (Registration / Discovery) Client\n" \
	"start EP_NAME [OPTIONS] [BOOTSTRAP FLAG]\n" \
	"-b \tSet the bootstrap flag (default 0)\n"
#define LWM2M_HELP_STOP "Stop the LwM2M RD (De-register) Client\nstop [OPTIONS]\n" \
	"-f \tForce close the connection\n"
#define LWM2M_HELP_UPDATE "Trigger Registration Update of the LwM2M RD Client\n"
#define LWM2M_HELP_PAUSE "LwM2M engine thread pause"
#define LWM2M_HELP_RESUME "LwM2M engine thread resume"

static int cmd_send(const struct shell *sh, size_t argc, char **argv)
{
	int ret = 0;
	struct lwm2m_ctx *ctx = lwm2m_rd_client_ctx();
	int path_cnt = argc - 1;
	bool confirmable = true;
	int ignore_cnt = 1; /* Subcmd + arguments preceding the path list */

	if (!ctx) {
		shell_error(sh, "no lwm2m context yet\n");
		return -ENOEXEC;
	}

	if (argc < 2) {
		shell_error(sh, "no arguments or path(s)\n");
		shell_help(sh);
		return -EINVAL;
	}

	if (strcmp(argv[1], "-n") == 0) {
		confirmable = false;
		path_cnt--;
		ignore_cnt++;
	}

	if ((argc - ignore_cnt) == 0) {
		shell_error(sh, "no path(s)\n");
		shell_help(sh);
		return -EINVAL;
	}

	for (int idx = ignore_cnt; idx < argc; idx++) {
		if (argv[idx][0] < '0' || argv[idx][0] > '9') {
			shell_error(sh, "invalid path: %s\n", argv[idx]);
			shell_help(sh);
			return -EINVAL;
		}
	}

	ret = lwm2m_engine_send(ctx, (const char **)&(argv[ignore_cnt]),
				path_cnt, confirmable);
	if (ret < 0) {
		shell_error(sh, "can't do send operation, request failed\n");
		return -ENOEXEC;
	}
	return 0;
}

static int cmd_exec(const struct shell *sh, size_t argc, char **argv)
{
	struct lwm2m_ctx *ctx = lwm2m_rd_client_ctx();

	if (!ctx) {
		shell_error(sh, "no lwm2m context yet\n");
		return -ENOEXEC;
	}

	int ignore_cnt = 2; /* Subcmd + PATH */
	const char *pathstr = argv[1];
	struct lwm2m_obj_path path;
	int ret = lwm2m_string_to_path(pathstr, &path, '/'); /* translate path -> path_obj */

	if (ret < 0) {
		shell_error(sh, "Illegal path (PATH %s)\n", pathstr);
		return -EINVAL;
	}

	struct lwm2m_engine_res *res = lwm2m_engine_get_res(&path);

	if (res == NULL) {
		shell_error(sh, "Resource not found\n");
		return -EINVAL;
	}

	if (!res->execute_cb) {
		shell_error(sh, "No execute callback\n!");
		return -EINVAL;
	}

	ret = res->execute_cb(path.obj_inst_id, argv[ignore_cnt],
			      argc - ignore_cnt);
	if (ret < 0) {
		shell_error(sh, "returned (err %d)\n", ret);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_read(const struct shell *sh, size_t argc, char **argv)
{
	struct lwm2m_ctx *ctx = lwm2m_rd_client_ctx();

	if (!ctx) {
		shell_error(sh, "no lwm2m context yet\n");
		return -ENOEXEC;
	}

	if (argc < 2) {
		shell_error(sh, "no arguments or path(s)\n");
		shell_help(sh);
		return -EINVAL;
	}
	const char *dtype = "-s"; /* default */
	const char *pathstr = argv[1];
	int ret = 0;

	if (argc > 2) { /* read + path + options(data type) */
		dtype = argv[2];
	}
	if (strcmp(dtype, "-s") == 0) {
		const char *buff;
		uint16_t buff_len = 0;

		ret = lwm2m_engine_get_res_buf(pathstr, (void **)&buff,
					       &buff_len, NULL, NULL);
		if (ret != 0) {
			goto out;
		}
		shell_print(sh, "%s\n", buff);
	} else if (strcmp(dtype, "-s8") == 0) {
		int8_t temp = 0;

		ret = lwm2m_engine_get_s8(pathstr, &temp);
		if (ret != 0) {
			goto out;
		}
		shell_print(sh, "%d\n", temp);
	} else if (strcmp(dtype, "-s16") == 0) {
		int16_t temp = 0;

		ret = lwm2m_engine_get_s16(pathstr, &temp);
		if (ret != 0) {
			goto out;
		}
		shell_print(sh, "%d\n", temp);
	} else if (strcmp(dtype, "-s32") == 0) {
		int32_t temp = 0;

		ret = lwm2m_engine_get_s32(pathstr, &temp);
		if (ret != 0) {
			goto out;
		}
		shell_print(sh, "%d\n", temp);
	} else if (strcmp(dtype, "-s64") == 0) {
		int64_t temp = 0;

		ret = lwm2m_engine_get_s64(pathstr, &temp);
		if (ret != 0) {
			goto out;
		}
		shell_print(sh, "%lld\n", temp);
	} else if (strcmp(dtype, "-u8") == 0) {
		uint8_t temp = 0;

		ret = lwm2m_engine_get_u8(pathstr, &temp);
		if (ret != 0) {
			goto out;
		}
		shell_print(sh, "%d\n", temp);
	} else if (strcmp(dtype, "-u16") == 0) {
		uint16_t temp = 0;

		ret = lwm2m_engine_get_u16(pathstr, &temp);
		if (ret != 0) {
			goto out;
		}
		shell_print(sh, "%d\n", temp);
	} else if (strcmp(dtype, "-u32") == 0) {
		uint32_t temp = 0;

		ret = lwm2m_engine_get_u32(pathstr, &temp);
		if (ret != 0) {
			goto out;
		}
		shell_print(sh, "%d\n", temp);
	} else if (strcmp(dtype, "-u64") == 0) {
		uint64_t temp = 0;

		ret = lwm2m_engine_get_u64(pathstr, &temp);
		if (ret != 0) {
			goto out;
		}
		shell_print(sh, "%lld\n", temp);
	} else if (strcmp(dtype, "-f") == 0) {
		double temp = 0;

		ret = lwm2m_engine_get_float(pathstr, &temp);
		if (ret != 0) {
			goto out;
		}
		shell_print(sh, "%f\n", temp);
	} else if (strcmp(dtype, "-b") == 0) {
		bool temp;

		ret = lwm2m_engine_get_bool(pathstr, &temp);
		if (ret != 0) {
			goto out;
		}
		shell_print(sh, "%d\n", temp);
	} else {
		shell_error(sh, "can't recognize data type %s\n", dtype);
		shell_help(sh);
		return -EINVAL;
	}
	return 0;
out:
	shell_error(sh, "can't do read operation, request failed (err %d)\n", ret);
	return -EINVAL;
}

static int cmd_write(const struct shell *sh, size_t argc, char **argv)
{
	struct lwm2m_ctx *ctx = lwm2m_rd_client_ctx();

	if (!ctx) {
		shell_error(sh, "no lwm2m context yet\n");
		return -ENOEXEC;
	}

	if (argc < 3) {
		shell_error(sh, "no arguments or path(s)\n");
		shell_help(sh);
		return -EINVAL;
	}

	int ret = 0;
	const char *pathstr = argv[1];
	const char *dtype;
	char *value;

	if (argc == 4) { /* write path options value */
		dtype = argv[2];
		value = argv[3];
	} else { /* write path value */
		dtype = "-s";
		value = argv[2];
	}

	if (strcmp(dtype, "-s") == 0) {
		ret = lwm2m_engine_set_string(pathstr, value);
	} else if (strcmp(dtype, "-f") == 0) {
		double new = 0;

		lwm2m_atof(value, &new); /* Convert string -> float */
		ret = lwm2m_engine_set_float(pathstr, &new);
	} else { /* All the types using stdlib funcs*/
		char *e;

		if (strcmp(dtype, "-s8") == 0) {
			ret = lwm2m_engine_set_s8(pathstr,
						  strtol(value, &e, 10));
		} else if (strcmp(dtype, "-s16") == 0) {
			ret = lwm2m_engine_set_s16(pathstr,
						   strtol(value, &e, 10));
		} else if (strcmp(dtype, "-s32") == 0) {
			ret = lwm2m_engine_set_s32(pathstr,
						   strtol(value, &e, 10));
		} else if (strcmp(dtype, "-s64") == 0) {
			ret = lwm2m_engine_set_s64(pathstr,
						   strtoll(value, &e, 10));
		} else if (strcmp(dtype, "-u8") == 0) {
			ret = lwm2m_engine_set_u8(pathstr,
						  strtoul(value, &e, 10));
		} else if (strcmp(dtype, "-u16") == 0) {
			ret = lwm2m_engine_set_u16(pathstr,
						   strtoul(value, &e, 10));
		} else if (strcmp(dtype, "-u32") == 0) {
			ret = lwm2m_engine_set_u32(pathstr,
						   strtoul(value, &e, 10));
		} else if (strcmp(dtype, "-u64") == 0) {
			ret = lwm2m_engine_set_u64(pathstr,
						   strtoull(value, &e, 10));
		} else if (strcmp(dtype, "-b") == 0) {
			ret = lwm2m_engine_set_bool(pathstr,
						    strtoul(value, &e, 10));
		} else {
			shell_error(sh, "can't recognize data type %s\n",
				    dtype);
			shell_help(sh);
			return -EINVAL;
		}
		if (*e != '\0') {
			shell_error(sh, "Invalid number: %s\n", value);
			shell_help(sh);
			return -EINVAL;
		}
	}

	if (ret < 0) {
		shell_error(
			sh,
			"can't do write operation, request failed (err %d)\n",
			ret);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_start(const struct shell *sh, size_t argc, char **argv)
{
	struct lwm2m_ctx *ctx = lwm2m_rd_client_ctx();

	if (!ctx) {
		shell_error(sh, "no lwm2m context yet\n");
		return -ENOEXEC;
	}
	uint32_t bootstrap_flag = 0;

	if (argc == 3) {
		shell_error(sh, "no specifier or value\n");
		shell_help(sh);
		return -EINVAL;
	} else if (argc == 4) {
		if (strcmp(argv[2], "-b") != 0) {
			shell_error(sh, "unknown specifier %s\n", argv[2]);
			shell_help(sh);
			return -EINVAL;
		}

		char *e;

		bootstrap_flag = strtol(argv[3], &e, 10);
		if (*e != '\0') {
			shell_error(sh, "Invalid number: %s\n", argv[3]);
			shell_help(sh);
			return -EINVAL;
		}
	}
	int ret = lwm2m_rd_client_start(ctx, argv[1], bootstrap_flag,
					ctx->event_cb, ctx->observe_cb);
	if (ret < 0) {
		shell_error(
			sh,
			"can't do start operation, request failed (err %d)\n",
			ret);
		return -ENOEXEC;
	}
	return 0;
}

static int cmd_stop(const struct shell *sh, size_t argc, char **argv)
{
	struct lwm2m_ctx *ctx = lwm2m_rd_client_ctx();

	if (!ctx) {
		shell_error(sh, "no lwm2m context yet\n");
		return -ENOEXEC;
	}
	bool forcefully = true;

	if (argc == 2) {
		if (strcmp(argv[1], "-f") != 0) {
			shell_error(sh, "can't recognize specifier %s\n",
				    argv[1]);
			shell_help(sh);
			return -EINVAL;
		}
		forcefully = false;
	}
	int ret = lwm2m_rd_client_stop(ctx, ctx->event_cb, forcefully);

	if (ret < 0) {
		shell_error(
			sh,
			"can't do stop operation, request failed (err %d)\n",
			ret);
		return -ENOEXEC;
	}
	return 0;
}

static int cmd_update(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	struct lwm2m_ctx *ctx = lwm2m_rd_client_ctx();

	if (!ctx) {
		shell_error(sh, "no lwm2m context yet\n");
		return -ENOEXEC;
	}
	lwm2m_rd_client_update();
	return 0;
}

static int cmd_pause(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return lwm2m_engine_pause();
}

static int cmd_resume(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return lwm2m_engine_resume();
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_lwm2m,
	SHELL_COND_CMD_ARG(CONFIG_LWM2M_VERSION_1_1, send, NULL,
			   LWM2M_HELP_SEND, cmd_send, 1, 9),
	SHELL_CMD_ARG(exec, NULL, LWM2M_HELP_EXEC, cmd_exec, 2, 9),
	SHELL_CMD_ARG(read, NULL, LWM2M_HELP_READ, cmd_read, 2, 1),
	SHELL_CMD_ARG(write, NULL, LWM2M_HELP_WRITE, cmd_write, 3, 1),
	SHELL_CMD_ARG(start, NULL, LWM2M_HELP_START, cmd_start, 2, 2),
	SHELL_CMD_ARG(stop, NULL, LWM2M_HELP_STOP, cmd_stop, 1, 1),
	SHELL_CMD_ARG(update, NULL, LWM2M_HELP_UPDATE, cmd_update, 1, 0),
	SHELL_CMD_ARG(pause, NULL, LWM2M_HELP_PAUSE, cmd_pause, 1, 0),
	SHELL_CMD_ARG(resume, NULL, LWM2M_HELP_RESUME, cmd_resume, 1, 0),
	SHELL_SUBCMD_SET_END);
SHELL_COND_CMD_ARG_REGISTER(CONFIG_LWM2M_SHELL, lwm2m, &sub_lwm2m,
			    LWM2M_HELP_CMD, NULL, 1, 0);
