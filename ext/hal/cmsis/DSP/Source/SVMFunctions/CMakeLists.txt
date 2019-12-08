# Copyright (c) 2019 Stephanos Ioannidis <root@stephanos.io>
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  arm_svm_linear_init_f32.c
  arm_svm_linear_predict_f32.c
  arm_svm_polynomial_init_f32.c
  arm_svm_polynomial_predict_f32.c
  arm_svm_rbf_init_f32.c
  arm_svm_rbf_predict_f32.c
  arm_svm_sigmoid_init_f32.c
  arm_svm_sigmoid_predict_f32.c
  )
