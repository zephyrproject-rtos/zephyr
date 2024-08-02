/// Cast void to memset to ignore its return value
///
//# The return of memset and memcpy is never checked and therefore
//# cast it to void to explicitly ignore while adhering to MISRA-C.
//
// Confidence: High
// Copyright (c) 2017 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

virtual patch

@depends on patch && !(file in "ext")@
expression e1,e2,e3;
@@
(
+ (void)
memset(e1,e2,e3);
|
+ (void)
memcpy(e1,e2,e3);
)
