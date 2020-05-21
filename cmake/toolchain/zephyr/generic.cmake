# SPDX-License-Identifier: Apache-2.0

if(${SDK_VERSION} VERSION_LESS_EQUAL 0.11.2)
  # For backward compatibility with 0.11.1 and 0.11.2
  # we need to source files from Zephyr repo
  include(${CMAKE_CURRENT_LIST_DIR}/${SDK_MAJOR_MINOR}/generic.cmake)

  set(TOOLCHAIN_KCONFIG_DIR ${CMAKE_CURRENT_LIST_DIR}/${SDK_MAJOR_MINOR})
else()
  include(${ZEPHYR_SDK_INSTALL_DIR}/cmake/zephyr/generic.cmake)

  set(TOOLCHAIN_KCONFIG_DIR ${ZEPHYR_SDK_INSTALL_DIR}/cmake/zephyr)
endif()
