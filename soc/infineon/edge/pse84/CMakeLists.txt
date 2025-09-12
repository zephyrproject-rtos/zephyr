# Copyright (c) 2025 Infineon Technologies AG,
# or an affiliate of Infineon Technologies AG.
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_CPU_CORTEX_M33 AND CONFIG_TRUSTED_EXECUTION_SECURE)
  zephyr_sources(soc_pse84_m33_s.c)

  zephyr_sources(security_config/pse84_s_mpc.c)
  zephyr_sources(security_config/pse84_s_protection.c)
  zephyr_sources(security_config/pse84_s_system.c)
  zephyr_sources(security_config/pse84_s_sau.c)

  # Use secure device in cy_device_headers
  zephyr_compile_definitions(COMPONENT_SECURE_DEVICE)

  # Configuration to use FLASH_BOOT
  zephyr_compile_definitions(FLASH_BOOT)

  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND edgeprotecttools image-metadata --image ${PROJECT_BINARY_DIR}/zephyr.hex  --output ${PROJECT_BINARY_DIR}/zephyr.hex --erased-val 0xff --header-size 0x400 --slot-size 0x130000  --hex-addr 0x60100000
  )
else()
  zephyr_sources_ifdef(CONFIG_CPU_CORTEX_M33 soc_pse84_m33.c)
  zephyr_sources_ifdef(CONFIG_CPU_CORTEX_M55 soc_pse84_m55.c)
endif()

zephyr_include_directories(security_config)
zephyr_sources(security_config/pse84_boot.c)

if(CONFIG_BOARD_KIT_PSE84_EVAL_PSE846GPS2DBZC4A_M55 AND CONFIG_CPU_HAS_CUSTOM_FIXED_SOC_MPU_REGIONS)
  zephyr_sources_ifdef(CONFIG_CPU_CORTEX_M55 mpu_regions.c)
endif()

zephyr_include_directories(.)

if(${ZEPHYR_TOOLCHAIN_VARIANT} STREQUAL "armclang")
  zephyr_library_sources(cy_syslib_ext.S)
endif()

# Add sections
if(CONFIG_BOARD_KIT_PSE84_EVAL_PSE846GPS2DBZC4A_M33)
  zephyr_linker_sources(SECTIONS shared_mem_sec.ld)
  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
else()
  zephyr_linker_sources(SECTIONS shared_mem.ld)
  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/soc/infineon/edge/pse84/linker_exclude_syslib.ld CACHE INTERNAL "")
endif()

zephyr_linker_sources_ifdef(CONFIG_CPU_CORTEX_M33 RWDATA rwdata.ld)
zephyr_linker_sources_ifdef(CONFIG_CPU_CORTEX_M55 RWDATA rwdata.ld)

# Edge family defines
zephyr_compile_definitions_ifdef(CONFIG_CPU_CORTEX_M33 COMPONENT_CM33)
zephyr_compile_definitions_ifdef(CONFIG_CPU_CORTEX_M33 CORE_NAME_CM33_0)
zephyr_compile_definitions_ifdef(CONFIG_CPU_CORTEX_M55 COMPONENT_CM55)
zephyr_compile_definitions_ifdef(CONFIG_CPU_CORTEX_M55 CORE_NAME_CM55_0)
