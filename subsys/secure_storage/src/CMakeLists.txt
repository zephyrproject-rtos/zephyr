# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources(
  common.c
)
add_subdirectory(its)

if(CONFIG_SECURE_STORAGE_PS_IMPLEMENTATION_ZEPHYR)
  add_subdirectory(ps)
endif()
