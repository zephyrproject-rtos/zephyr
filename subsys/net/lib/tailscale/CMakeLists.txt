# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_TAILSCALE_MAP_STREAM map_stream.c)
zephyr_library_sources_ifdef(CONFIG_TAILSCALE_CONTROL_STREAM control_stream.c)
zephyr_library_sources_ifdef(CONFIG_TAILSCALE_PEER_INVENTORY peer_inventory.c)
zephyr_library_sources_ifdef(CONFIG_TAILSCALE_POLICY packet_filter.c)
