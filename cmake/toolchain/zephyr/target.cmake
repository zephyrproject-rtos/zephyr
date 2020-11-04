# SPDX-License-Identifier: Apache-2.0

if(${SDK_VERSION} VERSION_LESS_EQUAL 0.11.2)
  # For backward compatibility with 0.11.1 and 0.11.2
  # we need to source files from Zephyr repo
  include(${CMAKE_CURRENT_LIST_DIR}/${SDK_MAJOR_MINOR}/target.cmake)
elseif("${ARCH}" STREQUAL "sparc")
  # SDK 0.11.3, 0.11.4 and 0.12.0-beta-1 does not have SPARC target support.
  include(${CMAKE_CURRENT_LIST_DIR}/${SDK_MAJOR_MINOR}/target.cmake)
else()
  include(${ZEPHYR_SDK_INSTALL_DIR}/cmake/zephyr/target.cmake)

  # Workaround, FIXME
  if("${ARCH}" STREQUAL "xtensa")
    if("${SOC_SERIES}" STREQUAL "cavs_v15")
      set(SR_XT_TC_SOC intel_apl_adsp)
    elseif("${SOC_SERIES}" STREQUAL "cavs_v18")
      set(SR_XT_TC_SOC intel_s1000)
    elseif("${SOC_SERIES}" STREQUAL "cavs_v20")
      set(SR_XT_TC_SOC intel_s1000)
    elseif("${SOC_SERIES}" STREQUAL "cavs_v25")
      set(SR_XT_TC_SOC intel_s1000)
    elseif("${SOC_SERIES}" STREQUAL "baytrail_adsp")
      set(SR_XT_TC_SOC intel_byt_adsp)
    elseif("${SOC_SERIES}" STREQUAL "broadwell_adsp")
      set(SR_XT_TC_SOC intel_bdw_adsp)
    elseif("${SOC_SERIES}" STREQUAL "intel_s1000")
      set(SR_XT_TC_SOC intel_s1000)
    elseif("${SOC_NAME}" STREQUAL "sample_controller")
      set(SR_XT_TC_SOC sample_controller)
    else()
      message(FATAL_ERROR "No compiler set for SOC_SERIES ${SOC_SERIES}")
    endif()
    set(SYSROOT_DIR ${TOOLCHAIN_HOME}/xtensa/${SR_XT_TC_SOC}/${SYSROOT_TARGET})
    set(CROSS_COMPILE ${TOOLCHAIN_HOME}/xtensa/${SR_XT_TC_SOC}/${CROSS_COMPILE_TARGET}/bin/${CROSS_COMPILE_TARGET}-)
  endif()
endif()
