# SPDX-License-Identifier: Apache-2.0

zephyr_library_named(zperf)

zephyr_library_sources(
  zperf_common.c
  zperf_udp_uploader.c
  zperf_tcp_uploader.c
)

if(CONFIG_NET_ZPERF_SERVER)
  zephyr_library_sources(zperf_session.c)
  zephyr_library_sources(zperf_udp_receiver.c)
  zephyr_library_sources(zperf_tcp_receiver.c)
endif()

zephyr_library_sources_ifdef(CONFIG_NET_SHELL
  zperf_shell.c
)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/subsys/net/ip
)
