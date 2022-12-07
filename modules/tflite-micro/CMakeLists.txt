# Copyright (c) 2021 Intel Corporation
# Copyright 2022 Arm Limited and/or its affiliates <open-source-office@arm.com>
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_TENSORFLOW_LITE_MICRO)

  set(TENSORFLOW_LITE_MICRO_DIR ${ZEPHYR_CURRENT_MODULE_DIR})

  zephyr_library()

  zephyr_include_directories(
    ${TENSORFLOW_LITE_MICRO_DIR}/.
    ${TENSORFLOW_LITE_MICRO_DIR}/third_party_static/gemmlowp
    ${TENSORFLOW_LITE_MICRO_DIR}/third_party_static/flatbuffers/include
    ${TENSORFLOW_LITE_MICRO_DIR}/third_party_static/ruy
  )

  if(CONFIG_TENSORFLOW_LITE_MICRO_CMSIS_NN_KERNELS)
    set(CMSIS_NN_OPTIMIZED_KERNEL_DIR cmsis_nn)
    set(tflm_cmsis_nn_glue_path ${ZEPHYR_CMSIS_MODULE_DIR})

    zephyr_library_include_directories(${tflm_cmsis_nn_glue_path})
    zephyr_library_compile_definitions(CMSIS_NN)
  endif()

  if (CONFIG_ARM_ETHOS_U)
    set(ETHOSU_CO_PROCESSOR ethos_u)
  endif()

  zephyr_library_sources(
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/simple_memory_allocator.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/debug_log.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/micro_error_reporter.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/all_ops_resolver.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/memory_helpers.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/test_helpers.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/recording_micro_allocator.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/micro_time.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/recording_simple_memory_allocator.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/micro_string.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/micro_profiler.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/micro_utils.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/flatbuffer_utils.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/micro_graph.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/mock_micro_graph.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/micro_interpreter.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/micro_allocator.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/system_setup.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/memory_planner/linear_memory_planner.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/memory_planner/greedy_memory_planner.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/c/common.c
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/kernels/internal/quantization_util.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/kernels/kernel_util.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/core/api/flatbuffer_conversions.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/core/api/error_reporter.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/core/api/tensor_utils.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/core/api/op_resolver.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/schema/schema_utils.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/activations.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/activations_common.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/${CMSIS_NN_OPTIMIZED_KERNEL_DIR}/add.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/add_n.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/arg_min_max.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/batch_to_space_nd.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/cast.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/ceil.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/circular_buffer.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/comparisons.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/concatenation.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/${CMSIS_NN_OPTIMIZED_KERNEL_DIR}/conv.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/conv_common.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/cumsum.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/depth_to_space.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/${CMSIS_NN_OPTIMIZED_KERNEL_DIR}/depthwise_conv.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/depthwise_conv_common.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/dequantize.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/detection_postprocess.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/elementwise.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/elu.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/${ETHOSU_CO_PROCESSOR}/ethosu.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/exp.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/expand_dims.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/fill.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/floor.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/floor_div.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/floor_mod.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/${CMSIS_NN_OPTIMIZED_KERNEL_DIR}/fully_connected.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/fully_connected_common.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/gather.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/gather_nd.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/hard_swish.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/hard_swish_common.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/if.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/kernel_runner.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/kernel_util.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/l2norm.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/l2_pool_2d.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/leaky_relu.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/logical.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/logical_common.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/logistic.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/logistic_common.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/log_softmax.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/maximum_minimum.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/${CMSIS_NN_OPTIMIZED_KERNEL_DIR}/mul.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/neg.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/pack.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/pad.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/${CMSIS_NN_OPTIMIZED_KERNEL_DIR}/pooling.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/pooling_common.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/prelu.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/quantize.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/quantize_common.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/reduce.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/reshape.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/resize_bilinear.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/resize_nearest_neighbor.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/round.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/shape.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/${CMSIS_NN_OPTIMIZED_KERNEL_DIR}/softmax.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/softmax_common.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/space_to_batch_nd.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/space_to_depth.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/split.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/split_v.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/squeeze.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/strided_slice.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/sub.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/${CMSIS_NN_OPTIMIZED_KERNEL_DIR}/svdf.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/svdf_common.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/tanh.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/transpose.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/transpose_conv.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/unpack.cc
    ${TENSORFLOW_LITE_MICRO_DIR}/tensorflow/lite/micro/kernels/zeros_like.cc
  )

endif()
