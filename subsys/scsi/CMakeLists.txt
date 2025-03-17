# Copyright (c) 2025 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

if (CONFIG_SCSI)
  zephyr_interface_library_named(SCSI)

  zephyr_library()
  zephyr_library_sources(scsi.c)

endif()
