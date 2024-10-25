# SPDX-License-Identifier: Apache-2.0

if(CONFIG_LIBCXX_LIBCPP)
  # LLVM C++ Standard Library makes use of POSIX and GNU C extension functions
  # in its headers.
  zephyr_compile_definitions($<$<COMPILE_LANGUAGE:CXX>:_POSIX_C_SOURCE=200809L>)
  zephyr_compile_definitions($<$<COMPILE_LANGUAGE:CXX>:_XOPEN_SOURCE=700>)
  zephyr_compile_definitions($<$<COMPILE_LANGUAGE:CXX>:_GNU_SOURCE>)
endif()

add_subdirectory(abi)

add_subdirectory_ifdef(CONFIG_MINIMAL_LIBCPP minimal)
