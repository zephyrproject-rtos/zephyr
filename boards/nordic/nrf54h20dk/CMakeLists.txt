# SPDX-License-Identifier: Apache-2.0

zephyr_library()
if(CONFIG_NRFS_MRAM_SERVICE_ENABLED)
  zephyr_library_sources(board.c)
endif()
