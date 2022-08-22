# SPDX-License-Identifier: Apache-2.0

if(COMPILER STREQUAL gcc)
  # GNU compiler options
  zephyr_compile_options(-mcpu=${GCC_M_CPU})
  if(CONFIG_ISA_ARCV2)
    # ISA_ARCV2 & 32BIT
    zephyr_compile_options(-mno-sdata)
    zephyr_compile_options_ifdef(CONFIG_CPU_ARCEM -mmpy-option=wlh1)
    zephyr_compile_options_ifdef(CONFIG_CPU_ARCHS -mmpy-option=plus_qmacw)
    if(CONFIG_CPU_ARCHS)
      zephyr_compile_options_ifdef(CONFIG_FPU -mfpu=fpud_all)
    else()
      zephyr_compile_options_ifdef(CONFIG_FPU -mfpu=fpuda_all)
    endif()
  endif()
else()
  # MWDT compiler options
  zephyr_compile_options_ifdef(CONFIG_SOC_NSIM_EM -arcv2em -core3 -Xdiv_rem=radix2
			       -Xmpy_option=mpyd -Xbitscan -Xswap -Xbarrel_shifter
			       -Xshift_assist -Xdsp2 -Xdsp_complex
			       -Xdsp_divsqrt=radix2 -Xdsp_itu -Xdsp_accshift=full
			       -Xfpus_div -Xfpu_mac -Xfpuda -Xfpus_mpy_slow
			       -Xfpus_div_slow -Xbitstream -Xtimer0 -Xtimer1)

  zephyr_compile_options_ifdef(CONFIG_SOC_NSIM_SEM -arcv2em -core3 -Xcode_density
			       -Xdiv_rem=radix2 -Xswap -Xbitscan -Xmpy_option=mpyd
			       -Xshift_assist -Xbarrel_shifter -Xdsp2
			       -Xdsp_complex -Xdsp_divsqrt=radix2
			       -Xdsp_accshift=limited -Xtimer0 -Xtimer1
			       -Xsec_timer0 -Xstack_check -Xsec_modes -Xdmac)

  zephyr_compile_options_ifdef(CONFIG_SOC_NSIM_HS -arcv2hs -core2 -Xatomic
			       -Xunaligned -Xcode_density -Xswap -Xbitscan
			       -Xmpy_option=qmpyh -Xshift_assist -Xbarrel_shifter
			       -Xtimer0 -Xtimer1)

  zephyr_ld_option_ifdef(CONFIG_SOC_NSIM_HS -Hlib=hs38_full)

  zephyr_compile_options_ifdef(CONFIG_SOC_NSIM_HS_SMP -arcv2hs -core2 -Xatomic
			       -Xll64 -Xdiv_rem=radix4 -Xunaligned -Xcode_density
			       -Xswap -Xbitscan -Xmpy_option=qmpyh -Xshift_assist
			       -Xbarrel_shifter -Xfpud_div -Xfpu_mac -Xrtc
			       -Xtimer0 -Xtimer1)

  zephyr_ld_option_ifdef(CONFIG_SOC_NSIM_HS_SMP -Hlib=hs38_full)

  zephyr_compile_options_ifdef(CONFIG_SOC_NSIM_HS_MPUV6 -arcv2hs -core2 -Xatomic
			       -Xunaligned -Xcode_density -Xswap -Xbitscan
			       -Xmpy_option=qmpyh -Xshift_assist -Xbarrel_shifter
			       -Xtimer0 -Xtimer1)

  zephyr_ld_option_ifdef(CONFIG_SOC_NSIM_HS_MPUV6 -Hlib=hs38_full)

  zephyr_compile_options_ifdef(CONFIG_SOC_NSIM_HS5X -arcv3hs -core0 -Xdual_issue -uarch_rev=0:0
			       -HL -Hlpc_width=0 -Xatomic=2 -Xll64 -Xunaligned -Xdiv_rem=radix4 -Xmpy_option=qmpyh
			       -Xtimer0 -Xtimer1 -Xrtc -dcache=32768,64,2,a -Hld_cycles=1)

  zephyr_ld_option_ifdef(CONFIG_SOC_NSIM_HS5X -Hlib=hs58_full)

  zephyr_compile_options_ifdef(CONFIG_SOC_NSIM_HS5X_SMP -arcv3hs -core0 -Xdual_issue -uarch_rev=0:0
             -HL -Hlpc_width=0 -Xatomic=2 -Xll64 -Xunaligned -Xdiv_rem=radix4 -Xmpy_option=qmpyh
             -Xtimer0 -Xtimer1 -Xrtc -dcache=32768,64,2,a -Hld_cycles=1)

  zephyr_ld_option_ifdef(CONFIG_SOC_NSIM_HS5X_SMP -Hlib=hs58_full)

  zephyr_compile_options_ifdef(CONFIG_SOC_NSIM_HS6X -arc64 -core0 -uarch_rev=0:0 -HL -Xatomic=2
             -Xunaligned -Xmpy_cycles=3 -Xtimer0 -Xtimer1 -Xrtc -dcache=32768,64,2,a -Hld_cycles=1)

  zephyr_ld_option_ifdef(CONFIG_SOC_NSIM_HS6X -Hlib=hs68_full_zephyr)

  zephyr_compile_options_ifdef(CONFIG_SOC_NSIM_HS6X_SMP -arc64 -core0 -uarch_rev=0:0 -HL -Xatomic=2
             -Xunaligned -Xmpy_cycles=3 -Xtimer0 -Xtimer1 -Xrtc -dcache=32768,64,2,a -Hld_cycles=1)

  zephyr_ld_option_ifdef(CONFIG_SOC_NSIM_HS6X_SMP -Hlib=hs68_full_zephyr)
endif()

zephyr_sources_ifdef(CONFIG_SMP smp.c)
