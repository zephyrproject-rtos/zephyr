# Copyright (c) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SCSI)
  zephyr_interface_library_named(SCSI)

  zephyr_library()
  zephyr_library_sources(
    scsi_core.c
    scsi_cmd.c
    scsi_sense.c
  )

endif()
