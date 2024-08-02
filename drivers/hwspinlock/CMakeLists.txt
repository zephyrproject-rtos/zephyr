# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_SQN_HWSPINLOCK sqn_hwspinlock.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE hwspinlock_handlers.c)
