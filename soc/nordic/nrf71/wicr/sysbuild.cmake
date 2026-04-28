# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Add WICR generator as a utility image
ExternalZephyrProject_Add(
  APPLICATION wicr
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/gen_wicr
)

sysbuild_add_dependencies(CONFIGURE wicr ${DEFAULT_IMAGE})

# Forward the default image's EDT pickle path to the WICR image.
sysbuild_cache_set(VAR DEFAULT_IMAGE_EDT_PICKLE
  ${CMAKE_BINARY_DIR}/${DEFAULT_IMAGE}/zephyr/edt.pickle
)
