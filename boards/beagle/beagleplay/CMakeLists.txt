# SPDX-License-Identifier: Apache-2.0

zephyr_library()

if(CONFIG_SOC_CC1352P7)
  zephyr_library_sources(../beagleconnect_freedom/board_antenna.c)
  zephyr_library_compile_definitions("DeviceFamily_CC13X2X7")
endif()
