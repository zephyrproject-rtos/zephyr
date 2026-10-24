# Copyright (c) 2026 Neoventor AB
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

set(DTC_OVERLAY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/boards/native_sim.overlay")
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(cdc_acm_work)

target_sources(app PRIVATE
  app_anchor.c
)

# Keep the driver in its usual library, but compile it through the test seam.
set(driver "${ZEPHYR_BASE}/subsys/usb/device_next/class/usbd_cdc_acm.c")
file(READ "${driver}" driver_text)
if(driver_text MATCHES "struct k_work_delayable rx_fifo_work")
  target_compile_definitions(subsys__usb__device_next PRIVATE RX_WORK_DELAYABLE=1)
else()
  target_compile_definitions(subsys__usb__device_next PRIVATE RX_WORK_DELAYABLE=0)
endif()

# Observe send-helper entry without changing the production source file.
set(enqueue_entry "static void cdc_acm_tx_fifo_enqueue(const struct device *dev)\n{")
string(FIND "${driver_text}" "${enqueue_entry}" enqueue_position)
if(enqueue_position EQUAL -1)
  message(FATAL_ERROR "Expected CDC ACM send enqueue entry")
endif()
string(REPLACE "${enqueue_entry}" "${enqueue_entry}\n\ttx_enqueue_entered(dev);"
  driver_text "${driver_text}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/usbd_cdc_acm_test_driver.c" "${driver_text}")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${driver}")

get_target_property(sources subsys__usb__device_next SOURCES)
set(remaining)
set(replaced 0)
foreach(source IN LISTS sources)
  if(source MATCHES "class/usbd_cdc_acm\\.c$")
    math(EXPR replaced "${replaced} + 1")
  else()
    list(APPEND remaining "${source}")
  endif()
endforeach()
if(NOT replaced EQUAL 1)
  message(FATAL_ERROR "Expected one CDC ACM driver source, found ${replaced}")
endif()
set_property(TARGET subsys__usb__device_next PROPERTY SOURCES "${remaining}")
target_sources(subsys__usb__device_next PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/src/driver_test.c"
)
target_include_directories(subsys__usb__device_next BEFORE PRIVATE
  "${CMAKE_CURRENT_BINARY_DIR}"
  "${ZEPHYR_BASE}/subsys/usb/device_next/class"
  "${ZEPHYR_BASE}/drivers/usb/udc"
)
