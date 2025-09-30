# SPDX-License-Identifier: Apache-2.0

list(TRANSFORM TOOLCHAIN_C_FLAGS REPLACE "^-march=([^ ]+)$" "-march=\\1_xandes")
list(TRANSFORM TOOLCHAIN_LD_FLAGS REPLACE "^-march=([^ ]+)$" "-march=\\1_xandes")
list(TRANSFORM LLEXT_APPEND_FLAGS REPLACE "^-march=([^ ]+)$" "-march=\\1_xandes")

foreach(grouped_flags IN LISTS TOOLCHAIN_GROUPED_LD_FLAGS)
  list(TRANSFORM ${grouped_flags}
    REPLACE "^-march=([^ ]+)$" "-march=\\1_xandes"
  )
endforeach()

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
