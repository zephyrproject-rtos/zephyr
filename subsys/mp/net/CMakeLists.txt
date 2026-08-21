#
# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_MP_NET)
  zephyr_library_sources(
    tcp_server.c
    mp_tcpsink.c
    mp_tcpsrc.c
  )
endif()
