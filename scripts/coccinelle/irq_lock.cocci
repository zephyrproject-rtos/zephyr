// Copyright (c) 2017 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

@find@
type T;
position p;
identifier i;
@@

T@p i = irq_lock();

@script:python raise_error@
t << find.T;
@@
if t in ["uint32_t", "unsigned int", "u32_t"]:
   cocci.include_match(False)

@replacement@
type find.T;
position find.p;
@@
- T@p
+ unsigned int

@find2@
type T;
position p;
identifier i;
@@

T@p i;
...
i = irq_lock();

@script:python raise_error2@
t << find2.T;
@@
if t in ["uint32_t", "unsigned int", "u32_t"]:
   cocci.include_match(False)

@replacement2@
type find2.T;
identifier find2.i;
@@
-  T i;
+  unsigned int i;
