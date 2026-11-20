# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(rfc2217.c)
zephyr_library_sources_ifdef(CONFIG_NET_RFC2217_SERVER rfc2217_server.c)
