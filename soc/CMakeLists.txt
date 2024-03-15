# SPDX-License-Identifier: Apache-2.0

# FIXME: SHADOW_VARS: Remove this once we have enabled -Wshadow globally.
#
# Limit warning of shadow variables to in-tree SoC files for now.
cmake_path(IS_PREFIX ZEPHYR_BASE "${SOC_DIR}" NORMALIZE _SOC_IS_IN_TREE)
if(_SOC_IS_IN_TREE)
  add_compile_options($<TARGET_PROPERTY:compiler,warning_shadow_variables>)
endif()
unset(_SOC_IS_IN_TREE)

add_subdirectory(common)

if(HWMv1)
  if(EXISTS ${SOC_DIR}/${ARCH}/CMakeLists.txt)
    add_subdirectory(${SOC_DIR}/${ARCH} soc/${ARCH})
  else()
    add_subdirectory(${SOC_DIR}/${ARCH}/${SOC_PATH} soc/${ARCH}/${SOC_PATH})
  endif()
elseif(HWMv2)
  # Below is inclusion of HWMv2 SoC CMake lists.
  string(TOUPPER SOC_FAMILY_${SOC_FAMILY}_DIR family_setting_dir)
  string(TOUPPER SOC_SERIES_${SOC_SERIES}_DIR series_setting_dir)
  string(TOUPPER SOC_${SOC_NAME}_DIR soc_setting_dir)

  if(DEFINED ${soc_setting_dir})
    add_subdirectory(${${soc_setting_dir}} soc/${SOC_NAME})
  elseif(DEFINED ${series_setting_dir})
    add_subdirectory(${${series_setting_dir}} soc/${SOC_SERIES})
  elseif(DEFINED ${family_setting_dir})
    add_subdirectory(${${family_setting_dir}} soc/${SOC_FAMILY})
  else()
    message(FATAL_ERROR "No CMakeLists.txt file found for SoC: ${SOC_NAME}, "
                        "series: ${SOC_SERIES}, family: ${SOC_FAMILY}")
  endif()

  # Include all SoC roots except Zephyr, as we are already in the Zephyr SoC root.
  set(local_soc_root ${SOC_ROOT})
  list(REMOVE_ITEM local_soc_root ${ZEPHYR_BASE})
  foreach(root ${local_soc_root})
    cmake_path(GET root FILENAME name)
    # A SoC root for HWMv1 may not contain a CMakeLists.txt file on this so
    # let's check for existence before including.
    if(EXISTS ${root}/soc/CMakeLists.txt)
      add_subdirectory(${root}/soc soc/${name})
    endif()
  endforeach()
endif()
