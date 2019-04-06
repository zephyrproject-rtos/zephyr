# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_SYS_PM_POLICY_DUMMY policy_dummy.c)
zephyr_sources_ifdef(CONFIG_SYS_PM_POLICY_RESIDENCY policy_residency.c)
