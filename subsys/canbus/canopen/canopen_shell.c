/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/shell/shell.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>

#include "canopen_internal.h"

#define SHELL_BUF_SIZE CONFIG_CANOPEN_SDO_BUFFER_SIZE

static sys_slist_t shell_nodes = SYS_SLIST_STATIC_INIT(&shell_nodes);

static const struct {
	const char *name;
	enum canopen_nmt_cmd cmd;
} shell_nmt_cmds[] = {
	{"start", CANOPEN_NMT_CMD_START},
	{"stop", CANOPEN_NMT_CMD_STOP},
	{"preop", CANOPEN_NMT_CMD_ENTER_PRE_OPERATIONAL},
	{"reset", CANOPEN_NMT_CMD_RESET_NODE},
	{"resetcomm", CANOPEN_NMT_CMD_RESET_COMMUNICATION},
};

static const struct {
	const char *name;
	uint8_t size;
	bool is_signed;
} shell_types[] = {
	{"u8", 1U, false},  {"u16", 2U, false}, {"u32", 4U, false}, {"u64", 8U, false},
	{"i8", 1U, true},   {"i16", 2U, true},  {"i32", 4U, true},  {"i64", 8U, true},
};

void canopen_shell_register(struct canopen_node *node)
{
	if (!sys_slist_find(&shell_nodes, &node->shell_node, NULL)) {
		sys_slist_append(&shell_nodes, &node->shell_node);
	}
}

static int shell_number(const struct shell *sh, const char *arg, unsigned long max,
			unsigned long *value)
{
	int err = 0;

	*value = shell_strtoul(arg, 0, &err);
	if ((err != 0) || (*value > max)) {
		shell_error(sh, "invalid value: %s", arg);
		return -EINVAL;
	}

	return 0;
}

static struct canopen_node *shell_node(const struct shell *sh, const char *arg)
{
	struct canopen_node *node;
	unsigned long id;

	if (shell_number(sh, arg, UINT8_MAX, &id) != 0) {
		return NULL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&shell_nodes, node, shell_node) {
		if (node->cfg->node_id == id) {
			return node;
		}
	}

	shell_error(sh, "no local node 0x%02lx", id);

	return NULL;
}

static int shell_mux(const struct shell *sh, char **argv, uint16_t *index, uint8_t *sub)
{
	unsigned long value;

	if (shell_number(sh, argv[0], UINT16_MAX, &value) != 0) {
		return -EINVAL;
	}

	*index = (uint16_t)value;

	if (shell_number(sh, argv[1], UINT8_MAX, &value) != 0) {
		return -EINVAL;
	}

	*sub = (uint8_t)value;

	return 0;
}

static int shell_integer(const struct shell *sh, const char *arg, size_t size, bool is_signed,
			 uint8_t *buf)
{
	unsigned int bits = size * 8U;
	bool negative = is_signed && (arg[0] == '-');
	uint64_t limit = (bits == 64U) ? UINT64_MAX : (BIT64(bits) - 1U);
	uint64_t value;
	int err = 0;

	value = shell_strtoull(negative ? &arg[1] : arg, 0, &err);
	if (is_signed) {
		limit >>= 1;
		if (negative) {
			limit++;
		}
	}

	if ((err != 0) || (value > limit)) {
		shell_error(sh, "invalid value: %s", arg);
		return -EINVAL;
	}

	sys_put_le64(negative ? (0U - value) : value, buf);

	return 0;
}

static void shell_print_value(const struct shell *sh, uint8_t type, const uint8_t *buf,
			      size_t len)
{
	size_t size = canopen_od_type_size(type);
	bool is_signed = (type == CANOPEN_OD_TYPE_INTEGER8) ||
			 (type == CANOPEN_OD_TYPE_INTEGER16) ||
			 (type == CANOPEN_OD_TYPE_INTEGER24) || (type == CANOPEN_OD_TYPE_INTEGER32);
	uint32_t value = 0U;

	if (type == CANOPEN_OD_TYPE_VISIBLE_STRING) {
		shell_print(sh, "\"%.*s\"", (int)len, (const char *)buf);
		return;
	}

	if ((size == 0U) || (size > sizeof(uint32_t)) || (type == CANOPEN_OD_TYPE_REAL32)) {
		shell_hexdump(sh, buf, len);
		return;
	}

	for (size_t i = size; i > 0U; i--) {
		value = (value << 8) | buf[i - 1U];
	}

	if (is_signed && (size < sizeof(uint32_t)) && ((value & BIT((size * 8U) - 1U)) != 0U)) {
		value |= GENMASK(31, size * 8U);
	}

	if (is_signed) {
		shell_print(sh, "%d (0x%0*x)", (int32_t)value, (int)(size * 2U),
			    (unsigned int)(value & GENMASK((size * 8U) - 1U, 0)));
	} else {
		shell_print(sh, "%u (0x%0*x)", value, (int)(size * 2U), value);
	}
}

static void shell_abort(const struct shell *sh, uint32_t abort)
{
	shell_error(sh, "abort 0x%08x", abort);
}

static const char *shell_state(uint8_t state)
{
	switch (state) {
	case CANOPEN_NMT_STOPPED:
		return "STOPPED";
	case CANOPEN_NMT_OPERATIONAL:
		return "OPERATIONAL";
	case CANOPEN_NMT_PRE_OPERATIONAL:
		return "PRE-OPERATIONAL";
	default:
		return "INITIALISING";
	}
}

static int cmd_list(const struct shell *sh, size_t argc, char **argv)
{
	struct canopen_node *node;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	SYS_SLIST_FOR_EACH_CONTAINER(&shell_nodes, node, shell_node) {
		shell_print(sh, "0x%02x %s error register 0x%02x %s", node->cfg->node_id,
			    shell_state(node->nmt_state), node->error_register,
			    node->cfg->can->name);
	}

	return 0;
}

static int cmd_nmt(const struct shell *sh, size_t argc, char **argv)
{
	struct canopen_node *node = shell_node(sh, argv[1]);
	unsigned long target;
	int err = -EINVAL;

	if (node == NULL) {
		return -EINVAL;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(shell_nmt_cmds); i++) {
		if (strcmp(argv[2], shell_nmt_cmds[i].name) != 0) {
			continue;
		}

		if (argc == 3) {
			err = canopen_nmt_local_cmd(node, shell_nmt_cmds[i].cmd);
		} else if (!IS_ENABLED(CONFIG_CANOPEN_MASTER)) {
			shell_error(sh, "sending NMT commands requires CONFIG_CANOPEN_MASTER");
			return -ENOTSUP;
		} else if (shell_number(sh, argv[3], 127U, &target) == 0) {
			err = canopen_nmt_send(node, (uint8_t)target, shell_nmt_cmds[i].cmd);
		} else {
			return -EINVAL;
		}

		if (err != 0) {
			shell_error(sh, "failed (%d)", err);
		}

		return err;
	}

	shell_error(sh, "unknown command: %s", argv[2]);

	return -EINVAL;
}

static int cmd_read(const struct shell *sh, size_t argc, char **argv)
{
	struct canopen_node *node = shell_node(sh, argv[1]);
	uint8_t buf[SHELL_BUF_SIZE];
	struct canopen_od_ref ref;
	size_t len = 0U;
	uint32_t abort;
	uint16_t index;
	uint8_t sub;

	ARG_UNUSED(argc);

	if ((node == NULL) || (shell_mux(sh, &argv[2], &index, &sub) != 0)) {
		return -EINVAL;
	}

	k_mutex_lock(&node->lock, K_FOREVER);
	abort = canopen_od_find(node, index, sub, &ref);
	if ((abort == CANOPEN_SDO_ABORT_NONE) && ((ref.attr & CANOPEN_OD_ATTR_READ) == 0U)) {
		abort = CANOPEN_SDO_ABORT_WRITE_ONLY;
	}
	if (abort == CANOPEN_SDO_ABORT_NONE) {
		abort = canopen_od_read_wire(&ref, buf, sizeof(buf), &len);
	}
	k_mutex_unlock(&node->lock);

	if (abort != CANOPEN_SDO_ABORT_NONE) {
		shell_abort(sh, abort);
		return -EIO;
	}

	shell_print_value(sh, ref.type, buf, len);

	return 0;
}

static int shell_parse_value(const struct shell *sh, uint8_t type, const char *arg, uint8_t *buf,
			     size_t *len)
{
	size_t size = canopen_od_type_size(type);

	if (type == CANOPEN_OD_TYPE_VISIBLE_STRING) {
		*len = strlen(arg);
		if (*len > SHELL_BUF_SIZE) {
			shell_error(sh, "value too long");
			return -EINVAL;
		}
		memcpy(buf, arg, *len);
		return 0;
	}

	if ((size == 0U) || (type == CANOPEN_OD_TYPE_REAL32) ||
	    (type == CANOPEN_OD_TYPE_REAL64)) {
		*len = hex2bin(arg, strlen(arg), buf, SHELL_BUF_SIZE);
		if (*len == 0U) {
			shell_error(sh, "invalid hex bytes: %s", arg);
			return -EINVAL;
		}
		return 0;
	}

	*len = size;

	return shell_integer(sh, arg, size,
			     (type == CANOPEN_OD_TYPE_INTEGER8) ||
				     (type == CANOPEN_OD_TYPE_INTEGER16) ||
				     (type == CANOPEN_OD_TYPE_INTEGER24) ||
				     (type == CANOPEN_OD_TYPE_INTEGER32) ||
				     (type == CANOPEN_OD_TYPE_INTEGER40) ||
				     (type == CANOPEN_OD_TYPE_INTEGER48) ||
				     (type == CANOPEN_OD_TYPE_INTEGER56) ||
				     (type == CANOPEN_OD_TYPE_INTEGER64),
			     buf);
}

static int cmd_write(const struct shell *sh, size_t argc, char **argv)
{
	struct canopen_node *node = shell_node(sh, argv[1]);
	uint8_t buf[MAX(SHELL_BUF_SIZE, sizeof(uint64_t))];
	struct canopen_od_ref ref;
	size_t len = 0U;
	uint32_t abort;
	uint16_t index;
	uint8_t sub;
	int err = 0;

	ARG_UNUSED(argc);

	if ((node == NULL) || (shell_mux(sh, &argv[2], &index, &sub) != 0)) {
		return -EINVAL;
	}

	k_mutex_lock(&node->lock, K_FOREVER);
	abort = canopen_od_find(node, index, sub, &ref);
	if ((abort == CANOPEN_SDO_ABORT_NONE) && ((ref.attr & CANOPEN_OD_ATTR_WRITE) == 0U)) {
		abort = CANOPEN_SDO_ABORT_READ_ONLY;
	}
	if (abort == CANOPEN_SDO_ABORT_NONE) {
		err = shell_parse_value(sh, ref.type, argv[4], buf, &len);
		if (err == 0) {
			abort = canopen_od_write_wire(node, &ref, buf, len);
		}
	}
	k_mutex_unlock(&node->lock);

	if (err != 0) {
		return err;
	}

	if (abort != CANOPEN_SDO_ABORT_NONE) {
		shell_abort(sh, abort);
		return -EIO;
	}

	return 0;
}

static int cmd_emcy(const struct shell *sh, size_t argc, char **argv)
{
	struct canopen_node *node = shell_node(sh, argv[1]);

	ARG_UNUSED(argc);

	if (node == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&node->lock, K_FOREVER);
	shell_print(sh, "error register 0x%02x, %u errors", node->error_register,
		    node->emcy.history_count);
	for (uint8_t i = 0U; i < node->emcy.history_count; i++) {
		shell_print(sh, "%u: 0x%08x", i + 1U, node->emcy.history[i]);
	}
	k_mutex_unlock(&node->lock);

	return 0;
}

static int cmd_sdo_read(const struct shell *sh, size_t argc, char **argv)
{
	struct canopen_node *node = shell_node(sh, argv[1]);
	uint8_t buf[SHELL_BUF_SIZE];
	size_t len = sizeof(buf);
	unsigned long server;
	uint32_t abort = 0U;
	uint16_t index;
	uint8_t sub;
	int err;

	ARG_UNUSED(argc);

	if ((node == NULL) || (shell_number(sh, argv[2], 127U, &server) != 0) ||
	    (shell_mux(sh, &argv[3], &index, &sub) != 0)) {
		return -EINVAL;
	}

	err = canopen_sdo_upload(node, (uint8_t)server, index, sub, buf, &len, &abort);
	if (err == -ECANCELED) {
		shell_abort(sh, abort);
		return err;
	} else if (err != 0) {
		shell_error(sh, "failed (%d)", err);
		return err;
	}

	shell_hexdump(sh, buf, len);

	return 0;
}

static int cmd_sdo_write(const struct shell *sh, size_t argc, char **argv)
{
	struct canopen_node *node = shell_node(sh, argv[1]);
	uint8_t buf[MAX(SHELL_BUF_SIZE, sizeof(uint64_t))];
	unsigned long server;
	uint32_t abort = 0U;
	size_t len = 0U;
	uint16_t index;
	uint8_t sub;
	int err = -EINVAL;

	ARG_UNUSED(argc);

	if ((node == NULL) || (shell_number(sh, argv[2], 127U, &server) != 0) ||
	    (shell_mux(sh, &argv[3], &index, &sub) != 0)) {
		return -EINVAL;
	}

	if (strcmp(argv[5], "str") == 0) {
		err = shell_parse_value(sh, CANOPEN_OD_TYPE_VISIBLE_STRING, argv[6], buf, &len);
	} else if (strcmp(argv[5], "hex") == 0) {
		err = shell_parse_value(sh, CANOPEN_OD_TYPE_OCTET_STRING, argv[6], buf, &len);
	} else {
		for (size_t i = 0U; i < ARRAY_SIZE(shell_types); i++) {
			if (strcmp(argv[5], shell_types[i].name) == 0) {
				len = shell_types[i].size;
				err = shell_integer(sh, argv[6], len, shell_types[i].is_signed,
						    buf);
				break;
			}
		}
	}

	if (len == 0U) {
		shell_error(sh, "unknown type: %s", argv[5]);
	}

	if (err != 0) {
		return err;
	}

	err = canopen_sdo_download(node, (uint8_t)server, index, sub, buf, len, &abort);
	if (err == -ECANCELED) {
		shell_abort(sh, abort);
	} else if (err != 0) {
		shell_error(sh, "failed (%d)", err);
	}

	return err;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_canopen_sdo,
	SHELL_CMD_ARG(read, NULL,
		SHELL_HELP("Read an object of another node",
			   "<node> <server> <index> <sub>"),
		cmd_sdo_read, 5, 0),
	SHELL_CMD_ARG(write, NULL,
		SHELL_HELP("Write an object of another node",
			   "<node> <server> <index> <sub> <u8|u16|u32|u64|i8|i16|i32|i64|str|hex> "
			   "<value>"),
		cmd_sdo_write, 7, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_canopen,
	SHELL_CMD_ARG(list, NULL,
		SHELL_HELP("List the local nodes", NULL),
		cmd_list, 1, 0),
	SHELL_CMD_ARG(nmt, NULL,
		SHELL_HELP("Change the NMT state of the node, or send an NMT command to a target",
			   "<node> <start|stop|preop|reset|resetcomm> [target]"),
		cmd_nmt, 3, 1),
	SHELL_CMD_ARG(read, NULL,
		SHELL_HELP("Read an object of the node", "<node> <index> <sub>"),
		cmd_read, 4, 0),
	SHELL_CMD_ARG(write, NULL,
		SHELL_HELP("Write an object of the node", "<node> <index> <sub> <value>"),
		cmd_write, 5, 0),
	SHELL_CMD_ARG(emcy, NULL,
		SHELL_HELP("Show the error register and error history", "<node>"),
		cmd_emcy, 2, 0),
	SHELL_COND_CMD(CONFIG_CANOPEN_MASTER, sdo, &sub_canopen_sdo,
		SHELL_HELP("SDO client commands", "<read|write> ..."),
		NULL),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(canopen, &sub_canopen, "CANopen commands", NULL);
