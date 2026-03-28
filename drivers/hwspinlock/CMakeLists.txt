# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_SQN_HWSPINLOCK sqn_hwspinlock.c)
zephyr_library_sources_ifdef(CONFIG_HWSPINLOCK_NXP_SEMA42 hwspinlock_nxp_sema42.c)
zephyr_library_sources_ifdef(CONFIG_HWSPINLOCK_TEST hwspinlock_test.c)
