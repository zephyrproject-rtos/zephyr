# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-FileCopyrightText: Copyright (c) 2026 Kirill Shypachov
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(stun.c)

zephyr_library_link_libraries_ifdef(CONFIG_MBEDTLS_BUILTIN mbedtls_iface)
