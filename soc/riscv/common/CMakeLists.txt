# SPDX-License-Identifier: Apache-2.0

if(CONFIG_RISCV_PMP AND NOT CONFIG_CPU_HAS_CUSTOM_FIXED_SOC_PMP_REGIONS)

  zephyr_library()
  zephyr_library_sources(riscv_pmp_regions.c)

endif()
