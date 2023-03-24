/*
 * Copyright (c) 2023 Lucas Denefle
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/drivers/retained_mem.h>
#include <zephyr/debug/coredump.h>
#include "coredump_internal.h"

const static struct device *coredump_retained_mem_dev =
	DEVICE_DT_GET(DT_ALIAS(coredump_retained_mem));
static size_t written;
static int error;

struct retainem_mem_hdr_t {
	/* 'C', 'D' */
	char magic[2];

	/* Size of the coredump */
	size_t size;

	/* Error on previous attempt to dump coredump */
	int error;
};

static int write_to_mem(uint8_t *buf, size_t buflen)
{
	int ret;
	size_t offset = written + sizeof(struct retainem_mem_hdr_t);

	/* Check if the current buffer is not going to overwrite the retained memory size */
	if ((buflen + offset) >= retained_mem_size(coredump_retained_mem_dev)) {
		return -ENOMEM;
	}

	ret = retained_mem_write(coredump_retained_mem_dev, offset, buf, buflen);

	/* If we successfully wrote to retained memory, increment our written size */
	if (!ret) {
		written += buflen;
	}

	return ret;
}

static void coredump_retained_mem_backend_start(void)
{
	/* Clear error on start up*/
	error = 0;

	/* Reset size written in coredump */
	written = 0;
}

static void coredump_retained_mem_backend_end(void)
{
	struct retainem_mem_hdr_t hdr = {
		.magic = {'C', 'D'},
		.size = written,
		.error = error,
	};

	retained_mem_write(coredump_retained_mem_dev, 0, (const uint8_t *)&hdr,
			   sizeof(struct retainem_mem_hdr_t));
}

static void coredump_retained_mem_backend_buffer_output(uint8_t *buf, size_t buflen)
{
	error = write_to_mem(buf, buflen);
}

static int coredump_retained_mem_backend_query(enum coredump_query_id query_id, void *arg)
{
	int rc;
	int ret;
	struct retainem_mem_hdr_t hdr;

	switch (query_id) {
	case COREDUMP_QUERY_GET_ERROR:
		ret = error;
		break;
	case COREDUMP_QUERY_HAS_STORED_DUMP:
		rc = retained_mem_read(coredump_retained_mem_dev, 0, (uint8_t *)&hdr, sizeof(hdr));
		ret = (hdr.magic[0] == 'C') && (hdr.magic[1] == 'D') && (hdr.error == 0) ? 1 : 0;
		break;
	case COREDUMP_QUERY_GET_STORED_DUMP_SIZE:
		rc = retained_mem_read(coredump_retained_mem_dev, 0, (uint8_t *)&hdr, sizeof(hdr));
		ret = hdr.size;
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static int coredump_retained_mem_backend_cmd(enum coredump_cmd_id cmd_id, void *arg)
{
	int ret = -ENOTSUP;

	switch (cmd_id) {
	case COREDUMP_CMD_CLEAR_ERROR:
		ret = 0;
		error = 0;
		break;
	case COREDUMP_CMD_VERIFY_STORED_DUMP:
		/* We don't verify RAM validity */
		ret = 0;
		break;
	case COREDUMP_CMD_ERASE_STORED_DUMP:
		ret = retained_mem_clear(coredump_retained_mem_dev);
		break;
	case COREDUMP_CMD_COPY_STORED_DUMP: {
		if (!arg) {
			ret = -EINVAL;
			break;
		}

		struct coredump_cmd_copy_arg *copy_arg = (struct coredump_cmd_copy_arg *)arg;

		/* Make sure to ignore the header when reading */
		size_t offset = copy_arg->offset + sizeof(struct retainem_mem_hdr_t);

		ret = retained_mem_read(coredump_retained_mem_dev, offset,
					(uint8_t *)copy_arg->buffer, copy_arg->length);
		break;
	}
	case COREDUMP_CMD_INVALIDATE_STORED_DUMP: {
		/* Erase the first two bytes will invalidate the whole dump */
		uint8_t not_magic[2] = {0xFF, 0xFF};

		ret = retained_mem_write(coredump_retained_mem_dev, 0, not_magic,
					 sizeof(not_magic));
		break;
	}
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

struct coredump_backend_api coredump_backend_retained_mem = {
	.start = coredump_retained_mem_backend_start,
	.end = coredump_retained_mem_backend_end,
	.buffer_output = coredump_retained_mem_backend_buffer_output,
	.query = coredump_retained_mem_backend_query,
	.cmd = coredump_retained_mem_backend_cmd,
};

#ifdef CONFIG_DEBUG_COREDUMP_SHELL
#include <zephyr/shell/shell.h>

static int cmd_coredump_error_get(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (error == 0) {
		shell_print(sh, "No error.");
	} else {
		shell_print(sh, "Error: %d", error);
	}

	return 0;
}

static int cmd_coredump_error_clear(const struct shell *sh, size_t argc, char **argv)
{
	error = 0;

	shell_print(sh, "Error cleared.");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_coredump_error,
			       SHELL_CMD(get, NULL, "Get Coredump error", cmd_coredump_error_get),
			       SHELL_CMD(clear, NULL, "Clear Coredump error",
					 cmd_coredump_error_clear),
			       SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_coredump,
			       SHELL_CMD(error, &sub_coredump_error, "Get/clear backend error.",
					 NULL),
			       SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(coredump, &sub_coredump, "Coredump commands (logging backend)", NULL);

#endif /* CONFIG_DEBUG_COREDUMP_SHELL */
