# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_BASE}/cmake/compiler/clang/compiler_flags.cmake)

# nostdinc needs to be cleared as it is needed for xtensa/config/core.h.
# nostdinc_include contains path to llvm headers.
set_compiler_property(PROPERTY nostdinc)
set_compiler_property(APPEND PROPERTY nostdinc_include ${NOSTDINC})

if($ENV{XCC_NO_G_FLAG})
  # Older xcc/clang cannot use "-g" due to this bug:
  # https://bugs.llvm.org/show_bug.cgi?id=11740.
  # Clear the related flag(s) here so it won't cause issues.
  set_compiler_property(PROPERTY debug)
endif()

# Clang version used by Xtensa does not support -fno-pic and -fno-pie
set_compiler_property(PROPERTY no_position_independent "")

# Remove after testing that -Wshadow works
set_compiler_property(PROPERTY warning_shadow_variables)

# xt-clang is usually based on older version of clang, and
# Zephyr main targets more recent versions. Because of this,
# some newer compiler flags may cause warnings where twister
# would mark as test being failed. To workaround that,
# add -Wno-unknown-warning-option to suppress those warnings.
check_set_compiler_property(APPEND PROPERTY warning_extended
                            -Wno-unknown-warning-option
)
