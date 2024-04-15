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
function(kconfig_gen bin_dir file dirs)
  file(MAKE_DIRECTORY "${bin_dir}")
  set(kconfig_file ${bin_dir}/${file})
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
    cmake_parse_arguments(SOC_V2 "" "NAME;DIR;HWM" "" ${line})

    list(APPEND kconfig_soc_source_dir "${SOC_V2_DIR}")

    if(HWM_TYPE STREQUAL "soc")
      set(setting_name SOC_${SOC_V2_NAME}_DIR)
    else()
      set(setting_name SOC_${HWM_TYPE}_${SOC_V2_NAME}_DIR)
    endif()
    # We support both SOC_foo_DIR and SOC_FOO_DIR.
    set(${setting_name} ${SOC_V2_DIR})
    string(TOUPPER ${setting_name} setting_name)
    set(${setting_name} ${SOC_V2_DIR})
  endif()

  if(idx EQUAL -1)
    break()
  endif()
endwhile()
list(REMOVE_DUPLICATES kconfig_soc_source_dir)

# Support multiple ARCH_ROOT and SOC_ROOT
set(arch_kconfig_file  Kconfig)
set(soc_defconfig_file Kconfig.defconfig)
set(soc_zephyr_file    Kconfig)
set(soc_kconfig_file   Kconfig.soc)
set(soc_sysbuild_file  Kconfig.sysbuild)
set(arch_kconfig_header "# Load arch Kconfig descriptions.\n")
set(defconfig_header    "# Load Zephyr SoC Kconfig defconfig.\n")
set(soc_zephyr_header   "# Load Zephyr SoC Kconfig descriptions.\n")
set(soc_kconfig_header  "# Load SoC Kconfig descriptions.\n")
set(soc_sysbuild_header "# Load SoC sysbuild Kconfig descriptions.\n")
file(WRITE ${KCONFIG_BINARY_DIR}/arch/${arch_kconfig_file} "${arch_kconfig_header}")
file(WRITE ${KCONFIG_BINARY_DIR}/soc/${soc_defconfig_file} "${defconfig_header}")
file(WRITE ${KCONFIG_BINARY_DIR}/soc/${soc_zephyr_file}    "${soc_zephyr_header}")
file(WRITE ${KCONFIG_BINARY_DIR}/soc/${soc_kconfig_file}   "${soc_kconfig_header}")
file(WRITE ${KCONFIG_BINARY_DIR}/soc/${soc_sysbuild_file}  "${soc_sysbuild_header}")

kconfig_gen("${KCONFIG_BINARY_DIR}/arch" "${arch_kconfig_file}" "${kconfig_arch_source_dir}")
kconfig_gen("${KCONFIG_BINARY_DIR}/soc" "${soc_defconfig_file}" "${kconfig_soc_source_dir}")
kconfig_gen("${KCONFIG_BINARY_DIR}/soc" "${soc_zephyr_file}" "${kconfig_soc_source_dir}")
kconfig_gen("${KCONFIG_BINARY_DIR}/soc" "${soc_kconfig_file}" "${kconfig_soc_source_dir}")
kconfig_gen("${KCONFIG_BINARY_DIR}/soc" "${soc_sysbuild_file}" "${kconfig_soc_source_dir}")

# Clear variables created by cmake_parse_arguments
unset(SOC_V2_NAME)
unset(SOC_V2_DIR)
unset(SOC_V2_HWM)
unset(ARCH_V2_NAME)
unset(ARCH_V2_DIR)
