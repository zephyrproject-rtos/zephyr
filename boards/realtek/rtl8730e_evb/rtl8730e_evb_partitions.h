/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Flash partition macros shared by all RTL8730E EVB cores.
 */

#ifndef RTL8730E_EVB_PARTITIONS_H_
#define RTL8730E_EVB_PARTITIONS_H_

#include <mem.h>

/* Partition sizes */
#define BOOT_SLOT_BASE    0x0
#define BOOT_SLOT_SIZE    DT_SIZE_K(256)   /* 0x00040000 - vendor boot.bin slot */
/* Vendor OTA1 span (ameba_flashcfg.c: 0x08040000..0x08300000). The tri-core WiFi
 * image set exceeds 1 MB.
 */
#define APP_SLOT_SIZE     DT_SIZE_K(2816)

/* Partition offsets (calculated) */
#define APP_SLOT0_OFFSET  (BOOT_SLOT_BASE   + BOOT_SLOT_SIZE)  /* 0x00040000 */

/*
 * Settings/NVS storage in the vendor VFS1 region (ameba_flashcfg.c: 0x08640000, 512 KB),
 * clear of both app OTA slots including IMG_APP_OTA2 (0x08340000..0x085FFFFF).
 * Each core's flash node must span >= 8 MB to reach it.
 */
#define STORAGE_OFFSET    0x640000

#endif /* RTL8730E_EVB_PARTITIONS_H_ */
