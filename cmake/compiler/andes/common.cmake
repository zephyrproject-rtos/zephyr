# SPDX-License-Identifier: Apache-2.0

# The libraries (libgcc.a, libc.a, ...) of an Andes toolchain are built only
# for its float ABI (TOOLCHAIN_FLOAT_ABI), so use it instead of the float ABI
# derived from Kconfig.
string(REGEX REPLACE "[fd]$" "" andes_mabi "${riscv_mabi}")
if(TOOLCHAIN_FLOAT_ABI STREQUAL "single")
  string(APPEND andes_mabi "f")
elseif(TOOLCHAIN_FLOAT_ABI STREQUAL "double")
  string(APPEND andes_mabi "d")
endif()

if(NOT andes_mabi STREQUAL riscv_mabi)
  if(TOOLCHAIN_FLOAT_ABI STREQUAL "soft")
    message(WARNING "${ANDES_TOOLCHAIN_PATH} only supports the soft-float ABI, "
                    "setting 'CONFIG_FLOAT_HARD=y' will be ignored.")
  elseif(TOOLCHAIN_FLOAT_ABI STREQUAL "single")
    if(NOT CONFIG_RISCV_ISA_EXT_F)
      message(WARNING "${ANDES_TOOLCHAIN_PATH} only supports the single-float ABI, "
                      "which requires CONFIG_RISCV_ISA_EXT_F=y, "
                      "the build is expected to fail due to an incompatible ABI.")
    elseif(NOT CONFIG_FPU)
      message(WARNING "${ANDES_TOOLCHAIN_PATH} only supports the single-float ABI, "
                      "which requires CONFIG_FPU=y, "
                      "the build is expected to fail due to an incompatible ABI.")
    elseif(NOT CONFIG_FLOAT_HARD)
      message(WARNING "${ANDES_TOOLCHAIN_PATH} only supports the single-float ABI, "
                      "setting 'CONFIG_FLOAT_HARD=n' will be ignored.")
    elseif(CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION)
      message(WARNING "${ANDES_TOOLCHAIN_PATH} only supports the single-float ABI, "
                      "setting 'CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION=y' will be ignored.")
    endif()
  elseif(TOOLCHAIN_FLOAT_ABI STREQUAL "double")
    if(NOT CONFIG_RISCV_ISA_EXT_D)
      message(WARNING "${ANDES_TOOLCHAIN_PATH} only supports the double-float ABI, "
                      "which requires CONFIG_RISCV_ISA_EXT_D=y, "
                      "the build is expected to fail due to an incompatible ABI.")
    elseif(NOT CONFIG_FPU)
      message(WARNING "${ANDES_TOOLCHAIN_PATH} only supports the double-float ABI, "
                      "which requires CONFIG_FPU=y, "
                      "the build is expected to fail due to an incompatible ABI.")
    elseif(NOT CONFIG_FLOAT_HARD)
      message(WARNING "${ANDES_TOOLCHAIN_PATH} only supports the double-float ABI, "
                      "setting 'CONFIG_FLOAT_HARD=n' will be ignored.")
    endif()
  endif()
  foreach(var RISCV_C_FLAGS TOOLCHAIN_C_FLAGS)
    list(TRANSFORM ${var} REPLACE "^-mabi=${riscv_mabi}$" "-mabi=${andes_mabi}")
  endforeach()
endif()

if(CONFIG_RISCV_CUSTOM_CSR_ANDES_EXECIT)
  list(APPEND TOOLCHAIN_C_FLAGS -mexecit)
  list(APPEND TOOLCHAIN_LD_FLAGS -Wl,--mexecit)

  if(CONFIG_RISCV_CUSTOM_CSR_ANDES_NEXECIT)
    list(APPEND TOOLCHAIN_LD_FLAGS -Wl,--mnexecitop)
  endif()
else()
  # Disable execit explicitly, as -Os enables it by default.
  list(APPEND TOOLCHAIN_C_FLAGS -mno-execit)
  list(APPEND TOOLCHAIN_LD_FLAGS -Wl,--mno-execit)
endif()

if(CONFIG_RISCV_CUSTOM_CSR_ANDES_HWDSP)
  list(APPEND TOOLCHAIN_C_FLAGS -mext-dsp)
endif()

# Add check for GCC version. Query the GCC bundled with the toolchain, since
# Andes Clang does not support -dumpfullversion and shares the GCC multilibs.
execute_process(
  COMMAND ${CROSS_COMPILE}gcc -dumpfullversion
  OUTPUT_VARIABLE GCC_COMPILER_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Legacy Andes toolchains (<= 14.2.0) rely on custom '-mext-xxx' to match the
# multilib variants.
if(GCC_COMPILER_VERSION VERSION_LESS_EQUAL "14.2.0")
  if(CONFIG_RISCV_ISA_EXT_ZCMP OR CONFIG_RISCV_ISA_EXT_ZCMT)
    list(APPEND TOOLCHAIN_C_FLAGS "-mext-zc")
  endif()
endif()
