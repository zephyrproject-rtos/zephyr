# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_TAILSCALE_MAP_STREAM map_stream.c)
zephyr_library_sources_ifdef(CONFIG_TAILSCALE_CONTROL_STREAM control_stream.c)
zephyr_library_sources_ifdef(CONFIG_TAILSCALE_PEER_INVENTORY peer_inventory.c)
zephyr_library_sources_ifdef(CONFIG_TAILSCALE_POLICY packet_filter.c)
zephyr_library_sources_ifdef(CONFIG_TAILSCALE_DNS_RESOLVER dns.c)

if(CONFIG_TAILSCALE)
  if(CONFIG_TAILSCALE_REQUIRED_PROXY AND NOT CONFIG_TAILSCALE_DERP)
    message(FATAL_ERROR "CONFIG_TAILSCALE_REQUIRED_PROXY requires CONFIG_TAILSCALE_DERP")
  endif()

  zephyr_library_sources(
    client.c
    control.c
    control_h2.c
    control_noise.c
    ingress.c
    policy.c
    routes.c
    socket.c
    underlay_dns.c
  )
  zephyr_library_sources_ifdef(CONFIG_TAILSCALE_TRANSPORT transport.c disco_box.c)
  zephyr_library_sources_ifdef(CONFIG_TAILSCALE_DERP derp_conn.c)
  zephyr_library_sources_ifdef(CONFIG_TAILSCALE_REQUIRED_PROXY proxy.c)
  zephyr_library_include_directories(../wireguard/crypto)
endif()
