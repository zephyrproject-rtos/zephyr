# Copyright (c) 2019 Stephanos Ioannidis <root@stephanos.io>
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  arm_entropy_f32.c
  arm_kullback_leibler_f32.c
  arm_logsumexp_dot_prod_f32.c
  arm_logsumexp_f32.c
  arm_max_f32.c
  arm_max_no_idx_f32.c
  arm_max_q15.c
  arm_max_q31.c
  arm_max_q7.c
  arm_mean_f32.c
  arm_mean_q15.c
  arm_mean_q31.c
  arm_mean_q7.c
  arm_min_f32.c
  arm_min_q15.c
  arm_min_q31.c
  arm_min_q7.c
  arm_power_f32.c
  arm_power_q15.c
  arm_power_q31.c
  arm_power_q7.c
  arm_rms_f32.c
  arm_rms_q15.c
  arm_rms_q31.c
  arm_std_f32.c
  arm_std_q15.c
  arm_std_q31.c
  arm_var_f32.c
  arm_var_q15.c
  arm_var_q31.c
  )
