# SPDX-License-Identifier: Apache-2.0

zephyr_library()

if(${CMAKE_HOST_SYSTEM_NAME} STREQUAL Linux)
  zephyr_library_sources(linux_fuel_gauge.c)
  target_sources(native_simulator INTERFACE linux_fuel_gauge_bottom.c)
else()
  message(FATAL_ERROR "CONFIG_NATIVE_LINUX_FUEL_GAUGE is only available on Linux")
endif()
