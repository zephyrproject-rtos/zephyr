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
#define LWM2M_HELP_SEND "send PATHS\nLwM2M SEND operation\n"
#define LWM2M_HELP_EXEC "exec PATH [PARAM]\nExecute a resource\n"
#define LWM2M_HELP_READ "read PATH [OPTIONS]\nRead value from LwM2M resource\n" \
	"-x \tRead value as hex stream (default)\n" \
	"-s \tRead value as string\n" \
	"-b \tRead value as bool (1/0)\n" \
	"-uX\tRead value as uintX_t\n" \
	"-sX\tRead value as intX_t\n" \
	"-f \tRead value as float\n" \
	"-t \tRead value as time_t\n"
#define LWM2M_HELP_WRITE "write PATH [OPTIONS] VALUE\nWrite into LwM2M resource\n" \
	"-s \tWrite value as string (default)\n" \
	"-b \tWrite value as bool\n" \
	"-uX\tWrite value as uintX_t\n" \
	"-sX\tWrite value as intX_t\n" \
	"-f \tWrite value as float\n" \
	"-t \tWrite value as time_t\n"
#define LWM2M_HELP_CREATE "create PATH\nCreate object instance\n"
#define LWM2M_HELP_START "start EP_NAME [BOOTSTRAP FLAG]\n" \
	"Start the LwM2M RD (Registration / Discovery) Client\n" \
	"-b \tSet the bootstrap flag (default 0)\n"
#define LWM2M_HELP_STOP "stop [OPTIONS]\nStop the LwM2M RD (De-register) Client\n" \
	"-f \tForce close the connection\n"
#define LWM2M_HELP_UPDATE "Trigger Registration Update of the LwM2M RD Client\n"
#define LWM2M_HELP_PAUSE "LwM2M engine thread pause"
#define LWM2M_HELP_RESUME "LwM2M engine thread resume"
#define LWM2M_HELP_LOCK "Lock the LwM2M registry"
#define LWM2M_HELP_UNLOCK "Unlock the LwM2M registry"
#define LWM2M_HELP_CACHE "cache PATH NUM\nEnable data cache for resource\n" \
	"PATH is LwM2M path\n" \
	"NUM how many elements to cache\n" \

static int cmd_send(const struct shell *sh, size_t argc, char **argv)
{
	int ret = 0;
	struct lwm2m_ctx *ctx = lwm2m_rd_client_ctx();
	int path_cnt = argc - 1;
	struct lwm2m_obj_path lwm2m_path_list[CONFIG_LWM2M_COMPOSITE_PATH_LIST_SIZE];

	if (!ctx) {
		shell_error(sh, "no lwm2m context yet\n");
		return -ENOEXEC;
	}

	if (argc < 2) {
		shell_error(sh, "no path(s)\n");
		shell_help(sh);
		return -EINVAL;
	}

	if (path_cnt > CONFIG_LWM2M_COMPOSITE_PATH_LIST_SIZE) {
		return -E2BIG;
	}

	for (int i = 0; i < path_cnt; i++) {
		const char *p = argv[1 + i];
		/* translate path -> path_obj */
		ret = lwm2m_string_to_path(p, &lwm2m_path_list[i], '/');
		if (ret < 0) {
			return ret;
		}
	}

	ret = lwm2m_send_cb(ctx, lwm2m_path_list, path_cnt, NULL);

	if (ret < 0) {
		shell_error(sh, "can't do send operation, request failed (%d)\n", ret);
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

	/* 0: exec, 1:<path> 2:[<param>] */
	char *param = (argc == 3) ? argv[2] : NULL;
	size_t param_len = param ? strlen(param) + 1 : 0;

	ret = res->execute_cb(path.obj_inst_id, param, param_len);
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
	const char *dtype = "-x"; /* default */
	const char *pathstr = argv[1];
	int ret = 0;
	struct lwm2m_obj_path path;

	ret = lwm2m_string_to_path(pathstr, &path, '/');
	if (ret < 0) {
		return ret;
	}

	if (argc > 2) { /* read + path + options(data type) */
		dtype = argv[2];
	}
	if (strcmp(dtype, "-x") == 0) {
		const char *buff;
		uint16_t buff_len = 0;

		ret = lwm2m_get_res_buf(&path, (void **)&buff,
					NULL, &buff_len, NULL);
		if (ret != 0) {
			goto out;
		}
		shell_hexdump(sh, buff, buff_len);
	} else if (strcmp(dtype, "-s") == 0) {
		const char *buff;
		uint16_t buff_len = 0;

		ret = lwm2m_get_res_buf(&path, (void **)&buff,
					NULL, &buff_len, NULL);
		if (ret != 0) {
			goto out;
		}
		shell_print(sh, "%.*s\n", buff_len, buff);
	} else if (strcmp(dtype, "-s8") == 0) {
		int8_t temp = 0;

		ret = lwm2m_get_s8(&path, &temp);
		if (ret != 0) {
			goto out;
		}
		shell_print(sh, "%d\n", temp);
	} else if (strcmp(dtype, "-s16") == 0) {
		int16_t temp = 0;

		ret = lwm2m_get_s16(&path, &temp);
		if (ret != 0) {
			goto out;
		}
		shell_print(sh, "%d\n", temp);
	} else if (strcmp(dtype, "-s32") == 0) {
		int32_t temp = 0;

		ret = lwm2m_get_s32(&path, &temp);
		if (ret != 0) {
			goto out;
		}
		shell_print(sh, "%d\n", temp);
	} else if (strcmp(dtype, "-s64") == 0) {
		int64_t temp = 0;

		ret = lwm2m_get_s64(&path, &temp);
		if (ret != 0) {
			goto out;
		}
		shell_print(sh, "%lld\n", temp);
	} else if (strcmp(dtype, "-u8") == 0) {
		uint8_t temp = 0;

		ret = lwm2m_get_u8(&path, &temp);
		if (ret != 0) {
			goto out;
		}
		shell_print(sh, "%d\n", temp);
	} else if (strcmp(dtype, "-u16") == 0) {
		uint16_t temp = 0;

		ret = lwm2m_get_u16(&path, &temp);
		if (ret != 0) {
			goto out;
		}
		shell_print(sh, "%d\n", temp);
	} else if (strcmp(dtype, "-u32") == 0) {
		uint32_t temp = 0;

		ret = lwm2m_get_u32(&path, &temp);
		if (ret != 0) {
			goto out;
		}
		shell_print(sh, "%d\n", temp);
	} else if (strcmp(dtype, "-u64") == 0) {
		uint64_t temp = 0;

		ret = lwm2m_get_u64(&path, &temp);
		if (ret != 0) {
			goto out;
		}
		shell_print(sh, "%lld\n", temp);
	} else if (strcmp(dtype, "-f") == 0) {
		double temp = 0;

		ret = lwm2m_get_f64(&path, &temp);
		if (ret != 0) {
			goto out;
		}
		shell_print(sh, "%f\n", temp);
	} else if (strcmp(dtype, "-b") == 0) {
		bool temp;

		ret = lwm2m_get_bool(&path, &temp);
		if (ret != 0) {
			goto out;
		}
		shell_print(sh, "%d\n", temp);
	} else if (strcmp(dtype, "-t") == 0) {
		time_t temp;

		ret = lwm2m_get_time(&path, &temp);
		if (ret != 0) {
			goto out;
		}
		shell_print(sh, "%lld\n", temp);
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
	struct lwm2m_obj_path path;

	ret = lwm2m_string_to_path(pathstr, &path, '/');
	if (ret < 0) {
		return ret;
	}

	if (argc == 4) { /* write path options value */
		dtype = argv[2];
		value = argv[3];
	} else { /* write path value */
		dtype = "-s";
		value = argv[2];
	}

	if (strcmp(dtype, "-s") == 0) {
		ret = lwm2m_set_string(&path, value);
	} else if (strcmp(dtype, "-f") == 0) {
		double new = 0;

		lwm2m_atof(value, &new); /* Convert string -> float */
		ret = lwm2m_set_f64(&path, new);
	} else { /* All the types using stdlib funcs*/
		char *e;

		if (strcmp(dtype, "-s8") == 0) {
			ret = lwm2m_set_s8(&path, strtol(value, &e, 10));
		} else if (strcmp(dtype, "-s16") == 0) {
			ret = lwm2m_set_s16(&path, strtol(value, &e, 10));
		} else if (strcmp(dtype, "-s32") == 0) {
			ret = lwm2m_set_s32(&path, strtol(value, &e, 10));
		} else if (strcmp(dtype, "-s64") == 0) {
			ret = lwm2m_set_s64(&path, strtoll(value, &e, 10));
		} else if (strcmp(dtype, "-u8") == 0) {
			ret = lwm2m_set_u8(&path, strtoul(value, &e, 10));
		} else if (strcmp(dtype, "-u16") == 0) {
			ret = lwm2m_set_u16(&path, strtoul(value, &e, 10));
		} else if (strcmp(dtype, "-u32") == 0) {
			ret = lwm2m_set_u32(&path, strtoul(value, &e, 10));
		} else if (strcmp(dtype, "-u64") == 0) {
			ret = lwm2m_set_u64(&path, strtoull(value, &e, 10));
		} else if (strcmp(dtype, "-b") == 0) {
			ret = lwm2m_set_bool(&path, strtoul(value, &e, 10));
		} else if (strcmp(dtype, "-t") == 0) {
			ret = lwm2m_set_time(&path, strtoll(value, &e, 10));
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

static int cmd_create(const struct shell *sh, size_t argc, char **argv)
{
	struct lwm2m_obj_path path;
	struct lwm2m_engine_obj_inst *obj_inst;
	int ret;

	if (argc < 2) {
		shell_error(sh, "No object ID given\n");
		shell_help(sh);
		return -EINVAL;
	}

	ret = lwm2m_string_to_path(argv[1], &path, '/');
	if (ret < 0) {
		shell_error(sh, "failed to read path (%d)\n", ret);
		return -ENOEXEC;
	}

	if (path.level != LWM2M_PATH_LEVEL_OBJECT_INST) {
		shell_error(sh, "path is not an object instance\n");
		shell_help(sh);
		return -EINVAL;
	}

	ret = lwm2m_create_obj_inst(path.obj_id, path.obj_inst_id, &obj_inst);
	if (ret < 0) {
		shell_error(sh, "Failed to create object instance %d/%d, ret=%d", path.obj_id,
			    path.obj_inst_id, ret);
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

static int cmd_lock(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	lwm2m_registry_lock();
	return 0;
}

static int cmd_unlock(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	lwm2m_registry_unlock();
	return 0;
}

static int cmd_cache(const struct shell *sh, size_t argc, char **argv)
{
#if (CONFIG_HEAP_MEM_POOL_SIZE > 0)
	int rc;
	int elems;
	struct lwm2m_time_series_elem *cache;
	struct lwm2m_obj_path obj_path;

	if (argc != 3) {
		shell_error(sh, "wrong parameters\n");
		return -EINVAL;
	}

	/* translate path -> path_obj */
	rc = lwm2m_string_to_path(argv[1], &obj_path, '/');
	if (rc < 0) {
		return rc;
	}

	if (obj_path.level < 3) {
		shell_error(sh, "Path string not correct\n");
		return -EINVAL;
	}

	if (lwm2m_cache_entry_get_by_object(&obj_path)) {
		shell_error(sh, "Cache already enabled for %s\n", argv[1]);
		return -ENOEXEC;
	}

	elems = atoi(argv[2]);
	if (elems < 1) {
		shell_error(sh, "Size must be 1 or more (given %d)\n", elems);
		return -EINVAL;
	}

	cache = k_malloc(sizeof(struct lwm2m_time_series_elem) * elems);
	if (!cache) {
		shell_error(sh, "Out of memory\n");
		return -ENOEXEC;
	}

	rc = lwm2m_enable_cache(&obj_path, cache, elems);
	if (rc) {
		shell_error(sh, "lwm2m_enable_cache(%u/%u/%u/%u, %p, %d) returned %d\n",
			    obj_path.obj_id, obj_path.obj_inst_id, obj_path.res_id,
			    obj_path.res_inst_id, cache, elems, rc);
		k_free(cache);
		return -ENOEXEC;
	}

	return 0;
#else
	shell_error(sh, "No heap configured\n");
	return -ENOEXEC;
#endif
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_lwm2m,
	SHELL_COND_CMD_ARG(CONFIG_LWM2M_VERSION_1_1, send, NULL,
			   LWM2M_HELP_SEND, cmd_send, 1, 9),
	SHELL_CMD_ARG(exec, NULL, LWM2M_HELP_EXEC, cmd_exec, 2, 1),
	SHELL_CMD_ARG(read, NULL, LWM2M_HELP_READ, cmd_read, 2, 1),
	SHELL_CMD_ARG(write, NULL, LWM2M_HELP_WRITE, cmd_write, 3, 1),
	SHELL_CMD_ARG(create, NULL, LWM2M_HELP_CREATE, cmd_create, 2, 0),
	SHELL_CMD_ARG(cache, NULL, LWM2M_HELP_CACHE, cmd_cache, 3, 0),
	SHELL_CMD_ARG(start, NULL, LWM2M_HELP_START, cmd_start, 2, 2),
	SHELL_CMD_ARG(stop, NULL, LWM2M_HELP_STOP, cmd_stop, 1, 1),
	SHELL_CMD_ARG(update, NULL, LWM2M_HELP_UPDATE, cmd_update, 1, 0),
	SHELL_CMD_ARG(pause, NULL, LWM2M_HELP_PAUSE, cmd_pause, 1, 0),
	SHELL_CMD_ARG(resume, NULL, LWM2M_HELP_RESUME, cmd_resume, 1, 0),
	SHELL_CMD_ARG(lock, NULL, LWM2M_HELP_LOCK, cmd_lock, 1, 0),
	SHELL_CMD_ARG(unlock, NULL, LWM2M_HELP_UNLOCK, cmd_unlock, 1, 0),

	SHELL_SUBCMD_SET_END);
SHELL_COND_CMD_ARG_REGISTER(CONFIG_LWM2M_SHELL, lwm2m, &sub_lwm2m,
			    LWM2M_HELP_CMD, NULL, 1, 0);
