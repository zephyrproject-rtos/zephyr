/// Use BIT() helper macro instead of hardcoding using bitshifting
///
// Confidence: High
// Copyright (c) 2017 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

virtual patch

@depends on patch && !(file in "ext")@
expression A;
@@

- (1 << A)
+ BIT(A)
