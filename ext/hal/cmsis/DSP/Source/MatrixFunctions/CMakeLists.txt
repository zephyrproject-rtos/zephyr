# Copyright (c) 2019 Stephanos Ioannidis <root@stephanos.io>
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  arm_mat_add_f32.c
  arm_mat_add_q15.c
  arm_mat_add_q31.c
  arm_mat_cmplx_mult_f32.c
  arm_mat_cmplx_mult_q15.c
  arm_mat_cmplx_mult_q31.c
  arm_mat_init_f32.c
  arm_mat_init_q15.c
  arm_mat_init_q31.c
  arm_mat_inverse_f32.c
  arm_mat_inverse_f64.c
  arm_mat_mult_f32.c
  arm_mat_mult_fast_q15.c
  arm_mat_mult_fast_q31.c
  arm_mat_mult_q15.c
  arm_mat_mult_q31.c
  arm_mat_scale_f32.c
  arm_mat_scale_q15.c
  arm_mat_scale_q31.c
  arm_mat_sub_f32.c
  arm_mat_sub_q15.c
  arm_mat_sub_q31.c
  arm_mat_trans_f32.c
  arm_mat_trans_q15.c
  arm_mat_trans_q31.c
  )
