# SPDX-FileCopyrightText: The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
zephyr_sources_ifdef(CONFIG_VIDEO buffer.c)
zephyr_sources_ifdef(CONFIG_VIDEO caps.c)
zephyr_sources_ifdef(CONFIG_VIDEO format.c)
zephyr_sources_ifdef(CONFIG_VIDEO frmival.c)
zephyr_sources_ifdef(CONFIG_VIDEO stream.c)
# zephyr-keep-sorted-stop
