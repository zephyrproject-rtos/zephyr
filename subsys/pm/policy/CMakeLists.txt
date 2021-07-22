# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_PM_POLICY_DUMMY policy_dummy.c)
zephyr_sources_ifdef(CONFIG_PM_POLICY_RESIDENCY_DEFAULT policy_residency.c)
