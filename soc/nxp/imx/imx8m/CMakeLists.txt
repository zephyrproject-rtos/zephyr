# Copyright 2024 NXP
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_MIMX8MP_ADSP)
  zephyr_include_directories(adsp)
  add_subdirectory(adsp)

  zephyr_sources(
    adsp/pinctrl_soc.h
  )

  #  west sign

  # See detailed comments in soc/intel/intel_adsp/common/CMakeLists.txt
  add_custom_target(zephyr.ri ALL
    DEPENDS ${CMAKE_BINARY_DIR}/zephyr/zephyr.ri
  )

  add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/zephyr/zephyr.ri
    COMMENT "west sign --if-tool-available --tool rimage ..."
    COMMAND  west sign --if-tool-available --tool rimage --build-dir ${CMAKE_BINARY_DIR} ${WEST_SIGN_OPTS}
    DEPENDS ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME}
  )

  set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/adsp/linker.ld CACHE INTERNAL "")
endif()

if(CONFIG_SOC_MIMX8ML8_A53 OR CONFIG_SOC_MIMX8MM6_A53 OR CONFIG_SOC_MIMX8MN6_A53)
  zephyr_include_directories(.)
  zephyr_include_directories(a53)

  zephyr_sources(
    a53/pinctrl_soc.h
    a53/soc.c
  )

  zephyr_sources_ifdef(CONFIG_ARM_MMU a53/mmu_regions.c)

  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm64/scripts/linker.ld CACHE INTERNAL "")
endif()

if(CONFIG_SOC_MIMX8MM6_M4)
  add_subdirectory(m4_mini)
endif()

if(CONFIG_SOC_MIMX8MQ6_M4)
  add_subdirectory(m4_quad)
endif()

if(CONFIG_SOC_MIMX8MP_M7)
  add_subdirectory(m7)
endif()
