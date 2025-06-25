/*
 * Copyright (c) 2022-2025 Nordic Semiconductor ASA
 * Copyright (c) 2023 Husqvarna AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#if FFCONF_DEF != 80386
#error "Configuration version mismatch"
#endif

#include <zephyr/devicetree.h>

/*
 * Overrides of FF_ options from ffconf.h
 */
#if defined(CONFIG_FS_FATFS_READ_ONLY)
#undef FF_FS_READONLY
#define FF_FS_READONLY	CONFIG_FS_FATFS_READ_ONLY
#endif /* defined(CONFIG_FS_FATFS_READ_ONLY) */

#if defined(CONFIG_FS_FATFS_MKFS)
#undef FF_USE_MKFS
#define FF_USE_MKFS	CONFIG_FS_FATFS_MKFS
#else
/* Note that by default the ffconf.h disables MKFS */
#undef FF_USE_MKFS
#define FF_USE_MKFS 1
#endif /* defined(CONFIG_FS_FATFS_MKFS) */

#if defined(CONFIG_FS_FATFS_CODEPAGE)
#undef FF_CODE_PAGE
#define FF_CODE_PAGE	CONFIG_FS_FATFS_CODEPAGE
#else
/* Note that default value, in ffconf.h, for FF_CODE_PAGE is 932 */
#undef FF_CODE_PAGE
#define FF_CODE_PAGE 437
#endif /* defined(CONFIG_FS_FATFS_CODEPAGE) */

#if defined(CONFIG_FS_FATFS_FF_USE_LFN)
#if CONFIG_FS_FATFS_FF_USE_LFN <= 3
#undef FF_USE_LFN
#define FF_USE_LFN CONFIG_FS_FATFS_FF_USE_LFN
#else
#error Invalid LFN buffer location
#endif
#endif /* defined(CONFIG_FS_FATFS_LFN) */

#if defined(CONFIG_FS_FATFS_MAX_LFN)
#undef FF_MAX_LFN
#define	FF_MAX_LFN	CONFIG_FS_FATFS_MAX_LFN
#endif /* defined(CONFIG_FS_FATFS_MAX_LFN) */

#if defined(CONFIG_FS_FATFS_MIN_SS)
#undef FF_MIN_SS
#define FF_MIN_SS	CONFIG_FS_FATFS_MIN_SS
#endif /* defined(CONFIG_FS_FATFS_MIN_SS) */

#if defined(CONFIG_FS_FATFS_MAX_SS)
#undef FF_MAX_SS
#define FF_MAX_SS		CONFIG_FS_FATFS_MAX_SS
#endif /* defined(CONFIG_FS_FATFS_MAX_SS) */

#if defined(CONFIG_FS_FATFS_EXFAT)
#undef FF_FS_EXFAT
#define FF_FS_EXFAT		CONFIG_FS_FATFS_EXFAT
#endif /* defined(CONFIG_FS_FATFS_EXFAT) */

#if defined(CONFIG_FS_FATFS_REENTRANT)
#undef FF_FS_REENTRANT
#undef FF_FS_TIMEOUT
#include <zephyr/kernel.h>
#define FF_FS_REENTRANT		CONFIG_FS_FATFS_REENTRANT
#define FF_FS_TIMEOUT		K_FOREVER
#endif /* defined(CONFIG_FS_FATFS_REENTRANT) */

#if defined(CONFIG_FS_FATFS_LBA64)
#undef FF_LBA64
#define FF_LBA64		CONFIG_FS_FATFS_LBA64
#endif /* defined(CONFIG_FS_FATFS_LBA64) */

#if defined(CONFIG_FS_FATFS_MULTI_PARTITION)
#undef FF_MULTI_PARTITION
#define FF_MULTI_PARTITION	CONFIG_FS_FATFS_MULTI_PARTITION
#endif /* defined(CONFIG_FS_FATFS_MULTI_PARTITION) */

/*
 * These options are override from default values, but have no Kconfig
 * options.
 */
#undef FF_FS_TINY
#define FF_FS_TINY 1

#undef FF_FS_NORTC
#if defined(CONFIG_FS_FATFS_HAS_RTC)
#define FF_FS_NORTC 0
#else
#define FF_FS_NORTC 1
#endif /* defined(CONFIG_FS_FATFS_HAS_RTC) */

/* Zephyr uses FF_VOLUME_STRS */
#undef FF_STR_VOLUME_ID
#define FF_STR_VOLUME_ID 1

/* By default FF_STR_VOLUME_ID in ffconf.h is 0, which means that
 * FF_VOLUME_STRS is not used. Zephyr uses FF_VOLUME_STRS.
 * The array of volume strings is automatically generated from devicetree.
 */

#define _FF_DISK_NAME(node) DT_PROP(node, disk_name),

#undef FF_VOLUME_STRS
#define FF_VOLUME_STRS \
	DT_FOREACH_STATUS_OKAY(zephyr_flash_disk, _FF_DISK_NAME) \
	DT_FOREACH_STATUS_OKAY(zephyr_ram_disk, _FF_DISK_NAME) \
	DT_FOREACH_STATUS_OKAY(zephyr_sdmmc_disk, _FF_DISK_NAME) \
	DT_FOREACH_STATUS_OKAY(zephyr_mmc_disk, _FF_DISK_NAME) \
	DT_FOREACH_STATUS_OKAY(st_stm32_sdmmc, _FF_DISK_NAME)

#undef FF_VOLUMES
#define FF_VOLUMES NUM_VA_ARGS_LESS_1(FF_VOLUME_STRS _)

#if defined(CONFIG_FS_FATFS_EXTRA_NATIVE_API)
#undef FF_USE_LABEL
#undef FF_USE_EXPAND
#undef FF_USE_FIND
#define FF_USE_LABEL 1
#define FF_USE_EXPAND 1
#define FF_USE_FIND 1
#endif /* defined(CONFIG_FS_FATFS_EXTRA_NATIVE_API) */

/*
 * When custom mount points are activated FF_VOLUME_STRS needs
 * to be undefined in order to be able to provide a custom
 * VolumeStr array containing the contents of
 * CONFIG_FS_FATFS_CUSTOM_MOUNT_POINTS. Additionally the
 * FF_VOLUMES define needs to be set to the correct mount
 * point count contained in
 * CONFIG_FS_FATFS_CUSTOM_MOUNT_POINT_COUNT.
 */
#if CONFIG_FS_FATFS_CUSTOM_MOUNT_POINT_COUNT
#undef FF_VOLUMES
#define FF_VOLUMES CONFIG_FS_FATFS_CUSTOM_MOUNT_POINT_COUNT
#undef FF_VOLUME_STRS
#endif /* CONFIG_FS_FATFS_CUSTOM_MOUNT_POINT_COUNT */

/*
 * Options provided below have been added to ELM FAT source code to
 * support Zephyr specific features, and are not part of ffconf.h.
 */
/*
 * The FS_FATFS_WINDOW_ALIGNMENT is used to align win buffer of FATFS structure
 * to allow more optimal use with MCUs that require specific bufer alignment
 * for DMA to work.
 */
#if defined(CONFIG_FS_FATFS_WINDOW_ALIGNMENT)
#define FS_FATFS_WINDOW_ALIGNMENT	CONFIG_FS_FATFS_WINDOW_ALIGNMENT
#else
#define FS_FATFS_WINDOW_ALIGNMENT	1
#endif /* defined(CONFIG_FS_FATFS_WINDOW_ALIGNMENT) */
