# SPDX-License-Identifier: Apache-2.0

zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)

# -mcpu=hs38_linux includes -matomic -mcode-density -mdiv-rem
# -mswap -mnorm -mll64 -mmpy-option=9 -mfpu=fpud_all
zephyr_cc_option(-mcpu=${GCC_M_CPU})
zephyr_cc_option(-mno-sdata)
zephyr_cc_option_ifdef(CONFIG_FPU -mfpu=fpud_all)

zephyr_sources(
  soc.c
  )
