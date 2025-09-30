# SPDX-License-Identifier: Apache-2.0

list(TRANSFORM TOOLCHAIN_C_FLAGS REPLACE "^-march=([^ ]+)$" "-march=\\1_xandes")
list(TRANSFORM TOOLCHAIN_LD_FLAGS REPLACE "^-march=([^ ]+)$" "-march=\\1_xandes")
list(TRANSFORM LLEXT_APPEND_FLAGS REPLACE "^-march=([^ ]+)$" "-march=\\1_xandes")

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
