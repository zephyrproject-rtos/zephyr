# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_include_directories(${ZEPHYR_BASE}/drivers)
zephyr_library_sources(mm_drv_common.c mm_drv_bank.c)

zephyr_library_sources_ifdef(
	CONFIG_MM_DRV_INTEL_ADSP_TLB
	mm_drv_intel_adsp_tlb.c
	)

zephyr_sources_ifdef(
	CONFIG_MM_DRV_INTEL_ADSP_MTL_TLB
	mm_drv_intel_adsp_regions.c
	mm_drv_intel_adsp_mtl_tlb.c
	)
