/*
 * Copyright (c) 2024 Zephyr Contributors
 * Copyright (c) 2025 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/util.h>

/* in case these are defined as macros */
#undef stdin
#undef stdout
#undef stderr

static char __aligned(WB_UP(CONFIG_ZVFS_LIBC_FILE_ALIGN)) _stdin[CONFIG_ZVFS_LIBC_FILE_SIZE];
static char __aligned(WB_UP(CONFIG_ZVFS_LIBC_FILE_ALIGN)) _stdout[CONFIG_ZVFS_LIBC_FILE_SIZE];
static char __aligned(WB_UP(CONFIG_ZVFS_LIBC_FILE_ALIGN)) _stderr[CONFIG_ZVFS_LIBC_FILE_SIZE];
FILE *const stdin = (FILE *)(&_stdin);
FILE *const stdout = (FILE *)(&_stdout);
FILE *const stderr = (FILE *)(&_stderr);

#define table_stride ROUND_UP(CONFIG_ZVFS_LIBC_FILE_SIZE, CONFIG_ZVFS_LIBC_FILE_ALIGN)
#define table_data   ((uint8_t *)_k_mem_slab_buf_zvfs_libc_file_table)

K_MEM_SLAB_DEFINE_STATIC(zvfs_libc_file_table, CONFIG_ZVFS_LIBC_FILE_SIZE, CONFIG_ZVFS_OPEN_MAX,
			 CONFIG_ZVFS_LIBC_FILE_ALIGN);

static int file_to_fd[CONFIG_ZVFS_OPEN_MAX];
#define ptr_to_idx(fp) (POINTER_TO_INT((uint8_t *)fp - table_data) / table_stride)

int zvfs_libc_file_alloc(int fd, const char *mode, FILE **fp, k_timeout_t timeout)
{
	int ret;

	__ASSERT_NO_MSG(mode != NULL);
	__ASSERT_NO_MSG(fp != NULL);

	ret = k_mem_slab_alloc(&zvfs_libc_file_table, (void **)fp, timeout);
	if (ret < 0) {
		return ret;
	}
	file_to_fd[ptr_to_idx(*fp)] = fd;
	zvfs_libc_file_alloc_cb(fd, mode, *fp);

	return 0;
}

void zvfs_libc_file_free(FILE *fp)
{
	if ((fp == NULL) || (fp == stdin) || (fp == stdout) || (fp == stderr)) {
		return;
	}

	k_mem_slab_free(&zvfs_libc_file_table, (void *)fp);
	file_to_fd[ptr_to_idx(fp)] = -1;
}

FILE *zvfs_libc_file_from_fd(int fd)
{
	if ((fd < 0) || (fd >= ARRAY_SIZE(file_to_fd))) {
		return NULL;
	}
	switch (fd) {
	case 0:
		return stdin;
	case 1:
		return stdout;
	case 2:
		return stderr;
	default:
		/* fd points at an actual file */
		break;
	}

	for (int i = 0; i < ARRAY_SIZE(file_to_fd); i++) {
		if (file_to_fd[i] == fd) {
			return (FILE *)&_k_mem_slab_buf_zvfs_libc_file_table[table_stride * i];
		}
	}
	return NULL;
}
