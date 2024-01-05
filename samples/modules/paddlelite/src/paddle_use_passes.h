/*
 * Copyright 2023 HNU-ESNL
 * Copyright 2023 YNU-SOFTWARE
 * Copyright 2023 PaddlePaddle
 * Copyright 2023 openEuler SIG-Zephyr
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once
#include "paddle_lite_factory_helper.h"  // NOLINT

USE_MIR_PASS(demo);
USE_MIR_PASS(static_kernel_pick_pass);
USE_MIR_PASS(variable_place_inference_pass);
USE_MIR_PASS(type_target_cast_pass);
USE_MIR_PASS(generate_program_pass);

USE_MIR_PASS(io_copy_kernel_pick_pass);
USE_MIR_PASS(argument_type_display_pass);
USE_MIR_PASS(runtime_context_assign_pass);
USE_MIR_PASS(graph_visualize_pass);

USE_MIR_PASS(remove_tf_redundant_ops_pass);
USE_MIR_PASS(lite_conv_bn_fuse_pass);
USE_MIR_PASS(lite_fc_fuse_pass);
USE_MIR_PASS(lite_shuffle_channel_fuse_pass);
USE_MIR_PASS(lite_transpose_softmax_transpose_fuse_pass);
USE_MIR_PASS(lite_interpolate_fuse_pass);
USE_MIR_PASS(lite_sequence_pool_concat_fuse_pass);
USE_MIR_PASS(identity_scale_eliminate_pass);
USE_MIR_PASS(identity_dropout_eliminate_pass);
USE_MIR_PASS(lite_conv_elementwise_fuse_pass);
USE_MIR_PASS(lite_conv_activation_fuse_pass);
USE_MIR_PASS(lite_var_conv_2d_activation_fuse_pass);
USE_MIR_PASS(lite_elementwise_add_activation_fuse_pass);
USE_MIR_PASS(lite_quant_dequant_fuse_pass);
USE_MIR_PASS(type_precision_cast_pass);
USE_MIR_PASS(type_layout_cast_pass);
USE_MIR_PASS(type_layout_cast_preprocess_pass);
USE_MIR_PASS(memory_optimize_pass);
USE_MIR_PASS(multi_stream_analysis_pass);
USE_MIR_PASS(elementwise_mul_constant_eliminate_pass)
USE_MIR_PASS(npu_subgraph_pass);
USE_MIR_PASS(xpu_subgraph_pass);
USE_MIR_PASS(mlu_subgraph_pass);
USE_MIR_PASS(mlu_postprocess_pass);
USE_MIR_PASS(weight_quantization_preprocess_pass);
USE_MIR_PASS(apu_subgraph_pass);
USE_MIR_PASS(quantized_op_attributes_inference_pass);
USE_MIR_PASS(control_flow_op_unused_inputs_and_outputs_eliminate_pass)
USE_MIR_PASS(__xpu__resnet_fuse_pass);
USE_MIR_PASS(__xpu__multi_encoder_fuse_pass);
USE_MIR_PASS(__xpu__embedding_with_eltwise_add_fuse_pass);
USE_MIR_PASS(__xpu__fc_fuse_pass);
