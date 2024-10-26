# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

set_linker_property(NO_CREATE TARGET linker PROPERTY c_library "-lc")
# Default per standard, will be populated by clang/target.cmake based on clang output.
set_linker_property(NO_CREATE TARGET linker PROPERTY rt_library "")
set_linker_property(TARGET linker PROPERTY c++_library "-lc++;-lc++abi")

if(CONFIG_CPP
   AND NOT CONFIG_MINIMAL_LIBCPP
   AND NOT CONFIG_NATIVE_LIBRARY
   # When new link principle is fully introduced, then the below condition can
   # be removed, and instead the external module c++ should use:
   # set_property(TARGET linker PROPERTY c++_library  "<external_c++_lib>")
   AND NOT CONFIG_EXTERNAL_MODULE_LIBCPP
)
  set_property(TARGET linker PROPERTY link_order_library "c++")
endif()

if(NOT CONFIG_MINIMAL_LIBC)
  set_property(TARGET linker APPEND PROPERTY link_order_library "c")
endif()

set_property(TARGET linker APPEND PROPERTY link_order_library "rt")
