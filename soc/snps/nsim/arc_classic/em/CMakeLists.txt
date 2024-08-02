# SPDX-License-Identifier: Apache-2.0

if(COMPILER STREQUAL gcc)
  # GNU compiler options
  zephyr_compile_options(-mcpu=${GCC_M_CPU})

  zephyr_compile_options(-mno-sdata)
  zephyr_compile_options_ifdef(CONFIG_CPU_ARCEM -mmpy-option=wlh1)
  zephyr_compile_options_ifdef(CONFIG_FPU -mfpu=fpuda_all)

else()
  # MWDT compiler options
  zephyr_compile_options_ifdef(CONFIG_SOC_NSIM_EM -arcv2em -core3 -Xdiv_rem=radix2
			       -Xmpy_option=mpyd -Xbitscan -Xswap -Xbarrel_shifter
			       -Xshift_assist -Xdsp2 -Xdsp_complex
			       -Xdsp_divsqrt=radix2 -Xdsp_itu -Xdsp_accshift=full
			       -Xfpus_div -Xfpu_mac -Xfpuda -Xfpus_mpy_slow
			       -Xfpus_div_slow -Xbitstream -Xtimer0 -Xtimer1)

  zephyr_compile_options_ifdef(CONFIG_SOC_NSIM_EM11D -arcv2em -core3 -Xdiv_rem=radix2
			       -Xbitscan -Xswap -Xbarrel_shifter
			       -Xshift_assist -Xfpus_div -Xfpu_mac -Xfpuda -Xfpus_mpy_slow
			       -Xfpus_div_slow -Xbitstream -Xtimer0 -Xtimer1)

  zephyr_ld_option_ifdef(CONFIG_SOC_NSIM_EM11D -Hlib=em9d_nrg_fpusp -Hdsplib)

  if(CONFIG_SOC_NSIM_EM11D)
    set_property(GLOBAL PROPERTY z_arc_dsp_options -Xxy -Xagu_large -Hfxapi -Xdsp2
              -Xdsp_accshift=full -Xdsp_divsqrt=radix2 -Xdsp_complex -Xdsp_itu
              -Xdsp_ctrl=postshift,noguard,convergent -Xmpy_option=mpyd)
  endif()
endif()
