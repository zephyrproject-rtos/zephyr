# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(cmsis_dsp_filtering)

if(CONFIG_CMSIS_DSP_TEST_FILTERING_BIQUAD)
  target_sources(app PRIVATE
    src/biquad_q15.c
    src/biquad_q31.c
    src/biquad_f32.c
    src/biquad_f64.c
    )

  target_sources_ifdef(CONFIG_CMSIS_DSP_FLOAT16 app PRIVATE
    src/biquad_f16.c
    )
endif()

if(CONFIG_CMSIS_DSP_TEST_FILTERING_DECIM)
  target_sources(app PRIVATE
    src/decim_q15.c
    src/decim_q31.c
    src/decim_f32.c
    )
endif()

if(CONFIG_CMSIS_DSP_TEST_FILTERING_FIR)
  target_sources(app PRIVATE
    src/fir_q7.c
    src/fir_q15.c
    src/fir_q31.c
    src/fir_f32.c
    )

  target_sources_ifdef(CONFIG_CMSIS_DSP_FLOAT16 app PRIVATE
    src/fir_f16.c
    )
endif()

if(CONFIG_CMSIS_DSP_TEST_FILTERING_MISC)
  target_sources(app PRIVATE
    src/misc_q7.c
    src/misc_q15.c
    src/misc_q31.c
    src/misc_f32.c
    )

  target_sources_ifdef(CONFIG_CMSIS_DSP_FLOAT16 app PRIVATE
    src/misc_f16.c
    )
endif()
