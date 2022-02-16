# No special flags are needed for xcc.
# Only select whether gcc or clang flags should be inherited.
if(CC STREQUAL "clang")
  include(${ZEPHYR_BASE}/cmake/compiler/clang/compiler_flags.cmake)

  # Now, let's overwrite the flags that are different in xcc/clang.
  if($ENV{XCC_NO_G_FLAG})
    # Older xcc/clang cannot use "-g" due to this bug:
    # https://bugs.llvm.org/show_bug.cgi?id=11740.
    # Clear the related flag(s) here so it won't cause issues.
    set_compiler_property(PROPERTY debug)
  endif()
else()
  include(${ZEPHYR_BASE}/cmake/compiler/gcc/compiler_flags.cmake)

  # XCC is based on GCC 4.2 which has a somewhat pedantic take on the
  # fact that linkage semantics differed between C99 and GNU at the
  # time.  Suppress the warning, it's the best we can do given that
  # it's a legacy compiler.
  set_compiler_property(APPEND PROPERTY warning_base "-fgnu89-inline")

  set_compiler_property(PROPERTY warning_error_misra_sane)
endif()
