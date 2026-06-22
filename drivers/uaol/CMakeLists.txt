# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_UAOL_INTEL_ADSP uaol_intel_adsp.c)
