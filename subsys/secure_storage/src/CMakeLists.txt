# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources(
  uid.c
)

zephyr_library_sources_ifdef(CONFIG_SECURE_STORAGE_ITS_STORE_IMPLEMENTATION_ZMS
  zms.c
)

add_subdirectory(its)
