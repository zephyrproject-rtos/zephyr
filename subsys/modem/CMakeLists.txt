# Copyright (c) 2023 Trackunit Corporation
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_MODEM_MODULES)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_MODEM_CHAT modem_chat.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_CMUX modem_cmux.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_PIPE modem_pipe.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_PIPELINK modem_pipelink.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_PPP modem_ppp.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_STATS modem_stats.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_UBX modem_ubx.c)

add_subdirectory(backends)

endif()
