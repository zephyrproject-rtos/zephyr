/*
 * Copyright 2023 HNU-ESNL
 * Copyright 2023 YNU-SOFTWARE
 * Copyright 2023 PaddlePaddle
 * Copyright 2023 openEuler SIG-Zephyr
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

// some platform-independent defintion

#if defined(_WIN32)
#define UNUSED
#define __builtin_expect(EXP, C) (EXP)
#else
#define UNUSED __attribute__((unused))
#endif

#define USE_LITE_OP(op_type__)       \
  extern int touch_op_##op_type__(); \
  int LITE_OP_REGISTER_FAKE(op_type__) UNUSED = touch_op_##op_type__();

#define USE_LITE_KERNEL(op_type__, target__, precision__, layout__, alias__) \
  extern int touch_##op_type__##target__##precision__##layout__##alias__();  \
  int op_type__##target__##precision__##layout__##alias__##__use_lite_kernel \
      UNUSED = touch_##op_type__##target__##precision__##layout__##alias__();

#define USE_MIR_PASS(name__)                      \
  extern bool mir_pass_registry##name__##_fake(); \
  static bool mir_pass_usage##name__ UNUSED =     \
      mir_pass_registry##name__##_fake();

#define LITE_OP_REGISTER_FAKE(op_type__) op_type__##__registry__
