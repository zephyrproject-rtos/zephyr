# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_SDMMC_SUBSYS sdmmc_subsys.c)
zephyr_library_sources_ifdef(CONFIG_MMC_SUBSYS mmc_subsys.c)
add_subdirectory_ifdef(CONFIG_NVME nvme)

# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_DISK_DRIVER_FLASH flashdisk.c)
zephyr_library_sources_ifdef(CONFIG_DISK_DRIVER_FTL ftl_dhara.c)
zephyr_library_sources_ifdef(CONFIG_DISK_DRIVER_LOOPBACK loopback_disk.c)
zephyr_library_sources_ifdef(CONFIG_DISK_DRIVER_MEMC_RAM memc_ramdisk.c)
zephyr_library_sources_ifdef(CONFIG_DISK_DRIVER_RAM ramdisk.c)
zephyr_library_sources_ifdef(CONFIG_DISK_DRIVER_SCSI scsi_disk.c scsi_partition.c)
zephyr_library_sources_ifdef(CONFIG_DISK_DRIVER_VIRTIO_BLK virtio_blk.c)
zephyr_library_sources_ifdef(CONFIG_SDMMC_STM32 sdmmc_stm32.c)
# zephyr-keep-sorted-stop
