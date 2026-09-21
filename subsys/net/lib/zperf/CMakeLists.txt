# SPDX-License-Identifier: Apache-2.0

zephyr_library_named(zperf)

zephyr_library_sources(
  zperf_api.c
  zperf_common.c
  zperf_iperf2_client.c
  )
zephyr_library_sources_ifdef(CONFIG_NET_ZPERF_RAW_TX zperf_raw_uploader.c)

zephyr_library_sources_ifdef(CONFIG_NET_ZPERF_SERVER
  zperf_iperf2_server.c
  zperf_session.c
  )

zephyr_library_sources_ifdef(CONFIG_NET_SHELL
  zperf_shell.c
)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/subsys/net/ip
)
