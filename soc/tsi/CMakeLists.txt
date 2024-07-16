# Copyright (c) 2024 TSI
# SPDX-License-Identifier: Apache-2.0

if(DEFINED SOC_SERIES)
  add_subdirectory(${SOC_SERIES})
else()
  add_subdirectory(${SOC_NAME})
endif()
