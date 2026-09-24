# SPDX-License-Identifier: Apache-2.0

add_subdirectory(its)

if(CONFIG_SECURE_STORAGE_PS_IMPLEMENTATION_ZEPHYR)
  add_subdirectory(ps)
endif()
