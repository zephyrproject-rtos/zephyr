/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * Copyright (c) 2023 Husqvarna AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#if FFCONF_DEF != 80286
#error "Configuration version mismatch"
#endif

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

/*
 * These options are override from default values, but have no Kconfig
 * options.
 */
#undef FF_FS_TINY
#define FF_FS_TINY 1

#undef FF_FS_NORTC
#define FF_FS_NORTC 1

/* Zephyr uses FF_VOLUME_STRS */
#undef FF_STR_VOLUME_ID
#define FF_STR_VOLUME_ID 1

/* By default FF_STR_VOLUME_ID in ffconf.h is 0, which means that
 * FF_VOLUME_STRS is not used. Zephyr uses FF_VOLUME_STRS, which
 * by default holds 8 possible strings representing mount points,
 * and FF_VOLUMES needs to reflect that, which means that dolt
 * value of 1 is overridden here with 8.
 */
#undef FF_VOLUMES
#define FF_VOLUMES 8

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

/* CONFIG_FF_VOLUME_STRS_OVERRIDE is used to override hard-coded ffconf.h
 * FF_VOLUMES_STRS content and, instead, provide one from the Kconfig option
 * CONFIG_FF_VOLUME_STRS which - at runtime - will require parsing once.
 */
#if defined(CONFIG_FF_VOLUME_STRS_OVERRIDE)
#undef FF_VOLUME_STRS
#define FF_VOLUME_STRS CONFIG_FF_VOLUME_STRS
#undef FF_VOLUMES
#define FF_VOLUMES CONFIG_FF_VOLUMES
#endif
