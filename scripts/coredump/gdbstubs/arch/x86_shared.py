#!/usr/bin/env python3
#
# Copyright Meta Platforms, Inc. and its affiliates.
#
# SPDX-License-Identifier: Apache-2.0


class ExceptionVectors:
    # Matches arch/x86/include/kernel_arch_data.h
    IV_DIVIDE_ERROR = 0
    IV_DEBUG = 1
    IV_NON_MASKABLE_INTERRUPT = 2
    IV_BREAKPOINT = 3
    IV_OVERFLOW = 4
    IV_BOUND_RANGE = 5
    IV_INVALID_OPCODE = 6
    IV_DEVICE_NOT_AVAILABLE = 7
    IV_DOUBLE_FAULT = 8
    IV_COPROC_SEGMENT_OVERRUN = 9
    IV_INVALID_TSS = 10
    IV_SEGMENT_NOT_PRESENT = 11
    IV_STACK_FAULT = 12
    IV_GENERAL_PROTECTION = 13
    IV_PAGE_FAULT = 14
    IV_RESERVED = 15
    IV_X87_FPU_FP_ERROR = 16
    IV_ALIGNMENT_CHECK = 17
    IV_MACHINE_CHECK = 18
    IV_SIMD_FP = 19
    IV_VIRT_EXCEPTION = 20
    IV_SECURITY_EXCEPTION = 30
