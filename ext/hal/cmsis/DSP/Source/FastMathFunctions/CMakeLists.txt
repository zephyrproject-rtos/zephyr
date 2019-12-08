# Copyright (c) 2019 Stephanos Ioannidis <root@stephanos.io>
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  arm_sqrt_q15.c
  arm_sqrt_q31.c
  arm_vexp_f32.c
  arm_vlog_f32.c
  )

zephyr_library_sources_ifdef(CONFIG_CMSIS_DSP_TABLES_ARM_COS_F32  arm_cos_f32.c)
zephyr_library_sources_ifdef(CONFIG_CMSIS_DSP_TABLES_ARM_COS_Q31  arm_cos_q31.c)
zephyr_library_sources_ifdef(CONFIG_CMSIS_DSP_TABLES_ARM_COS_Q15  arm_cos_q15.c)
zephyr_library_sources_ifdef(CONFIG_CMSIS_DSP_TABLES_ARM_SIN_F32  arm_sin_f32.c)
zephyr_library_sources_ifdef(CONFIG_CMSIS_DSP_TABLES_ARM_SIN_Q31  arm_sin_q31.c)
zephyr_library_sources_ifdef(CONFIG_CMSIS_DSP_TABLES_ARM_SIN_Q15  arm_sin_q15.c)
