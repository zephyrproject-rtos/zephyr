# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_ZVFS_EVENTFD zvfs_eventfd.c)
zephyr_library_sources_ifdef(CONFIG_ZVFS_POLL zvfs_poll.c)
