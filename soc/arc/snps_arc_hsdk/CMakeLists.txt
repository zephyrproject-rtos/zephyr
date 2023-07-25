# SPDX-License-Identifier: Apache-2.0

zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)

if(COMPILER STREQUAL gcc)
  # GNU compiler options
  # -mcpu=hs38_linux includes -matomic -mcode-density -mdiv-rem
  # -mswap -mnorm -mll64 -mmpy-option=9 -mfpu=fpud_all
  zephyr_cc_option(-mcpu=${GCC_M_CPU})
  zephyr_cc_option(-mno-sdata)
  zephyr_cc_option_ifdef(CONFIG_FPU -mfpu=fpud_all)
else()
  # MWDT compiler options
  zephyr_compile_options(-arcv2hs -core2 -Xatomic -Xll64 -Xunaligned -Xcode_density
		   -Xdiv_rem=radix4 -Xswap -Xbitscan -Xmpy_option=qmpyh
		   -Xshift_assist -Xbarrel_shifter -Xtimer0 -Xtimer1 -Xrtc)
  zephyr_compile_options_ifdef(CONFIG_FPU -Xfpu_mac -Xfpud_div)

  zephyr_ld_options(-Hlib=hs38_full)
endif()
