/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log_backend_flash.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_output_dict.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/cbprintf.h>
#include <zephyr/sys/util.h>

static const char *const severity[] = {"none", "err", "wrn", "inf", "dbg"};

/**
 * One stored record, read back whole, and the buffers it is formatted with.
 * They are static because a record is as large as the configuration allows and
 * a shell thread's stack is not. Protected by @c buffers_lock as long as they
 * are in use.
 */
static uint8_t record[CONFIG_LOG_BACKEND_FLASH_SHELL_RECORD_MAX] __aligned(Z_LOG_MSG_ALIGNMENT);
static uint8_t package[CONFIG_LOG_BACKEND_FLASH_SHELL_RECORD_MAX] __aligned(Z_LOG_MSG_ALIGNMENT);
static char text[CONFIG_LOG_BACKEND_FLASH_SHELL_TEXT_MAX];

static K_MUTEX_DEFINE(buffers_lock);

struct text_out {
	size_t len;
};

static int text_out(int c, void *arg)
{
	struct text_out *out = arg;

	if (out->len < (sizeof(text) - 1U)) {
		text[out->len++] = (char)c;
	}

	return c;
}

/** Read the range a command was given, defaulting to the whole log. */
static int parse_range(const struct shell *sh, size_t argc, char **argv, uint32_t *index,
		       uint32_t *count)
{
	int err = 0;

	*index = 0;
	*count = 0;

	if (argc > 1) {
		*index = (uint32_t)shell_strtoul(argv[1], 0, &err);
	}

	if (err == 0 && argc > 2) {
		*count = (uint32_t)shell_strtoul(argv[2], 0, &err);
		if (*count == 0U) {
			shell_error(sh, "count must not be zero");
			return -EINVAL;
		}
	}

	return 0;
}

static int cmd_info(const struct shell *sh, size_t argc, char **argv)
{
	struct log_backend_flash_info info;
	int rc;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	rc = log_backend_flash_query(&info);
	if (rc < 0) {
		shell_error(sh, "cannot read the log: %d", rc);
		return rc;
	}

	shell_print(sh, "records:   %u", info.record_cnt);
	shell_print(sh, "dropped:   %u", info.dropped_cnt);
	shell_print(sh, "used:      %zu of %zu bytes", info.used, info.size);
	shell_print(sh, "build id:  %s%s", info.build_id,
		    info.build_id_matches ? "" : " (not this firmware)");

	return 0;
}

/**
 * Format one record the way the log would have been printed.
 *
 * Only safe when the log was written by the running firmware: a record holds
 * the address of its format string, so formatting one from another build
 * dereferences whatever now lives at that address.
 *
 * cbpprintf to @c text rather than using shell_cbpprintf, because a separate
 * shell_fprintf call for the header part would risk being interleaved with
 * live logs (the shell TX mutex is released between them).
 */
static void print_record(const struct shell *sh, uint32_t index, size_t len)
{
	struct log_dict_output_normal_msg_hdr_t hdr;
	struct text_out out = {0};
	const char *name;
	int slen;

	if (len < sizeof(hdr)) {
		shell_warn(sh, "%u: record too short", index);
		return;
	}

	memcpy(&hdr, record, sizeof(hdr));

	if (hdr.package_len > sizeof(package)) {
		shell_warn(sh, "%u: package does not fit in the shell buffer", index);
		return;
	}

	memcpy(package, &record[sizeof(hdr)], hdr.package_len);

	slen = cbpprintf(text_out, &out, package);
	if (slen < 0) {
		shell_warn(sh, "%u: cannot format the message: %d", index, slen);
		return;
	}
	/* printk() carries its own newline; the shell adds one of its own. */
	while ((out.len > 0U) && (text[out.len - 1U] == '\n')) {
		out.len--;
	}
	text[out.len] = '\0';

	if (hdr.level == LOG_LEVEL_NONE) {
		/* printk() rather than a log message. */
		shell_print(sh, "[%10llu] %s", (unsigned long long)hdr.timestamp, text);
	} else {
		name = log_source_name_get(hdr.domain, (uint32_t)hdr.source);
		shell_print(sh, "[%10llu] <%s> %s: %s", (unsigned long long)hdr.timestamp,
			    severity[MIN(hdr.level, ARRAY_SIZE(severity) - 1U)],
			    (name != NULL) ? name : "?", text);
	}

	if (hdr.data_len > 0U) {
		shell_hexdump(sh, &record[sizeof(hdr) + hdr.package_len], hdr.data_len);
	}
}

/** Walk a range of records, one at a time so that one record has to fit. */
static int for_each_record(const struct shell *sh, uint32_t index, uint32_t count,
			   void (*fn)(const struct shell *sh, uint32_t index, size_t len))
{
	uint32_t done = 0;
	int rc = 0;

	k_mutex_lock(&buffers_lock, K_FOREVER);

	while ((count == 0U) || (done < count)) {
		uint32_t got;
		ssize_t len;

		len = log_backend_flash_read(index + done, 1, record, sizeof(record), &got);
		if (len < 0) {
			shell_error(sh, "cannot read the log: %zd", len);
			rc = (int)len;
			goto unlock;
		}

		if (got == 0U) {
			if (len == 0 && done == 0U) {
				shell_print(sh, "no records");
			}
			break;
		}

		fn(sh, index + done, (size_t)len);
		done++;
	}

unlock:
	k_mutex_unlock(&buffers_lock);

	return rc;
}

static int cmd_read(const struct shell *sh, size_t argc, char **argv)
{
	struct log_backend_flash_info info;
	uint32_t index;
	uint32_t count;
	int rc;

	rc = parse_range(sh, argc, argv, &index, &count);
	if (rc < 0) {
		return rc;
	}

	rc = log_backend_flash_query(&info);
	if (rc < 0) {
		shell_error(sh, "cannot read the log: %d", rc);
		return rc;
	}

	if (!info.build_id_matches) {
		shell_error(sh, "the log was written by build '%s', not by this one",
			    info.build_id);
		shell_error(sh, "use 'dump' and decode it against that build, or erase the log");
		return -EPERM;
	}

	return for_each_record(sh, index, count, print_record);
}

/**
 * Hex of one stored record, exactly as log_parser.py expects to be fed.
 * Can't use shell_hexdump since that adds offset, spaces and ASCII.
 *
 * The hex is built up in @c text and handed to the shell a line at a time,
 * rather than a byte at a time, so that a line is printed in one call and
 * cannot be torn in half by whatever else writes to the shell. It still
 * may be torn on a line-basis, but that's easier to filter out.
 */
static void dump_record(const struct shell *sh, uint32_t index, size_t len)
{
	const size_t per_line = (sizeof(text) - 1U) / 2U;
	size_t done = 0;

	ARG_UNUSED(index);

	while (done < len) {
		size_t n = MIN(per_line, len - done);

		(void)bin2hex(&record[done], n, text, sizeof(text));
		shell_print(sh, "%s", text);
		done += n;
	}
}

static int cmd_dump(const struct shell *sh, size_t argc, char **argv)
{
	uint32_t index;
	uint32_t count;
	int rc;

	rc = parse_range(sh, argc, argv, &index, &count);
	if (rc < 0) {
		return rc;
	}

	return for_each_record(sh, index, count, dump_record);
}

static int cmd_erase(const struct shell *sh, size_t argc, char **argv)
{
	int rc;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	rc = log_backend_flash_erase();
	if (rc < 0) {
		shell_error(sh, "cannot erase the log: %d", rc);
		return rc;
	}

	shell_print(sh, "log erased");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_log_flash,
	SHELL_CMD_ARG(info, NULL, "Show what the log partition holds.\nUsage: info", cmd_info, 1,
		      0),
	SHELL_CMD_ARG(read, NULL, "Print stored records.\nUsage: read [<first> [<count>]]",
		      cmd_read, 1, 2),
	SHELL_CMD_ARG(dump, NULL,
		      "Hex dump stored records, for decoding on a host.\n"
		      "Usage: dump [<first> [<count>]]",
		      cmd_dump, 1, 2),
	SHELL_CMD_ARG(erase, NULL, "Discard the stored log.\nUsage: erase", cmd_erase, 1, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(log_flash, &sub_log_flash, "Commands for the flash log backend", NULL);
