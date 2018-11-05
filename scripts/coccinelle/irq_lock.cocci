/// Use unsigned int as the return value for irq_lock()
///
// Confidence: High
// Copyright (c) 2017 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

virtual patch

@find depends on !(file in "ext")@
type T;
identifier i;
typedef uint32_t,u32_t;
@@

(
 uint32_t i = irq_lock();
|
 unsigned int i = irq_lock();
|
 u32_t i = irq_lock();
|
- T
+ unsigned int
  i = irq_lock();
)

@find2 depends on !(file in "ext") exists@
type T;
identifier i;
@@

(
 uint32_t i;
|
 unsigned int i;
|
 u32_t i;
|
- T
+ unsigned int
  i;
  ...
  i = irq_lock();
)
