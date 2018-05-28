/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <misc/printk.h>
#include <logging/sys_log.h>
#include <stdio.h>
#include <zephyr.h>
#include <nffs/nffs.h>
#include <fs.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define LOG_BUF_SIZE 	   	(512)
#define PATH_LEN_MAX 		 (50)				// Maximum path length.
#define BUF_LEN 		    (100)				// File read buffer length
#define ACCESS_DIGITS_NUM 	  (5)				// Number of digits used to count file access
#define NUL_TERMIN_LEN		  (1) 				// Size of NUL string terminator

static const char m_dir_name[] 		= "/mydir";
static const char m_file_name[] 	= "/myfile";
static const char m_file_contents[] = "This is myfile. Number of times this file has been written to: 00000";

void main(void){
	SYS_LOG_INF("Starting NFFS flash example.");
	int err;
	fs_file_t fs_file;
	struct fs_dirent stat;
	u32_t file_contents_bytes = sizeof(m_file_contents);
	char path_name[PATH_LEN_MAX] = "";
	strcat(path_name, m_dir_name);
	strcat(path_name, m_file_name);

	/* Before creating any objects, check if there are existing contents in the file system.
	On first boot/after flash erase the system will be blank. If blank, a folder and file will be created.
	Reset/power cycle the target and observe that directories and files are retained in flash
	and the file written counter incremented. */

	/* Validate root directory. */
	err = nffs_misc_validate_root_dir();
	if (err) {
		SYS_LOG_WRN("Could not validate root dir. Probably due to this being first boot. Err: %d", err);
	}

	/* Check directory status and create a directory if nothing is found. */
	err = fs_stat(m_dir_name, &stat);
	if (err) {
		SYS_LOG_WRN("Could not get directory status. Probably due to this being first boot. Creating dir.");

		err = fs_mkdir(m_dir_name);
		if (err) {
			SYS_LOG_ERR("Could not create dir. Err: %d", err);
			return;
		}

		err = fs_stat(m_dir_name, &stat);
		if (err) {
			SYS_LOG_ERR("Could not get directory status. Err: %d", err);
			return;
		}
	}

	/* Open a file, or create a new file if it does not exist. */
	err = fs_open(&fs_file, path_name);
	if (err) {
		SYS_LOG_ERR("fs_open failed. Err: %d", err);
		return;
	}

	char read_from_file[BUF_LEN];
	int bytes_read;
	int times_written;

	/* Read file contents if it exists. */
	bytes_read = fs_read(&fs_file, (void*)&read_from_file, file_contents_bytes);
	if (bytes_read != file_contents_bytes) {
		SYS_LOG_WRN("fs_read read: %d bytes, should have read: %d bytes. Probably due to this being first boot.", bytes_read, file_contents_bytes);
		times_written = 0;
	}
	else{
		char *ptr = read_from_file;
		for (u32_t offset = (bytes_read - ACCESS_DIGITS_NUM - NUL_TERMIN_LEN); offset < bytes_read; offset++){
			if (isdigit(read_from_file[offset])){
				times_written = strtol(ptr + offset, NULL, 10);
				break;
			}
			else{
				times_written = -1;
			}
		}
	}

	if (times_written < 0){
		SYS_LOG_ERR("Error. Could not find any valid numeric value in file");
		return;
	}

	times_written++;

	/* Write content to file. */
	char buf[BUF_LEN] = "";
	strncpy(buf, m_file_contents, strlen(m_file_contents) - ACCESS_DIGITS_NUM);
	char num_buf[ACCESS_DIGITS_NUM + NUL_TERMIN_LEN];
	sprintf(num_buf, "%5d", times_written);
	strcat(buf, num_buf);

	err = fs_seek(&fs_file, 0, FS_SEEK_SET); // Seek to go to the beginning of the file
	if (err) {
		SYS_LOG_ERR("fs_seek err %d", err);
		return;
	}

	u32_t bytes_written;
	bytes_written = fs_write(&fs_file, (void*)buf, sizeof(buf));
	if (bytes_written != sizeof(buf)) {
		SYS_LOG_ERR("fs_write err. Written %d bytes, should have written %d bytes", bytes_written, sizeof(buf));
		return;
	}

	/* Read from file. */
	err = fs_seek(&fs_file, 0, FS_SEEK_SET); // Seek to go to the beginning of the file
	if (err) {
		SYS_LOG_ERR("fs_seek. Err: %d", err);
		return;
	}

	/* Read and print file contents. */
	bytes_read = fs_read(&fs_file, (void*)&read_from_file, file_contents_bytes);
	if (bytes_read != file_contents_bytes) {
		SYS_LOG_ERR("fs_read error. Read %d bytes, should have read %d bytes", bytes_read, file_contents_bytes);
		return;
	}

	SYS_LOG_INF("File contents: %s", read_from_file);
}