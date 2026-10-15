# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(
  CONFIG_NET_BUF
  buf.c
  buf_simple.c
  )

if(CONFIG_NET_UNITY_BUILD)
  zephyr_library_unity()
endif()
