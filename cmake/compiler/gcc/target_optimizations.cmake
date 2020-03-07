# SPDX-License-Identifier: Apache-2.0

# See root CMakeLists.txt for description and expectations of this macro
#
# NOTE: Some GNU toolchains break with plain '-Os' or '-Og', but is fixable
# with tweaks. So allow user to override, via ifndef, the compile flags that
# CONFIG_{NO,DEBUG,SPEED,SIZE}_OPTIMIZATIONS will cause, yet still leaving the
# selection logic in kconfig.
#
# These macros leaves it up to the root CMakeLists.txt to choose the CMake
# variable names to store the optimization flags in.

macro(toolchain_cc_optimize_for_no_optimizations_flag dest_var_name)
  set_ifndef(${dest_var_name}  "-O0")
endmacro()

macro(toolchain_cc_optimize_for_debug_flag dest_var_name)
  # -Og optimisation flag is only supported from gcc 4.8.0 and above.
  # Fall back to using -O0 flag if running an older gcc version.
  if(CMAKE_C_COMPILER_VERSION VERSION_LESS "4.8.0")
    set_ifndef(${dest_var_name}  "-O0")
  else()
    set_ifndef(${dest_var_name}  "-Og")
  endif()
endmacro()

macro(toolchain_cc_optimize_for_speed_flag dest_var_name)
  set_ifndef(${dest_var_name}  "-O2")
endmacro()

macro(toolchain_cc_optimize_for_size_flag dest_var_name)
  set_ifndef(${dest_var_name}  "-Os")
endmacro()
