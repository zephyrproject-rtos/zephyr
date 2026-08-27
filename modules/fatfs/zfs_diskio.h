/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MODULES_FATFS_ZFS_DISKIO_H_
#define ZEPHYR_MODULES_FATFS_ZFS_DISKIO_H_
/*
 * Header file for Zephyr specific portions of the FatFS disk interface.
 * These APIs are internal to Zephyr's FatFS VFS implementation
 */

/*
 * Values that can be passed to buffer pointer used by disk_ioctl() when
 * sending CTRL_POWER IOCTL
 */
#define DISK_IOCTL_POWER_OFF 0x0
#define DISK_IOCTL_POWER_ON 0x1


#endif /* ZEPHYR_MODULES_FATFS_ZFS_DISKIO_H_ */
