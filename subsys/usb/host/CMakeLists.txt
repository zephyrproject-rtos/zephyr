# SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR})

zephyr_library_sources(
  usbh_ch9.c
  usbh_class.c
  usbh_core.c
  usbh_api.c
  usbh_device.c
  usbh_desc.c
  usbh_class.c
)

zephyr_library_sources_ifdef(
  CONFIG_USBH_SHELL
  usbh_shell.c
)

zephyr_library_sources_ifdef(
  CONFIG_USBH_VIDEO_CLASS
  class/usbh_uvc.c
)

zephyr_library_sources_ifdef(
  CONFIG_USBIP
  usbip.c
)

zephyr_linker_sources(DATA_SECTIONS usbh_data.ld)
