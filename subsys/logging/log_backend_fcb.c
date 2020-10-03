/*
 * Copyright (c) 2020 Xylem Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log_backend.h>
#include <logging/log_core.h>
#include <logging/log_msg.h>
#include <logging/log_output.h>
#include "log_backend_std.h"
#include <fs/fcb.h>
#include <assert.h>

static struct flash_sector sectors[CONFIG_LOG_BACKEND_FCB_SECTOR_COUNT] = {
	{ .fs_off = CONFIG_LOG_BACKEND_FCB_SECTOR1_OFFSET,
	  .fs_size = CONFIG_LOG_BACKEND_FCB_SECTOR1_SIZE },
#if CONFIG_LOG_BACKEND_FCB_SECTOR_COUNT >= 2
	{ .fs_off = CONFIG_LOG_BACKEND_FCB_SECTOR2_OFFSET,
	  .fs_size = CONFIG_LOG_BACKEND_FCB_SECTOR2_SIZE },
#endif
#if CONFIG_LOG_BACKEND_FCB_SECTOR_COUNT >= 3
	{ .fs_off = CONFIG_LOG_BACKEND_FCB_SECTOR3_OFFSET,
	  .fs_size = CONFIG_LOG_BACKEND_FCB_SECTOR3_SIZE },
#endif
#if CONFIG_LOG_BACKEND_FCB_SECTOR_COUNT >= 4
	{ .fs_off = CONFIG_LOG_BACKEND_FCB_SECTOR4_OFFSET,
	  .fs_size = CONFIG_LOG_BACKEND_FCB_SECTOR4_SIZE },
#endif
#if CONFIG_LOG_BACKEND_FCB_SECTOR_COUNT >= 5
#error "Max 4 log sectors are supported"
#endif
};

static uint8_t buf[CONFIG_LOG_BACKEND_FCB_LINE_LEN];
static struct fcb fcb = {
	.f_magic = 0xfcb00106, /* "fcb log" */
	.f_version = 1,
	.f_sector_cnt = CONFIG_LOG_BACKEND_FCB_SECTOR_COUNT,
	.f_sectors = sectors,
};

static int store_line(uint8_t *data, size_t length, void *ctx)
{
	struct fcb_entry loc;
	int rc;

	/* format: [00:00:04.388,000] <inf> this is a log line */
	/* store only <wrn> and <err> lines. */
	/* search for < to manage with and without colours */
	char *p = memchr(data, '<', length);
	if (!p || (p[1] != 'e' && p[1] != 'w'))
		return length;

	while (1) {
		rc = fcb_append(&fcb, length, &loc);
		if (!rc) {
			break;
		}
		else if (rc == -ENOSPC) {
			rc = fcb_rotate(&fcb);
			if (rc) {
				printk("fcb_rotate() returned %d\n", rc);
				return length;
			}
		}
		else {
			printk("fcb_append() returned %d\n", rc);
			return length;
		}
	}
	rc = flash_area_write(fcb.fap, FCB_ENTRY_FA_DATA_OFF(loc),
			      data, length);
	if (rc) {
		printk("flash_area_write() returned %d\n", rc);
		return length;
	}

	rc = fcb_append_finish(&fcb, &loc);
	if (rc) {
		printk("fcb_append_finish() returned %d\n", rc);
		return length;
	}

	return length;
}

LOG_OUTPUT_DEFINE(log_output, store_line, buf, sizeof(buf));

static void put(const struct log_backend *const backend,
		struct log_msg *msg)
{
	uint32_t flag = LOG_OUTPUT_FLAG_FORMAT_SYST;

	log_backend_std_put(&log_output, flag, msg);
}

static void print_old_lines(void)
{
	int corrupted = 0;
	struct fcb_entry loc = {.fe_sector = NULL};
	while (1) {
		int rc;
		rc = fcb_getnext(&fcb, &loc);
		if (rc < 0) {
			if (rc == -ENOTSUP)
				break;
			printk("fcb_getnext() failed with rc %d\n", rc);
			break;
		}

		int len = MIN(sizeof(buf), loc.fe_data_len);
		rc = flash_area_read(fcb.fap, loc.fe_data_off, buf, len);
		if (rc < 0) {
			printk("flash_read_area(%d, %d) failed with rc %d\n",
			       loc.fe_data_off, len, rc);
			break;
		}
		buf[len] = 0;
		if (buf[0] == '[' && buf[3] == ':' && buf[17] == ']')
			printk("flog: %s", buf);
		else
			corrupted++;
	}
	if (corrupted)
		printk("flog: discarded %d corrupt log lines\n", corrupted);
}

static void log_backend_fcb_init(void)
{
	int rc;
	struct fcb* fcbp = &fcb;

	rc = fcb_init(CONFIG_LOG_BACKEND_FCB_FLASH_AREA_ID, fcbp);
	if (rc && rc != -ENOMSG) {
		printk("fcb_init() failed with rc %d\n", rc);
		return;
	}

	print_old_lines();
}

static void panic(struct log_backend const *const backend)
{
	log_backend_std_panic(&log_output);
}

static void dropped(const struct log_backend *const backend, uint32_t cnt)
{
	ARG_UNUSED(backend);

	log_backend_std_dropped(&log_output, cnt);
}

static void sync_string(const struct log_backend *const backend,
			struct log_msg_ids src_level, uint32_t timestamp,
			const char *fmt, va_list ap)
{
	uint32_t flag = LOG_OUTPUT_FLAG_FORMAT_SYST;

	log_backend_std_sync_string(&log_output, flag, src_level,
				    timestamp, fmt, ap);
}

static void sync_hexdump(const struct log_backend *const backend,
			 struct log_msg_ids src_level, uint32_t timestamp,
			 const char *metadata, const uint8_t *data, uint32_t length)
{
	uint32_t flag = LOG_OUTPUT_FLAG_FORMAT_SYST;

	log_backend_std_sync_hexdump(&log_output, flag, src_level,
				     timestamp, metadata, data, length);
}

const struct log_backend_api log_backend_fcb_api = {
	.put = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ? NULL : put,
	.put_sync_string = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ?
		sync_string : NULL,
	.put_sync_hexdump = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ?
		sync_hexdump : NULL,
	.panic = panic,
	.init = log_backend_fcb_init,
	.dropped = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ? NULL : dropped,
};

LOG_BACKEND_DEFINE(log_backend_fcb, log_backend_fcb_api, true);
