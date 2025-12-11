# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)
zephyr_include_directories(${ZEPHYR_BASE}/subsys/net/ip)

zephyr_library_sources(capture.c)

if(CONFIG_NET_CAPTURE_COOKED_MODE)
  zephyr_library_sources(cooked.c)
endif()
