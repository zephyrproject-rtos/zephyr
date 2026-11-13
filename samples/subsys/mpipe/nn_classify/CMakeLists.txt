# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nn_classify_pipeline)

file(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

if(CONFIG_MPIPE_NN_BACKEND_TFLM)
  # The model: stock person detection from the tflite-micro module.
  # Override with -DNN_MODEL=<path>.
  if(NOT DEFINED NN_MODEL)
    set(NN_MODEL
      ${ZEPHYR_TFLITE_MICRO_MODULE_DIR}/tensorflow/lite/micro/models/person_detect.tflite)
  endif()

  generate_inc_file_for_target(app ${NN_MODEL}
    ${ZEPHYR_BINARY_DIR}/include/generated/nn_model.inc)
endif()
