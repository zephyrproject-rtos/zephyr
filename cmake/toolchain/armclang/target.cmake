# SPDX-License-Identifier: Apache-2.0

# Set CMAKE_SYSTEM_ARCH to ensure CMake ARMClang detection.
if(CONFIG_ARMV8_M_BASELINE)
  set(CMAKE_SYSTEM_ARCH armv8-m.base)
elseif(CONFIG_ARMV8_M_MAINLINE)
  set(CMAKE_SYSTEM_ARCH armv8-m.main)
elseif(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
  set(CMAKE_SYSTEM_ARCH armv6-m)
elseif(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
  set(CMAKE_SYSTEM_ARCH armv7-m)
endif()
