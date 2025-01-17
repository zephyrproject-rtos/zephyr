# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_BASE}/cmake/compiler/clang/compiler_flags.cmake)

# nostdinc_include contains path to llvm headers and also relative
# path of "include-fixed".
# Clear "nostdinc" and nostdinc_include
set_compiler_property(PROPERTY nostdinc)
set_compiler_property(PROPERTY nostdinc_include)

# For C++ code, re-add the standard includes directory which was
# cleared up from nostdinc_inlcude in above lines with no
# "include-fixed" this time"
if(CONFIG_CPP)
  execute_process(
    COMMAND ${CMAKE_C_COMPILER} --print-file-name=include/stddef.h
    OUTPUT_VARIABLE _OUTPUT
    COMMAND_ERROR_IS_FATAL ANY
    )
  get_filename_component(_OUTPUT "${_OUTPUT}" DIRECTORY)
  string(REGEX REPLACE "\n" "" _OUTPUT "${_OUTPUT}")
  set_compiler_property(PROPERTY nostdinc_include "${_OUTPUT}")
endif()

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
