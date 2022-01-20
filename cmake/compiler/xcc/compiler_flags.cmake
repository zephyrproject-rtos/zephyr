# No special flags are needed for xcc.
# Only select whether gcc or clang flags should be inherited.
if(CC STREQUAL "clang")
  if($ENV{XCC_NO_G_FLAG})
    set(GCC_NO_G_FLAG 1)
  endif()

  include(${ZEPHYR_BASE}/cmake/compiler/clang/compiler_flags.cmake)
else()
  include(${ZEPHYR_BASE}/cmake/compiler/gcc/compiler_flags.cmake)

  # XCC is based on GCC 4.2 which has a somewhat pedantic take on the
  # fact that linkage semantics differed between C99 and GNU at the
  # time.  Suppress the warning, it's the best we can do given that
  # it's a legacy compiler.
  set_compiler_property(APPEND PROPERTY warning_base "-fgnu89-inline")
endif()
