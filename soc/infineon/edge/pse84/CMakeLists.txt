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

  find_program(EDGEPROTECTTOOLS edgeprotecttools OPTIONAL)

  if(${EDGEPROTECTTOOLS} STREQUAL "EDGEPROTECTTOOLS-NOTFOUND")
    message(WARNING "Could not find edgeprotecttools.
      This will result in a nonfunctional secure cm33.
      Please consult index.rst for kit_pse84_eval board.")
  else()
    message("-- Found edgeprotecttools: ${EDGEPROTECTTOOLS}")

    set(unsigned_hex ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}.hex)
    set(signed_hex ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}.signed.hex)
    dt_nodelabel(m33s_header NODELABEL "m33s_header")
    dt_reg_addr(header_addr PATH ${m33s_header})
    dt_reg_size(header_size PATH ${m33s_header})

    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND edgeprotecttools image-metadata --image ${unsigned_hex} --output ${signed_hex}
        --erased-val 0xff --hex-addr ${header_addr} --header-size ${header_size}
    )

    set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts ${signed_hex})
  endif()
else()
  zephyr_sources_ifdef(CONFIG_CPU_CORTEX_M55 soc_pse84_m55.c)
endif()

zephyr_include_directories(security_config)
zephyr_sources(security_config/pse84_boot.c)

if(CONFIG_CPU_HAS_CUSTOM_FIXED_SOC_MPU_REGIONS)
  zephyr_sources_ifdef(CONFIG_CPU_CORTEX_M55 mpu_regions.c)
endif()

zephyr_include_directories(.)

if(${ZEPHYR_TOOLCHAIN_VARIANT} STREQUAL "armclang")
  zephyr_library_sources(cy_syslib_ext.S)
endif()

# Add sections
if(CONFIG_CPU_CORTEX_M33)
  zephyr_linker_sources(SECTIONS shared_mem_sec.ld)
  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
else()
  zephyr_linker_sources(SECTIONS shared_mem.ld)
  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/soc/infineon/edge/pse84/linker_exclude_syslib.ld CACHE INTERNAL "")
endif()

zephyr_linker_sources_ifdef(CONFIG_CPU_CORTEX_M33 RWDATA rwdata.ld)
zephyr_linker_sources_ifdef(CONFIG_CPU_CORTEX_M55 RWDATA rwdata.ld)
