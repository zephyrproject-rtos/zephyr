# SPDX-FileCopyrightText: Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

config USBH_MSD_CLASS
	bool "USB host Mass Storage Class support [EXPERIMENTAL]"
	select EXPERIMENTAL
	select DISK_ACCESS
	help
	  USB Host Mass Storage Class (MSC) implementation using
	  Bulk-Only Transport (BOT) and SCSI transparent command set.
	  Registers connected USB flash drives as disk_access devices
	  so that file systems (FAT, ext2, etc.) can be mounted on them.

if USBH_MSD_CLASS

config USBH_MSD_INSTANCES_COUNT
	int "Number of MSD host instances"
	default 2
	range 1 4
	help
	  Maximum number of USB Mass Storage devices that can be
	  connected simultaneously.

config USBH_MSD_MAX_SECTORS_PER_XFER
	int "Maximum sectors per single BOT transfer"
	default 16
	range 1 128
	help
	  Limits the number of 512-byte sectors transferred in a single
	  Bulk-Only Transport READ(10) or WRITE(10) transaction.
	  Higher values improve throughput at the cost of larger stack/heap
	  buffers (each sector is 512 bytes).

module = USBH_MSD
module-str = usbh_msd
source "subsys/logging/Kconfig.template.log_config"

endif # USBH_MSD_CLASS
