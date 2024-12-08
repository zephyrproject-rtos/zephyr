# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zperf)

target_sources(app PRIVATE
  src/main.c
  )

if (CONFIG_NET_SAMPLE_CODE_RELOCATE)
  # Relocate key networking stack components and L2 layer to RAM
  zephyr_code_relocate(LIBRARY subsys__net__ip
    LOCATION "${CONFIG_NET_SAMPLE_CODE_RAM_NAME}_TEXT" NOKEEP)
  zephyr_code_relocate(LIBRARY subsys__net
    LOCATION "${CONFIG_NET_SAMPLE_CODE_RAM_NAME}_TEXT" NOKEEP)
if (CONFIG_NET_L2_ETHERNET)
  zephyr_code_relocate(LIBRARY drivers__ethernet
    LOCATION "${CONFIG_NET_SAMPLE_CODE_RAM_NAME}_TEXT" NOKEEP)
  zephyr_code_relocate(LIBRARY subsys__net__l2__ethernet
    LOCATION "${CONFIG_NET_SAMPLE_CODE_RAM_NAME}_TEXT" NOKEEP)
endif()
endif()

if (CONFIG_USB_DEVICE_STACK_NEXT)
  include(${ZEPHYR_BASE}/samples/subsys/usb/common/common.cmake)
endif()
