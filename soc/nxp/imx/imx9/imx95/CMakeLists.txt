# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

if(${CONFIG_MCUX_CORE_SUFFIX} STREQUAL "_cm7")
  add_subdirectory(m7)
elseif(${CONFIG_MCUX_CORE_SUFFIX} STREQUAL "_ca55")
  add_subdirectory(a55)
endif()
