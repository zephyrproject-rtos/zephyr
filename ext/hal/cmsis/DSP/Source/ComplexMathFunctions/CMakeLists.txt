# Copyright (c) 2019 Stephanos Ioannidis <root@stephanos.io>
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  arm_cmplx_conj_f32.c
  arm_cmplx_conj_q15.c
  arm_cmplx_conj_q31.c
  arm_cmplx_dot_prod_f32.c
  arm_cmplx_dot_prod_q15.c
  arm_cmplx_dot_prod_q31.c
  arm_cmplx_mag_f32.c
  arm_cmplx_mag_squared_f32.c
  arm_cmplx_mag_squared_q15.c
  arm_cmplx_mag_squared_q31.c
  arm_cmplx_mult_cmplx_f32.c
  arm_cmplx_mult_cmplx_q15.c
  arm_cmplx_mult_cmplx_q31.c
  arm_cmplx_mult_real_f32.c
  arm_cmplx_mult_real_q15.c
  arm_cmplx_mult_real_q31.c
  )

zephyr_library_sources_ifdef(CONFIG_CMSIS_DSP_TABLES_ARM_CMPLX_MAG_Q31  arm_cmplx_mag_q31.c)
zephyr_library_sources_ifdef(CONFIG_CMSIS_DSP_TABLES_ARM_CMPLX_MAG_Q15  arm_cmplx_mag_q15.c)
