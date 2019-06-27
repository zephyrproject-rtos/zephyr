# SPDX-License-Identifier: Apache-2.0

# See root CMakeLists.txt for description and expectations of these macros

macro(toolchain_cc_imacros header_file)

  zephyr_compile_options(--imacros=${header_file})

endmacro()
