# SPDX-License-Identifier: Apache-2.0

# See root CMakeLists.txt for description and expectations of this macro
macro(toolchain_cc_security_canaries)

  zephyr_compile_options(-fstack-protector-all)

  # Only a valid option with GCC 7.x and above
  zephyr_cc_option(-mstack-protector-guard=global)

endmacro()
