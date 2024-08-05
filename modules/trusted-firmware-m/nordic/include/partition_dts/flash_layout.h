/*
 * Copyright (c) 2018-2022 Arm Limited. All rights reserved.
 * Copyright (c) 2020 Nordic Semiconductor ASA. All rights reserved.
 * Copyright (c) 2024 Embeint Inc
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __FLASH_LAYOUT_H__
#define __FLASH_LAYOUT_H__

/* This header file is included from linker scatter file as well, where only a
 * limited C constructs are allowed. Therefore it is not possible to include
 * here the platform_base_address.h to access flash related defines. To resolve
 * this some of the values are redefined here with different names, these are
 * marked with comment.
 */

#include <zephyr/tfm_devicetree_generated.h>

#if !defined(MCUBOOT_IMAGE_NUMBER) || (MCUBOOT_IMAGE_NUMBER == 1)
/* Secure + Non-secure image primary slot */
#define FLASH_AREA_0_ID           (1)
#define FLASH_AREA_0_OFFSET       (FLASH_S_PARTITION_OFFSET)
#define FLASH_AREA_0_SIZE         (FLASH_S_PARTITION_SIZE + FLASH_NS_PARTITION_SIZE)
/* Secure + Non-secure secondary slot */
#define FLASH_AREA_2_ID           (2)
#define FLASH_AREA_2_OFFSET       (FLASH_S1_S_PARTITION_OFFSET)
#define FLASH_AREA_2_SIZE         (FLASH_S1_S_PARTITION_SIZE + FLASH_S1_NS_PARTITION_SIZE)
/* Scratch slot */
#define FLASH_AREA_SCRATCH_ID     (3)
#define FLASH_AREA_SCRATCH_OFFSET (FLASH_SCRATCH_PARTITION_OFFSET)
#define FLASH_AREA_SCRATCH_SIZE   (FLASH_SCRATCH_PARTITION_SIZE)
#elif (MCUBOOT_IMAGE_NUMBER == 2)
/* Secure image primary slot */
#define FLASH_AREA_0_ID           (1)
#define FLASH_AREA_0_OFFSET       (FLASH_S_PARTITION_OFFSET)
#define FLASH_AREA_0_SIZE         (FLASH_S_PARTITION_SIZE)
/* Non-secure image primary slot */
#define FLASH_AREA_1_ID           (2)
#define FLASH_AREA_1_OFFSET       (FLASH_NS_PARTITION_OFFSET)
#define FLASH_AREA_1_SIZE         (FLASH_NS_PARTITION_SIZE)
/* Secure secondary slot */
#define FLASH_AREA_2_ID           (3)
#define FLASH_AREA_2_OFFSET       (FLASH_S1_S_PARTITION_OFFSET)
#define FLASH_AREA_2_SIZE         (FLASH_S1_S_PARTITION_SIZE)
/* Non-secure secondary slot */
#define FLASH_AREA_3_ID           (4)
#define FLASH_AREA_3_OFFSET       (FLASH_S1_NS_PARTITION_OFFSET)
#define FLASH_AREA_3_SIZE         (FLASH_S1_NS_PARTITION_SIZE)
/* Scratch slot */
#define FLASH_AREA_SCRATCH_ID     (5)
#define FLASH_AREA_SCRATCH_OFFSET (FLASH_SCRATCH_PARTITION_OFFSET)
#define FLASH_AREA_SCRATCH_SIZE   (FLASH_SCRATCH_PARTITION_SIZE)

#else /* MCUBOOT_IMAGE_NUMBER > 2 */
#error "Only MCUBOOT_IMAGE_NUMBER 1 or 2 are supported!"
#endif /* MCUBOOT_IMAGE_NUMBER */
/* Maximum number of image sectors supported by the bootloader. */
#define MCUBOOT_MAX_IMG_SECTORS                                                                    \
	((FLASH_S_PARTITION_SIZE + FLASH_NS_PARTITION_SIZE) / FLASH_AREA_IMAGE_SECTOR_SIZE)

#if FLASH_AREA_SCRATCH_SIZE > 0
#define MCUBOOT_STATUS_MAX_ENTRIES                                                                 \
	((FLASH_S_PARTITION_SIZE + FLASH_NS_PARTITION_SIZE) / FLASH_AREA_SCRATCH_SIZE)
#else
#define MCUBOOT_STATUS_MAX_ENTRIES (0)
#endif

/* In this target the CMSIS driver requires only the offset from the base
 * address instead of the full memory address.
 */
/* Base address of dedicated flash area for PS */
#define TFM_HAL_PS_FLASH_AREA_ADDR FLASH_PS_AREA_OFFSET
/* Size of dedicated flash area for PS */
#define TFM_HAL_PS_FLASH_AREA_SIZE FLASH_PS_AREA_SIZE
#define PS_RAM_FS_SIZE             TFM_HAL_PS_FLASH_AREA_SIZE

/* In this target the CMSIS driver requires only the offset from the base
 * address instead of the full memory address.
 */
/* Base address of dedicated flash area for ITS */
#define TFM_HAL_ITS_FLASH_AREA_ADDR FLASH_ITS_AREA_OFFSET
/* Size of dedicated flash area for ITS */
#define TFM_HAL_ITS_FLASH_AREA_SIZE FLASH_ITS_AREA_SIZE
#define ITS_RAM_FS_SIZE             TFM_HAL_ITS_FLASH_AREA_SIZE

/* OTP / NV counter definitions */
#define TFM_OTP_NV_COUNTERS_AREA_SIZE   (FLASH_OTP_NV_COUNTERS_AREA_SIZE / 2)
#define TFM_OTP_NV_COUNTERS_AREA_ADDR   FLASH_OTP_NV_COUNTERS_AREA_OFFSET
#define TFM_OTP_NV_COUNTERS_SECTOR_SIZE FLASH_OTP_NV_COUNTERS_SECTOR_SIZE
#define TFM_OTP_NV_COUNTERS_BACKUP_AREA_ADDR                                                       \
	(TFM_OTP_NV_COUNTERS_AREA_ADDR + TFM_OTP_NV_COUNTERS_AREA_SIZE)

#endif /* __FLASH_LAYOUT_H__ */
