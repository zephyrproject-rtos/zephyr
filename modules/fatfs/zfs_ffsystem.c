/*
 * OS Dependent Functions for FatFs
 *
 * Copyright (c) 2023 Husqvarna AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/* The file is based on template file by (C)ChaN, 2022, as
 * available from FAT FS module source:
 * https://github.com/zephyrproject-rtos/fatfs/blob/master/option/ffsystem.c
 */

#include <zephyr/kernel.h>

#include <ff.h>

#if FF_USE_LFN == 3	/* Dynamic memory allocation */
/* Allocate a memory block */
void *ff_memalloc(UINT msize)
{
	return k_malloc(msize);
}

/* Free a memory block */
void ff_memfree(void *mblock)
{
	k_free(mblock);
}
#endif /* FF_USE_LFN == 3 */

#if FF_FS_REENTRANT	/* Mutual exclusion */
/* Table of Zephyr mutex. One for each volume and an extra one for the ff system.
 * See also the template file used as reference. Link is available in the header of this file.
 */
static struct k_mutex fs_reentrant_mutex[FF_VOLUMES + 1];

/* Create a Mutex
 * Mutex ID vol: Volume mutex (0 to FF_VOLUMES - 1) or system mutex (FF_VOLUMES)
 * Returns 1: Succeeded or 0: Could not create the mutex
 */
int ff_mutex_create(int vol)
{
	return (int)(k_mutex_init(&fs_reentrant_mutex[vol]) == 0);
}

/* Delete a Mutex
 * Mutex ID vol: Volume mutex (0 to FF_VOLUMES - 1) or system mutex (FF_VOLUMES)
 */
void ff_mutex_delete(int vol)
{
	/* (nothing to do) */
	(void)vol;
}

/* Request Grant to Access the Volume
 * Mutex ID vol: Volume mutex (0 to FF_VOLUMES - 1) or system mutex (FF_VOLUMES)
 * Returns 1: Succeeded or 0: Timeout
 */
int ff_mutex_take(int vol)
{
	return (int)(k_mutex_lock(&fs_reentrant_mutex[vol], FF_FS_TIMEOUT) == 0);
}

/* Release Grant to Access the Volume
 * Mutex ID vol: Volume mutex (0 to FF_VOLUMES - 1) or system mutex (FF_VOLUMES)
 */
void ff_mutex_give(int vol)
{
	k_mutex_unlock(&fs_reentrant_mutex[vol]);
}
#endif /* FF_FS_REENTRANT */
