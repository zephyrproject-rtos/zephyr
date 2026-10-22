# SPDX-License-Identifier: Apache-2.0

zephyr_library_named(zperf)

zephyr_library_sources(
  zperf_api.c
  zperf_common.c
  )
zephyr_library_sources_ifdef(CONFIG_NET_ZPERF_RAW_TX zperf_raw_uploader.c)
zephyr_library_sources_ifdef(CONFIG_NET_ZPERF_IPERF2 zperf_iperf2_client.c)
zephyr_library_sources_ifdef(CONFIG_NET_UDP zperf_udp_pacing.c)

if(CONFIG_NET_ZPERF_IPERF3)
  zephyr_library_sources(zperf_iperf3.c zperf_iperf3_client.c)
endif()

if(CONFIG_NET_ZPERF_SERVER)
  if(CONFIG_NET_ZPERF_IPERF2)
    zephyr_library_sources(zperf_iperf2_server.c zperf_session.c)
  endif()
  zephyr_library_sources_ifdef(CONFIG_NET_ZPERF_IPERF3 zperf_iperf3_server.c)
endif()

zephyr_library_sources_ifdef(CONFIG_NET_SHELL
  zperf_shell.c
)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/subsys/net/ip
)
