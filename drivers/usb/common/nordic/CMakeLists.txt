# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_NRF_USBHS_WRAPPER)
  zephyr_library()
  zephyr_include_directories(.)
  zephyr_library_sources(nrf_usbhs_wrapper.c)
endif()
