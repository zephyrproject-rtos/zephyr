# Copyright (c) 2019 Stephanos Ioannidis <root@stephanos.io>
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  arm_pid_init_f32.c
  arm_pid_init_q15.c
  arm_pid_init_q31.c
  arm_pid_reset_f32.c
  arm_pid_reset_q15.c
  arm_pid_reset_q31.c
  )

zephyr_library_sources_ifdef(CMSIS_DSP_TABLES_ARM_SIN_COS_F32 arm_sin_cos_f32.c)
zephyr_library_sources_ifdef(CMSIS_DSP_TABLES_ARM_SIN_COS_Q31 arm_sin_cos_q31.c)
