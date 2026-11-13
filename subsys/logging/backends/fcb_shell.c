/** @file
 * @brief FCB shell module
 *
 * Provide some FCB logger shell commands that can be useful to applications.
 *
 * Copyright (c) 2026 Sentek Pty Ltd
 *
 */

#define LOG_MODULE_NAME modem_shell

#include <zephyr/kernel.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/shell/shell.h>

#include <zephyr/sys/printk.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/device.h>

#if CONFIG_FCB
#include <fcb_shell.h>
#include <fcb_log.h>
#include <zephyr/fs/fcb.h>
#include "fcb_priv.h" /* zephyr_include_directories(${ZEPHYR_BASE}/subsys/fs/fcb) in CMkeList.txt provides path  */
#include "log_backend_fcb.h"
#endif

#define MODULE fcbs

#ifdef CONFIG_DEBUG_BUILD
#define FCB_LOG_LEVEL 4
#else
#define FCB_LOG_LEVEL 3
#endif

//#define FCB_SHELL_WITH_FCBLINE_TEST_COMMAND

LOG_MODULE_REGISTER(fcbs, FCB_LOG_LEVEL);

extern struct k_mutex fcb_lock;
static atomic_t cancel_cmd = ATOMIC_INIT(0);

/**@brief set_cancel_cmd() sets module variable for cancelling dumping FCB logs which could be long.
 *
 * @param[in]  none
 * @return     void
 */
void set_cancel_cmd(void)
{
	atomic_set(&cancel_cmd, 1);
}

static int cmd_fcb_logs(const struct shell *sh, size_t argc, char *argv[])
{

	int rc = 0;
	// dump saved in FCB logs to shell
	struct fcb *fcb = get_fcb_instance();
	struct fcb_entry entry_loc;
	memset(&entry_loc, 0, sizeof(entry_loc));
	uint8_t log_data[MAX_FCB_LOG_ENTRY_SIZE + 1];
	const struct flash_sector *sector = NULL;

	int pg = 0;
	int num_lines = 0;
	int req_sector = -1; // sector index starts at 0
	int start_line = 0;
	int end_line = 0;
	int cur_line = 0;
	int read_lines = 0;
	int active_ix = 0;
	int i = 0;

	if (argc == 1) {
		shell_help(sh);
		return 0;
	}
	atomic_set(&cancel_cmd, 0);
	if (argc > 2) {
		num_lines = strtol(argv[1], NULL, 10);
		pg = strtol(argv[2], NULL, 10);
		start_line = (pg - 1) * num_lines + 1;
		end_line = start_line + num_lines - 1;
	}
	if (argc == 4) {
		req_sector = strtol(argv[3], NULL, 10);
		sector = &fcb->f_sectors[req_sector];
	}
	if (argc == 2) {
		if (strcmp(argv[1], "info") == 0) {
			// 'info' return fcb->f_sector_cnt and active_ix
			if (fcb_is_empty(fcb) == 0) {
				for (i = 0; i < fcb->f_sector_cnt; i++) {
					if (fcb->f_active.fe_sector == &fcb->f_sectors[i]) {
						active_ix = i;
						break;
					}
				}
			}
			shell_fprintf(sh, SHELL_NORMAL, "%d/%d\n", active_ix, fcb->f_sector_cnt);
			return 0;
		}
#ifdef FCB_SHELL_WITH_FCBLINE_TEST_COMMAND
		if (strcmp(argv[1], "fcbline") == 0) // append to fcb logs line spanning 2 elements
		{
			const char ln[] = "This command is for Flash Circular Buffer logging "
					  "containing very long data line spanning over 2 elements";
			memset(log_data, 0, sizeof(log_data));
			int clen = strlen(ln);
#if (DT_CHOSEN(nordic_pm_ext_flash) == mx25r64)
			if (clen % 4 != 0) {
				clen = ((clen / 4) + 1) * 4;
			}
#endif
			strncpy(log_data, ln, clen);
			LOG_INF("%s", log_data); // fcb_append_log_entry(log_data,clen);

			return 0;
		}
#endif
		if (strcmp(argv[1], "clear") == 0) // clear fcb logs
		{
			const struct log_backend *fcb_log_backend = log_backend_fcb_get();
			// lock fcb_lock mutex to prevent apending elements

			log_backend_disable(fcb_log_backend); // disable fcb logs while erasing
			if (k_mutex_lock(&fcb_lock, K_MSEC(500)) != 0) {
				log_backend_enable(log_backend_fcb_get(), NULL,
						   CONFIG_LOG_MAX_LEVEL);
				shell_warn(sh, "FCB clear - try again");
				return -EACCES;
			}
			// erase all sectors if fcb
			for (int i = 0; i < fcb->f_sector_cnt; i++) {
				rc = fcb_erase_sector(fcb, &fcb->f_sectors[i]);
				if (rc < 0) {
					shell_error(sh, "FCB clear Failed, code[%d]", rc);
					rc = -EIO;
					goto out_clear;
				}
				if (i == fcb->f_sector_cnt - 1) {
					shell_print(sh, "FCB clear success");
				}
			}
		out_clear:
			k_mutex_unlock(&fcb_lock);
			// fcb_log_backend->api->init(fcb_log_backend);
			rc = fcb_init(fcb->fap->fa_id, fcb);
			if (rc == 0) {
				log_backend_enable(log_backend_fcb_get(), NULL,
						   CONFIG_LOG_MAX_LEVEL);
			}
			return rc;
		}
		if (strncmp(argv[1], "all", 3) == 0) {
			start_line = 0;  // dump all elements
			req_sector = -1; //  for dumping all elements
		}
	}

	if (fcb_is_empty(fcb) == 0) {
		int oldest_ix = 0;
		for (i = 0; i < fcb->f_sector_cnt; i++) {
			if (fcb->f_oldest == &fcb->f_sectors[i]) {
				oldest_ix = i;
				break;
			}
		}
		int ix;

		for (i = 0; i < fcb->f_sector_cnt; i++) {
			if (atomic_get(&cancel_cmd)) {
				break;
			}
			ix = (i + oldest_ix) % fcb->f_sector_cnt;

			struct fcb_entry_ctx entry_ctx;
			memset(&entry_ctx, 0, sizeof(struct fcb_entry_ctx));
			if (req_sector < 0) // 'fcb all' requested
			{
				entry_ctx.loc.fe_sector = &fcb->f_sectors[ix];
				entry_ctx.loc.fe_elem_off = 0U;
				uint8_t tmp_str[4];
				// Read directly first 2 bytes and if they are erase value then skip
				// sector
				if (fcb_flash_read(fcb, entry_ctx.loc.fe_sector,
						   entry_ctx.loc.fe_elem_off, tmp_str, 2) != 0) {
					continue; // if fail to read sector header do skip it
				}
				if ((tmp_str[0] == fcb->f_erase_value) &&
				    (tmp_str[1] == fcb->f_erase_value)) {
					continue; // if sector is empty then check next sector
				}
			} else {
				ix = req_sector;
			}
			// shell_fprintf(sh, SHELL_NORMAL, "\nReading sector %d offs=%p\n", ix,
			// (void*)fcb->f_sectors[ix].fs_off);
			int sector_no = ix;
			sector = &fcb->f_sectors[ix];
			entry_ctx.loc.fe_sector = &fcb->f_sectors[ix];
			entry_ctx.loc.fe_elem_off = 0U;
			rc = k_mutex_lock(&fcb->f_mtx, K_FOREVER);
			if (rc < 0) {
				return -EACCES;
			}

			while ((rc = fcb_getnext_nolock(fcb, &entry_ctx.loc)) != -ENOTSUP) {
				if (sector && (entry_ctx.loc.fe_sector != sector)) {
					break;
				}
				if (cancel_cmd) {
					break;
				}
				k_mutex_unlock(&fcb->f_mtx);
				entry_ctx.fap = fcb->fap;

				const struct flash_sector *rsector = entry_ctx.loc.fe_sector;
				// Find sector index
				int idx = -1;
				for (int j = 0; j < fcb->f_sector_cnt; j++) {
					if (rsector == &fcb->f_sectors[j]) {
						idx = j;
						break;
					}
				}

				if (idx == sector_no) {
					memset(log_data, 0, sizeof(log_data));
					int rc = flash_area_read(entry_ctx.fap,
								 fcb->f_sectors[ix].fs_off +
									 entry_ctx.loc.fe_data_off,
								 &log_data[0],
								 entry_ctx.loc.fe_data_len);
					if (rc) {
						LOG_ERR("FCB fail read entry,error %d", rc);
					}
					cur_line++;
					if ((start_line == 0) // dumping all FCB
					    || ((cur_line >= start_line) &&
						(cur_line <= end_line))) // output requested chunk
					{
						shell_fprintf(sh, SHELL_NORMAL, "%s", &log_data[0]);
						read_lines++;
					}
				}

				rc = k_mutex_lock(&fcb->f_mtx, K_FOREVER);
				if (rc < 0) {
					return -EINVAL;
				}
				if ((start_line > 0) && (cur_line >= end_line)) {
					break;
				}
			}
			k_mutex_unlock(&fcb->f_mtx);
			if (req_sector >= 0) { // processing only 1 sector
				break;
			}
		}
	}
	if (read_lines == 0) {
		rc = -ENOMSG;
	} else if (argc == 2) {
		rc = 0; // for 'fcb all' the last fcb_getnext_nolock() call result -ENOTSUP
	} else if (argc > 2) {
		rc = read_lines;
	}
	return rc;
}

SHELL_CMD_REGISTER(fcb, NULL,
		   "FCB commands: \r\n"
		   "\t info - return [active sector/sector count]\r\n"
		   "\t clear - erase fcb logs region\r\n"
		   "\t all - dump all logs\r\n"
#ifdef FCB_SHELL_WITH_FCBLINE_TEST_COMMAND
		   "\t fcbline - inserts 2 element line\r\n"
#endif
		   "\t {E P S - dumps [ElementsPerPage Page Sector]}",
		   cmd_fcb_logs);
