# Copyright (c) 2025 Infineon Technologies AG,
# or an affiliate of Infineon Technologies AG.
#
# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_BASE}/soc/infineon/edge/pse84/pse84_metadata.cmake)

if(CONFIG_CPU_CORTEX_M33 AND CONFIG_TRUSTED_EXECUTION_SECURE)
  zephyr_sources(soc_pse84_m33_s.c)

  zephyr_sources(security_config/pse84_s_mpc.c)
  zephyr_sources(security_config/pse84_s_protection.c)
  zephyr_sources(security_config/pse84_s_system.c)
  zephyr_sources(security_config/pse84_s_sau.c)

  set(unsigned_hex ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}.hex)
  set(signed_hex ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}.signed.hex)

  pse84_add_metadata_secure_hex(${unsigned_hex} ${signed_hex})
else()
  zephyr_sources_ifdef(CONFIG_CPU_CORTEX_M55 soc_pse84_m55.c)
endif()

zephyr_include_directories(security_config)
zephyr_sources(security_config/pse84_boot.c)

zephyr_sources(mpu_regions.c)

zephyr_sources_ifdef(CONFIG_PM power.c)

zephyr_include_directories(.)

if(${ZEPHYR_TOOLCHAIN_VARIANT} STREQUAL "armclang")
  zephyr_library_sources(cy_syslib_ext.S)
endif()

# Add sections
if(CONFIG_CPU_CORTEX_M33)
  zephyr_linker_sources(SECTIONS shared_mem_sec.ld)
else()
  zephyr_linker_sources(SECTIONS shared_mem.ld)
endif()

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/soc/infineon/edge/pse84/linker_exclude_syslib.ld CACHE INTERNAL "")

zephyr_linker_sources_ifdef(CONFIG_CPU_CORTEX_M33 RWDATA rwdata.ld)
zephyr_linker_sources_ifdef(CONFIG_CPU_CORTEX_M55 RWDATA rwdata.ld)
