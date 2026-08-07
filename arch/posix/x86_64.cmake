# SPDX-License-Identifier: Apache-2.0

if(CONFIG_64BIT)
  # some gcc versions fail to build without -fPIC
  zephyr_compile_options(-m64 -fPIC)
  zephyr_link_libraries(-m64)

  target_link_options(native_simulator INTERFACE "-m64")
  target_compile_options(native_simulator INTERFACE "-m64")
else()
  zephyr_compile_options(-m32)
  zephyr_link_libraries(-m32)

  target_link_options(native_simulator INTERFACE "-m32")
  target_compile_options(native_simulator INTERFACE "-m32")

  # When building for 32bits x86, gcc defaults to using the old 8087 float arithmetic
  # which causes some issues with literal float comparisons. So we set it
  # to use the SSE2 float path instead
  # (clang defaults to use SSE, but, setting this option for it is safe)
  check_set_compiler_property(APPEND PROPERTY fpsse2 "SHELL:-msse2 -mfpmath=sse")
  zephyr_compile_options($<TARGET_PROPERTY:compiler,fpsse2>)
  target_compile_options(native_simulator INTERFACE "$<TARGET_PROPERTY:compiler,fpsse2>")
endif()
