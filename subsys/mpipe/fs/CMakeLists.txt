#
# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_MPIPE_FS)
  zephyr_library_sources(
    mpipe_file_src.c
    mpipe_file_sink.c
  )
endif()
