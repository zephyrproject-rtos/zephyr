# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS "$ENV{ZEPHYR_BASE}")
project(tailscale_map_stream)

target_include_directories(app PRIVATE "${ZEPHYR_BASE}/subsys/net/lib/tailscale")
target_sources_ifdef(CONFIG_TAILSCALE_MAP_STREAM app PRIVATE src/map_stream.c)
target_sources_ifdef(CONFIG_TAILSCALE_CONTROL_STREAM app PRIVATE src/control_stream.c)
target_sources_ifdef(CONFIG_TAILSCALE_PEER_INVENTORY app PRIVATE src/peer_inventory.c)
target_sources_ifdef(CONFIG_TAILSCALE_POLICY app PRIVATE src/policy.c)
target_sources_ifdef(CONFIG_TAILSCALE_POLICY app PRIVATE src/dns_policy.c)
target_sources_ifdef(CONFIG_TAILSCALE_POLICY app PRIVATE src/named_filter.c)
