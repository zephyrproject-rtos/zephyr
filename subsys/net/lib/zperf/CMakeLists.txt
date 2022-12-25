# SPDX-License-Identifier: Apache-2.0

zephyr_library_named(zperf)

zephyr_library_sources(
  zperf_common.c
  zperf_session.c
  zperf_udp_receiver.c
  zperf_udp_uploader.c
  zperf_tcp_receiver.c
  zperf_tcp_uploader.c
)

zephyr_library_sources_ifdef(CONFIG_NET_SHELL
  zperf_shell.c
)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/subsys/net/ip
)
