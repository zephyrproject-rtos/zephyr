# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_DAI_INTEL_DMIC	dmic.c)
zephyr_library_sources_ifdef(CONFIG_DAI_INTEL_DMIC_NHLT	dmic_nhlt.c)
