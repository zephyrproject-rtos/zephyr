# SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
# SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
# SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
#
# SPDX-License-Identifier: Apache-2.0

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

if(CONFIG_CPU_TC162)
  set(GCC_M_CPU tc3xx)
elseif(CONFIG_CPU_TC18)
  set(GCC_M_CPU tc4xx)
else()
  message(FATAL_ERROR "Unsupported TriCore CPU")
endif()

set(TRICORE_C_FLAGS "-mcpu=${GCC_M_CPU}")

list(APPEND TOOLCHAIN_C_FLAGS ${TRICORE_C_FLAGS})
list(APPEND TOOLCHAIN_LD_FLAGS NO_SPLIT ${TRICORE_C_FLAGS})

set(DT_PREPROCESS_EXTRA_CPPFLAGS "${TRICORE_C_FLAGS}" CACHE STRING
  "Target CPU flags for the devicetree preprocess step" FORCE
)

set(LINKER lld)
