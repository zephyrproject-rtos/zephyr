# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(mm_drv_common.c)

zephyr_library_sources_ifdef(
  CONFIG_MM_DRV_INTEL_ADSP_TLB
  mm_drv_intel_adsp_tlb.c
)
