# SPDX-License-Identifier: Apache-2.0

# See root CMakeLists.txt for description and expectations of these macros

macro(toolchain_cc_imacros header_file)

  # We cannot use the "-imacros foo" form here as CMake insists on
  # deduplicating arguments, meaning that subsequent usages after the
  # first one will see the "-imacros " part removed.
  # gcc and clang support the "--imacros=foo" form but not xcc.
  # Let's use the "combined" form (without space) which is supported
  # by everyone so far.
  zephyr_compile_options(-imacros${header_file})

endmacro()
