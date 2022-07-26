#
# Copyright (c) 2022 tangchunhui@coros.com
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_KEY_GPIO key_gpio.c)

zephyr_library_sources_ifdef(CONFIG_USERSPACE key_handlers.c)
