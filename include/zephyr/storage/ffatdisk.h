/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_STORAGE_FFATDISK
#define ZEPHYR_INCLUDE_STORAGE_FFATDISK

#include <stdint.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/iterable_sections.h>

/**
 * @brief Fake FAT Disk support
 * @defgroup ffat Fake FAT Disk support
 * @ingroup storage_apis
 * @{
 */

/**
 * @cond INTERNAL_HIDDEN
 * Internally used
 */

#define FAT_FILE_NAME_LEN		11U

struct ffat_file {
	sys_snode_t node;
	const char *disk_name;
	size_t size;
	uint32_t start;
	uint32_t end;
	int (*rd_cb)(struct ffat_file *const file, const uint32_t sector,
		     uint8_t *const buf, const uint32_t size);
	int (*wr_cb)(struct ffat_file *const file, const uint32_t sector,
		     const uint8_t *const buf, const uint32_t size);
	void const *priv;
	char name[FAT_FILE_NAME_LEN];
	uint8_t attr;
};

/** @endcond */

/**
 * @brief Macro to define the FFAT file and place it in the right memory section.
 */
#define FFAT_FILE_DEFINE(s_name, d_name, f_name, f_size, f_rd_cb, f_wr_cb, f_priv)	\
	BUILD_ASSERT(f_size <= UINT32_MAX,						\
		     "File size is greater than UINT32_MAX");				\
	static STRUCT_SECTION_ITERABLE(ffat_file, s_name) = {				\
		.name = f_name,								\
		.disk_name = d_name,							\
		.size = f_size,								\
		.rd_cb = f_rd_cb,							\
		.wr_cb = f_wr_cb,							\
		.priv = f_priv,								\
	}

/**
 * @}
 */

#endif	/* ZEPHYR_INCLUDE_STORAGE_FFATDISK */
