# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_MPIPE_NET
  tcp_server.c
  mpipe_tcp_server_sink.c
  mpipe_tcp_server_src.c
)
