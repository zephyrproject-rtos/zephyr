# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_compile_definitions_ifdef(
  CONFIG_NEWLIB_LIBC __LINUX_ERRNO_EXTENSIONS__
  )

zephyr_library_sources_ifdef(CONFIG_NET_L2_DUMMY    dummy.c)

if(CONFIG_NET_PSEUDO_IFACE)
  add_subdirectory(any)
endif()
