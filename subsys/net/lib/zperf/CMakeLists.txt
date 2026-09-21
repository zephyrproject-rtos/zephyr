# SPDX-License-Identifier: Apache-2.0

zephyr_library_named(zperf)

zephyr_library_sources(
  zperf_api.c
  zperf_common.c
  )
zephyr_library_sources_ifdef(CONFIG_NET_ZPERF_RAW_TX zperf_raw_uploader.c)
zephyr_library_sources_ifdef(CONFIG_NET_ZPERF_IPERF2 zperf_iperf2_client.c)

if(CONFIG_NET_ZPERF_SERVER)
  zephyr_library_sources(zperf_session.c)
  zephyr_library_sources_ifdef(CONFIG_NET_ZPERF_IPERF2 zperf_iperf2_server.c)
endif()

zephyr_library_sources_ifdef(CONFIG_NET_SHELL
  zperf_shell.c
)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/subsys/net/ip
)
