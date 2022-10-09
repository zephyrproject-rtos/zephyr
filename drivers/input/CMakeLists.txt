# Copyright (c) 2022, tangchunhui@coros.com
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_INPUT input_internal.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE input_handlers.c)

add_subdirectory_ifdef(CONFIG_KEYBOARD keyboard)
