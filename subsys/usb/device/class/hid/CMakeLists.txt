# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_include_directories(
  # USB headers
  #${ZEPHYR_BASE}/include/zephyr/drivers/usb
  ${ZEPHYR_BASE}/include/zephyr/usb
  ${ZEPHYR_BASE}/subsys/usb/device
  )

zephyr_library_sources(
  core.c
  )
