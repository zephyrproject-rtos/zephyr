# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Add UICR generator as a utility image
ExternalZephyrProject_Add(
  APPLICATION uicr
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/gen_uicr
)

sysbuild_add_dependencies(CONFIGURE uicr ${DEFAULT_IMAGE})

# Forward the default image's EDT pickle path to the UICR image.
sysbuild_cache_set(VAR DEFAULT_IMAGE_EDT_PICKLE
  ${CMAKE_BINARY_DIR}/${DEFAULT_IMAGE}/zephyr/edt.pickle
)
