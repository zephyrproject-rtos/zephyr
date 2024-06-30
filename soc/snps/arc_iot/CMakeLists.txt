# SPDX-License-Identifier: Apache-2.0

if(COMPILER STREQUAL gcc)
  # GNU compiler options
  zephyr_compile_options(-mcpu=${GCC_M_CPU} -mno-sdata -mmpy-option=6)
  zephyr_compile_options_ifdef(CONFIG_FPU -mfpu=fpuda_all)
else()
  # MWDT compiler options
  zephyr_compile_options(-arcv2em -core2 -Xunaligned -Hrgf_banked_regs=32 -Hrgf_num_banks=2
			       -Xcode_density -Xdiv_rem=radix2 -Xswap -Xbitscan
			       -Xshift_assist -Xbarrel_shifter -Xtimer0 -Xtimer1 -Xstack_check -Xdmac)

  zephyr_compile_options_ifdef(CONFIG_FPU -Xfpus_div -Xfpu_mac -Xfpuda -Xfpus_mpy_slow -Xfpus_div_slow)

  zephyr_ld_options(-Hlib=iotdk)
endif()

zephyr_sources(
  soc.c
  sysconf.c
  )

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
