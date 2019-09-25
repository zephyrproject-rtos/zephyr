# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_include_directories(. ${ZEPHYR_BASE}/subsys/net/ip)
zephyr_library_compile_definitions_ifdef(
  CONFIG_NEWLIB_LIBC __LINUX_ERRNO_EXTENSIONS__
  )

zephyr_library_sources_ifdef(CONFIG_NET_L2_CANBUS_RAW canbus_raw.c)
zephyr_library_sources_ifdef(CONFIG_NET_L2_CANBUS 6locan.c)
