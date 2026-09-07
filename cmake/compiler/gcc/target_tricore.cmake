# SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
# SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
# SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
#
# SPDX-License-Identifier: Apache-2.0

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

if(CONFIG_CPU_TC162)
  set(GCC_M_CPU tc39xx)
elseif(CONFIG_CPU_TC18)
  set(GCC_M_CPU tc4DAx)
else()
  message(FATAL_ERROR "Unsupported TriCore CPU")
endif()

set(TRICORE_C_FLAGS "-mcpu=${GCC_M_CPU}")
list(APPEND TOOLCHAIN_C_FLAGS "-mno-eabi-bitfield-limit")
list(APPEND TOOLCHAIN_C_FLAGS "-fstrict-volatile-bitfields")
list(APPEND TOOLCHAIN_C_FLAGS "-Wno-error=pragmas")
# ivopts can form a base pointer outside the object, in an unimplemented
# segment, which the core traps; keep base pointers inside the object.
list(APPEND TOOLCHAIN_C_FLAGS "-fno-ivopts")

list(APPEND TOOLCHAIN_C_FLAGS ${TRICORE_C_FLAGS})
list(APPEND TOOLCHAIN_LD_FLAGS NO_SPLIT ${TRICORE_C_FLAGS})
