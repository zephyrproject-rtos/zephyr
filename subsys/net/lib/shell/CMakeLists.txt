# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources(net_shell.c)
zephyr_library_include_directories(. ${ZEPHYR_BASE}/subsys/net/l2)
zephyr_library_include_directories(. ${ZEPHYR_BASE}/subsys/net/lib)
zephyr_library_include_directories(. ${ZEPHYR_BASE}/subsys/net/ip)
zephyr_library_link_libraries_ifdef(CONFIG_MBEDTLS mbedTLS)

zephyr_library_sources(allocs.c)
zephyr_library_sources(arp.c)
zephyr_library_sources(capture.c)
zephyr_library_sources(conn.c)
zephyr_library_sources(dns.c)
zephyr_library_sources(events.c)
zephyr_library_sources(gptp.c)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ARCH_DIR}/${ARCH}/include
  )
