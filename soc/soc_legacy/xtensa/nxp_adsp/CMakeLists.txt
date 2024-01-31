# NXP i.MX8/RT SoC family CMake file
#
# Copyright (c) 2021 NXP
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_NXP_RT595)
  zephyr_compile_definitions(CPU_MIMXRT595SFFOC_dsp)
endif()

add_subdirectory(common)
zephyr_include_directories(${SOC_SERIES}/include)

#  west sign

# See detailed comments in soc/xtensa/intel_adsp/common/CMakeLists.txt
add_custom_target(zephyr.ri ALL
  DEPENDS ${CMAKE_BINARY_DIR}/zephyr/zephyr.ri
)

add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/zephyr/zephyr.ri
  COMMENT "west sign --if-tool-available --tool rimage ..."
  COMMAND  west sign --if-tool-available --tool rimage --build-dir ${CMAKE_BINARY_DIR} ${WEST_SIGN_OPTS}
  DEPENDS ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME}
)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/${SOC_SERIES}/linker.ld CACHE INTERNAL "")
