# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_PM_CPU_OPS pm_cpu_ops_weak_impl.c)

zephyr_sources_ifdef(CONFIG_PM_CPU_OPS_PSCI pm_cpu_ops_psci.c)
