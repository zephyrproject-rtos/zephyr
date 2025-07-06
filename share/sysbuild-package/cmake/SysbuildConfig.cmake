# SPDX-License-Identifier: Apache-2.0

if(Sysbuild_FIND_COMPONENTS)
  set(Zephyr_FIND_COMPONENTS ${Sysbuild_FIND_COMPONENTS})
else()
  set(Zephyr_FIND_COMPONENTS sysbuild_default)
endif()
include(${CMAKE_CURRENT_LIST_DIR}/../../zephyr-package/cmake/ZephyrConfig.cmake)
set(Sysbuild_FOUND True)
