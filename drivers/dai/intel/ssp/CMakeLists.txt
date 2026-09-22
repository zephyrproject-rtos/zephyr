# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_DAI_INTEL_SSP		ssp.c)
zephyr_library_compile_options(-std=gnu99)
