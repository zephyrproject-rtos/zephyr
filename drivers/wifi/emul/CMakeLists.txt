# SPDX-FileCopyrightText: 2026 Basalte bv
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(wifi_emul.c)
zephyr_library_sources_ifdef(CONFIG_WIFI_EMUL_SHELL wifi_emul_shell.c)
