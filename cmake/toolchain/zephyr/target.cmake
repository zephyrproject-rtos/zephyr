# SPDX-License-Identifier: Apache-2.0

if(${SDK_VERSION} VERSION_LESS_EQUAL 0.11.2)
  # For backward compatibility with 0.11.1 and 0.11.2
  # we need to source files from Zephyr repo
  include(${CMAKE_CURRENT_LIST_DIR}/${SDK_MAJOR_MINOR}/target.cmake)
elseif(("${ARCH}" STREQUAL "sparc") AND (${SDK_VERSION} VERSION_LESS 0.12))
  # SDK 0.11.3, 0.11.4 does not have SPARC target support.
  include(${CMAKE_CURRENT_LIST_DIR}/${SDK_MAJOR_MINOR}/target.cmake)
else()
  include(${ZEPHYR_SDK_INSTALL_DIR}/cmake/zephyr/target.cmake)

  # Workaround, FIXME: Waiting for new SDK.
  if("${ARCH}" STREQUAL "xtensa")
    set(SYSROOT_DIR ${TOOLCHAIN_HOME}/xtensa/${SOC_TOOLCHAIN_NAME}/${SYSROOT_TARGET})
    set(CROSS_COMPILE ${TOOLCHAIN_HOME}/xtensa/${SOC_TOOLCHAIN_NAME}/${CROSS_COMPILE_TARGET}/bin/${CROSS_COMPILE_TARGET}-)
  endif()
endif()
