# SPDX-License-Identifier: Apache-2.0

zephyr_library_named(zperf)

zephyr_library_sources(zperf_common.c)
zephyr_library_sources_ifdef(CONFIG_NET_UDP zperf_udp_uploader.c)
zephyr_library_sources_ifdef(CONFIG_NET_TCP zperf_tcp_uploader.c)

if(CONFIG_NET_ZPERF_SERVER)
  zephyr_library_sources(zperf_session.c)
  zephyr_library_sources_ifdef(CONFIG_NET_UDP zperf_udp_receiver.c)
  zephyr_library_sources_ifdef(CONFIG_NET_TCP zperf_tcp_receiver.c)
endif()

zephyr_library_sources_ifdef(CONFIG_NET_SHELL
  zperf_shell.c
)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/subsys/net/ip
)
