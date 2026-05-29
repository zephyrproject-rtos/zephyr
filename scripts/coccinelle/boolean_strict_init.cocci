// Check violations for MISRA C:2012 Rule 10.1 (boolean essential type)
// https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_10_01.c
//
// Rule 10.1: Operands shall not be of an inappropriate essential type
//            (boolean variables shall use true/false, not 0/1)
//
// This enforces Zephyr coding guideline #59 which requires boolean
// variables to be initialized and assigned with true/false literals.
//
// Confidence: High
// Copyright: (C) 2026 Espressif Systems (Shanghai) Co., Ltd.
//
// SPDX-License-Identifier: Apache-2.0

virtual report
virtual patch

// Rule 10.1: bool initialization or assignment with integer literal 0/1
// Note: Coccinelle matches type regardless of storage class specifiers
@r depends on report@
identifier v;
position p;
@@
(
  bool v@p = 0;
|
  bool v@p = 1;
|
  bool v;
  ...
  v@p = 0;
|
  bool v;
  ...
  v@p = 1;
|
  bool v = ...;
  ...
  v@p = 0;
|
  bool v = ...;
  ...
  v@p = 1;
)

@script:python depends on report@
p << r.p;
@@
msg = "WARNING: Violation to rule 10.1 (Boolean variable initialized/assigned with integer literal, use true/false)"
coccilib.report.print_report(p[0], msg)

// Auto-fix
@patch_r depends on patch@
identifier v;
@@
(
  bool v =
  (
-   0
+   false
  |
-   1
+   true
  )
  ;
|
  bool v;
  ...
  v =
  (
-   0
+   false
  |
-   1
+   true
  )
  ;
|
  bool v = ...;
  ...
  v =
  (
-   0
+   false
  |
-   1
+   true
  )
  ;
)
