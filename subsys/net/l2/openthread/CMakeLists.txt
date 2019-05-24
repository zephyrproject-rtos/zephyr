# SPDX-License-Identifier: Apache-2.0

zephyr_library_named(subsys__net__ip__l2__openthread)
zephyr_library_include_directories(. ../../lib/openthread/platform
  ${ZEPHYR_BASE}/subsys/net/ip)
zephyr_library_compile_definitions_ifdef(
  CONFIG_NEWLIB_LIBC __LINUX_ERRNO_EXTENSIONS__
  )

zephyr_library_sources(
  openthread.c
  openthread_utils.c
  )

add_dependencies(subsys__net__ip__l2__openthread
  ot
  )
