# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

if(CONFIG_SOC_MIMX9529_M7)
  add_subdirectory(m7)
endif()
