# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2023, Nordic Semiconductor ASA

# This CMake module works together with the list_hardware.py script to obtain
# all archs and SoC implementations defined in the Zephyr build system.
#
# The result from list_hardware.py is then used to generate Kconfig files for
# the build system.
#
# The following files are generated in '<kconfig-binary-dir>/soc'
# - Kconfig.defconfig: Contains references to SoC defconfig files for Zephyr integration.
# - Kconfig: Contains references to regular SoC Kconfig files for Zephyr integration.
# - Kconfig.soc: Contains references to generic SoC Kconfig files.
# - Kconfig.sysbuild: Contains references to SoC sysbuild Kconfig files.
#
# The following file is generated in '<kconfig-binary-dir>/arch'
# - Kconfig: Contains references to regular arch Kconfig files for Zephyr integration.

include_guard(GLOBAL)

if(NOT HWMv2)
  return()
endif()

# Internal helper function for creation of Kconfig files.
function(kconfig_gen bin_dir file dirs comment)
  set(kconfig_header "# Load ${comment} descriptions.\n")
  set(kconfig_file ${KCONFIG_BINARY_DIR}/${bin_dir}/${file})
  file(WRITE ${kconfig_file} "${kconfig_header}")

  foreach(dir ${dirs})
    cmake_path(CONVERT "${dir}" TO_CMAKE_PATH_LIST dir)
    file(APPEND ${kconfig_file} "osource \"${dir}/${file}\"\n")
  endforeach()
endfunction()

# 'SOC_ROOT' and 'ARCH_ROOT' are prioritized lists of directories where their
# implementations may be found. It always includes ${ZEPHYR_BASE}/[arch|soc]
# at the lowest priority.
list(APPEND SOC_ROOT ${ZEPHYR_BASE})
list(APPEND ARCH_ROOT ${ZEPHYR_BASE})

list(TRANSFORM ARCH_ROOT PREPEND "--arch-root=" OUTPUT_VARIABLE arch_root_args)
list(TRANSFORM SOC_ROOT PREPEND "--soc-root=" OUTPUT_VARIABLE soc_root_args)

execute_process(COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/list_hardware.py
                ${arch_root_args} ${soc_root_args}
                --archs --socs
                --cmakeformat={TYPE}\;{NAME}\;{DIR}\;{HWM}
                OUTPUT_VARIABLE ret_hw
                ERROR_VARIABLE err_hw
                RESULT_VARIABLE ret_val
)
if(ret_val)
  message(FATAL_ERROR "Error listing hardware.\nError message: ${err_hw}")
endif()

set(kconfig_soc_source_dir)

while(TRUE)
  string(FIND "${ret_hw}" "\n" idx REVERSE)
  math(EXPR start "${idx} + 1")
  string(SUBSTRING "${ret_hw}" ${start} -1 line)
  string(SUBSTRING "${ret_hw}" 0 ${idx} ret_hw)

  cmake_parse_arguments(HWM "" "TYPE" "" ${line})
  if(HWM_TYPE STREQUAL "arch")
    cmake_parse_arguments(ARCH_V2 "" "NAME;DIR" "" ${line})

    list(APPEND kconfig_arch_source_dir "${ARCH_V2_DIR}")
    list(APPEND ARCH_V2_NAME_LIST ${ARCH_V2_NAME})
    string(TOUPPER "${ARCH_V2_NAME}" ARCH_V2_NAME_UPPER)
    set(ARCH_V2_${ARCH_V2_NAME_UPPER}_DIR ${ARCH_V2_DIR})
  elseif(HWM_TYPE MATCHES "^soc|^series|^family")
    cmake_parse_arguments(SOC_V2 "" "NAME;HWM" "DIR" ${line})

    list(APPEND kconfig_soc_source_dir "${SOC_V2_DIR}")
    string(TOUPPER "${SOC_V2_NAME}" SOC_V2_NAME_UPPER)
    string(TOUPPER "${HWM_TYPE}" HWM_TYPE_UPPER)

    if(HWM_TYPE STREQUAL "soc")
      # We support both SOC_foo_DIR and SOC_FOO_DIR.
      set(SOC_${SOC_V2_NAME}_DIRECTORIES ${SOC_V2_DIR})
      set(SOC_${SOC_V2_NAME_UPPER}_DIRECTORIES ${SOC_V2_DIR})
      list(GET SOC_V2_DIR 0 SOC_${SOC_V2_NAME}_DIR)
      list(GET SOC_V2_DIR 0 SOC_${SOC_V2_NAME_UPPER}_DIR)
    else()
      # We support both SOC_series_foo_DIR and SOC_SERIES_FOO_DIR (and family /  FAMILY).
      set(SOC_${HWM_TYPE}_${SOC_V2_NAME}_DIR ${SOC_V2_DIR})
      set(SOC_${HWM_TYPE_UPPER}_${SOC_V2_NAME_UPPER}_DIR ${SOC_V2_DIR})
    endif()
  endif()

  if(idx EQUAL -1)
    break()
  endif()
endwhile()
list(REMOVE_DUPLICATES kconfig_soc_source_dir)

# Support multiple ARCH_ROOT, SOC_ROOT and BOARD_ROOT
kconfig_gen("arch" "Kconfig"             "${kconfig_arch_source_dir}" "Zephyr Arch Kconfig")
kconfig_gen("soc"  "Kconfig.defconfig"   "${kconfig_soc_source_dir}"  "Zephyr SoC defconfig")
kconfig_gen("soc"  "Kconfig"             "${kconfig_soc_source_dir}"  "Zephyr SoC Kconfig")
kconfig_gen("soc"  "Kconfig.soc"         "${kconfig_soc_source_dir}"  "SoC Kconfig")
kconfig_gen("soc"  "Kconfig.sysbuild"    "${kconfig_soc_source_dir}"  "Sysbuild SoC Kconfig")
kconfig_gen("boards" "Kconfig.defconfig" "${BOARD_DIRECTORIES}"       "Zephyr board defconfig")
kconfig_gen("boards" "Kconfig.${BOARD}"  "${BOARD_DIRECTORIES}"       "board Kconfig")
kconfig_gen("boards" "Kconfig"           "${BOARD_DIRECTORIES}"       "Zephyr board Kconfig")
kconfig_gen("boards" "Kconfig.sysbuild"  "${BOARD_DIRECTORIES}"       "Sysbuild board Kconfig")

# Clear variables created by cmake_parse_arguments
unset(SOC_V2_NAME)
unset(SOC_V2_DIR)
unset(SOC_V2_HWM)
unset(ARCH_V2_NAME)
unset(ARCH_V2_DIR)
