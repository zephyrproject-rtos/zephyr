# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_REBOOT reboot.c)

zephyr_include_directories(.)
