# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_SNTP
  sntp.c
)

zephyr_sources_ifdef(CONFIG_SNTP_CLIENT
  sntp_simple.c
)

zephyr_sources_ifdef(CONFIG_SNTP_SERVER
  sntp_server.c
)
