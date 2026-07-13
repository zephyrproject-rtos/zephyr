# SPDX-FileCopyrightText: Copyright (c) 2026 STMicroelectronics
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
zephyr_sources_ifdef(CONFIG_GRAPHIC operation.c)
zephyr_sources_ifdef(CONFIG_GRAPHIC_SHELL graphic_shell.c)
# zephyr-keep-sorted-stop
