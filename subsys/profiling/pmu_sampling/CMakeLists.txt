# SPDX-License-Identifier: Apache-2.0

zephyr_sources(pmu_sampling.c)
zephyr_sources_ifdef(CONFIG_ARM64 pmu_sampling_arm64.c)
