# SPDX-License-Identifier: Apache-2.0

if(CONFIG_ARM64)
  include(aarch64.cmake)
else()
  include(aarch32.cmake)
endif()
