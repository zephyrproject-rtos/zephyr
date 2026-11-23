# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#
# This file is the INHERIT equivalent to
# https://github.com/ARM-software/psa-arch-tests/blob/master/api-tests/tools/cmake/compiler/GNUARM.cmake
#
# The INHERIT concept was introduced in
# https://github.com/ARM-software/psa-arch-tests/pull/276

if(${CPU_ARCH} STREQUAL armv7m)
  set(TARGET_SWITCH "-march=armv7-m")
elseif(${CPU_ARCH} STREQUAL armv8m_ml)
  set(TARGET_SWITCH "-march=armv8-m.main -mcmse")
elseif(${CPU_ARCH} STREQUAL armv8m_bl)
  set(TARGET_SWITCH "-march=armv8-m.base -mcmse")
endif()

set(CMAKE_C_FLAGS   "${TARGET_SWITCH} -g -Wall -Werror -Wextra -fdata-sections -ffunction-sections -mno-unaligned-access")
set(CMAKE_ASM_FLAGS "${TARGET_SWITCH} -mthumb")
set(CMAKE_EXE_LINKER_FLAGS "-Xlinker --fatal-warnings -Xlinker --gc-sections -z max-page-size=0x400 -lgcc -lc -lnosys")
