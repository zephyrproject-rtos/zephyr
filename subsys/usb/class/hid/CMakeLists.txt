# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_include_directories(
  # USB headers
  #${ZEPHYR_BASE}/include/drivers/usb
  ${ZEPHYR_BASE}/include/usb
  ${ZEPHYR_BASE}/subsys/usb
  )

zephyr_library_sources(
  core.c
  )
