# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_SDK_INSTALL_DIR}/cmake/zephyr/target.cmake)

# Workaround, FIXME: Waiting for new SDK.
if("${ARCH}" STREQUAL "xtensa")
  set(SYSROOT_DIR ${TOOLCHAIN_HOME}/xtensa/${SOC_TOOLCHAIN_NAME}/${SYSROOT_TARGET})
  set(CROSS_COMPILE ${TOOLCHAIN_HOME}/xtensa/${SOC_TOOLCHAIN_NAME}/${CROSS_COMPILE_TARGET}/bin/${CROSS_COMPILE_TARGET}-)
endif()
