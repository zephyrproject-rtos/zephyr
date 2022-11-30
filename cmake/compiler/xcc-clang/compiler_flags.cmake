# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_BASE}/cmake/compiler/clang/compiler_flags.cmake)

# Clear "nostdinc"
set_compiler_property(PROPERTY nostdinc)
set_compiler_property(PROPERTY nostdinc_include)
set_compiler_property(PROPERTY nostdincxx)

if((CMAKE_C_COMPILER_VERSION VERSION_LESS_EQUAL 4) OR
  (CMAKE_CXX_COMPILER_VERSION VERSION_LESS_EQUAL 4))
  if($ENV{XCC_NO_G_FLAG})
    # Older xcc/clang cannot use "-g" due to this bug fixed in Apr 19, 2016:
    # https://github.com/llvm/llvm-project/issues/12112
    # Clear the related flag(s) here so it won't cause issues.
    set_compiler_property(PROPERTY debug)
  endif()
endif()

# Clang version used by Xtensa does not support -fno-pic and -fno-pie
set_compiler_property(PROPERTY no_position_independent "")
