# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  addr_utils.c
)

zephyr_sources_ifdef(CONFIG_NET_URL url.c)
