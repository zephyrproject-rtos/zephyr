# SPDX-License-Identifier: Apache-2.0

# See root CMakeLists.txt for description and expectations of these macros

macro(toolchain_cc_cpp_base_flags dest_list_name)
  list(APPEND ${dest_list_name} "-fcheck-new")
endmacro()

# The "register" keyword was deprecated since C++11, but not for C++98
macro(toolchain_cc_cpp_dialect_std_98_flags dest_list_name)
  list(APPEND ${dest_list_name} "-std=c++98")
endmacro()
macro(toolchain_cc_cpp_dialect_std_11_flags dest_list_name)
  list(APPEND ${dest_list_name} "-std=c++11")
  list(APPEND ${dest_list_name} "-Wno-register")
endmacro()
macro(toolchain_cc_cpp_dialect_std_14_flags dest_list_name)
  list(APPEND ${dest_list_name} "-std=c++14")
  list(APPEND ${dest_list_name} "-Wno-register")
endmacro()
macro(toolchain_cc_cpp_dialect_std_17_flags dest_list_name)
  list(APPEND ${dest_list_name} "-std=c++17")
  list(APPEND ${dest_list_name} "-Wno-register")
endmacro()
macro(toolchain_cc_cpp_dialect_std_2a_flags dest_list_name)
  list(APPEND ${dest_list_name} "-std=c++2a")
  list(APPEND ${dest_list_name} "-Wno-register")
endmacro()

macro(toolchain_cc_cpp_no_exceptions_flag dest_var_name)
  set_ifndef(${dest_var_name}  "-fno-exceptions")
endmacro()

macro(toolchain_cc_cpp_no_rtti_flag dest_var_name)
  set_ifndef(${dest_var_name}  "-fno-rtti")
endmacro()
