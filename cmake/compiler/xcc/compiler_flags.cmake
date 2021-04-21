# No special flags are needed for xcc.
# Only select whether gcc or clang flags should be inherited.
if(CC STREQUAL "clang")
  include(${ZEPHYR_BASE}/cmake/compiler/clang/compiler_flags.cmake)
else()
  include(${ZEPHYR_BASE}/cmake/compiler/gcc/compiler_flags.cmake)
endif()
