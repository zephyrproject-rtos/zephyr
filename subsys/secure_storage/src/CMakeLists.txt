# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources(
  uid.c
)

if(CONFIG_SECURE_STORAGE_ITS_STORE_IMPLEMENTATION_ZMS OR
   CONFIG_SECURE_STORAGE_PS_STORE_IMPLEMENTATION_ZMS)
  zephyr_library_sources(
    zms.c
  )
endif()

add_subdirectory(its)

if(CONFIG_SECURE_STORAGE_PS_IMPLEMENTATION_ZEPHYR)
  add_subdirectory(ps)
endif()
