# SPDX-License-Identifier: Apache-2.0

# See root CMakeLists.txt for description and expectations of this macro

macro(toolchain_cc_asm_base_flags dest_var_name)
  # Specify assembly as the source language for the preprocessor to expect
  set_ifndef(${dest_var_name} "-xassembler-with-cpp")
endmacro()
