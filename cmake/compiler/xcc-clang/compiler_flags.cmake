# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_BASE}/cmake/compiler/clang/compiler_flags.cmake)

# Clear "nostdinc"
set_compiler_property(PROPERTY nostdinc)
set_compiler_property(PROPERTY nostdinc_include)

if($ENV{XCC_NO_G_FLAG})
  # Older xcc/clang cannot use "-g" due to this bug:
  # https://bugs.llvm.org/show_bug.cgi?id=11740.
  # Clear the related flag(s) here so it won't cause issues.
  set_compiler_property(PROPERTY debug)
endif()

# Clang version used by Xtensa does not support -fno-pic and -fno-pie
set_compiler_property(PROPERTY no_position_independent "")
