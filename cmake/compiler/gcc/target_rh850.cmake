# SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
#
# SPDX-License-Identifier: Apache-2.0

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

if(CONFIG_CPU_G4KH)
  set(RH850_C_FLAGS -mv850e3v5 -mg4 -mrh850-abi)
elseif(CONFIG_CPU_G4MH)
  set(RH850_C_FLAGS -mv850e3v5 -mg4mh2 -mrh850-abi)
else()
  message(FATAL_ERROR "Unsupported RH850 CPU")
endif()

if(CONFIG_FXU)
  list(APPEND RH850_C_FLAGS -mfxu)
endif()

if(NOT CONFIG_FPU)
  list(APPEND RH850_C_FLAGS -msoft-float)
endif()
list(APPEND TOOLCHAIN_C_FLAGS ${RH850_C_FLAGS})
list(APPEND TOOLCHAIN_LD_FLAGS NO_SPLIT ${RH850_C_FLAGS})
