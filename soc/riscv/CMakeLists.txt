# SPDX-License-Identifier: Apache-2.0

add_subdirectory(common)

if(SOC_FAMILY)
  add_subdirectory(${SOC_FAMILY})
else()
  add_subdirectory(${SOC_NAME})
endif()
