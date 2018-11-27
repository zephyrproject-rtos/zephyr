# LLVM/Clang has significant overlap with GNU/GCC
include(${TOOLCHAIN_ROOT}/cmake/compiler/api_gcc_freestanding.cmake)
include(${TOOLCHAIN_ROOT}/cmake/compiler/api_gcc_optimizations.cmake)
include(${TOOLCHAIN_ROOT}/cmake/compiler/api_gcc_security.cmake)
include(${TOOLCHAIN_ROOT}/cmake/compiler/api_gcc_asm.cmake)
include(${TOOLCHAIN_ROOT}/cmake/compiler/api_gcc_cpp.cmake)
include(${TOOLCHAIN_ROOT}/cmake/compiler/api_gcc_warnings.cmake)

macro(toolchain_cc)
  toolchain_cc_security()
  toolchain_cc_optimizations()
  toolchain_cc_cpp()
  toolchain_cc_freestanding()
  toolchain_cc_warnings()
  toolchain_cc_asm()

  zephyr_cc_option(
    #FIXME: need to fix all of those
    -Wno-sometimes-uninitialized
    -Wno-shift-overflow
    -Wno-missing-braces
    -Wno-self-assign
    -Wno-address-of-packed-member
    -Wno-unused-function
    -Wno-initializer-overrides
    -Wno-section
    -Wno-unknown-warning-option
    -Wno-unused-variable
    -Wno-format-invalid-specifier
    -Wno-gnu
    # comparison of unsigned expression < 0 is always false
    -Wno-tautological-compare
  )
endmacro()
