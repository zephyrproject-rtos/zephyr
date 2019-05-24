# SPDX-License-Identifier: Apache-2.0

# See root CMakeLists.txt for description and expectations of this macro
macro(toolchain_cc_security_fortify)

  if(NOT CONFIG_NO_OPTIMIZATIONS)
    # _FORTIFY_SOURCE: Detect common-case buffer overflows for certain functions
    # _FORTIFY_SOURCE=1 : Compile-time checks (requires -O1 at least)
    # _FORTIFY_SOURCE=2 : Additional lightweight run-time checks
    zephyr_compile_definitions(
      _FORTIFY_SOURCE=2
    )
  endif()

endmacro()
