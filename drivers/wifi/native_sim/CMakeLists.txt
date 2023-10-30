# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_WIFI_NATIVE_SIM
  wifi_drv.c
  mgmt_api.c
  iface_api.c
  wifi_drv_api.c
  )

target_sources(native_simulator INTERFACE wifi_drv_adapt.c)
