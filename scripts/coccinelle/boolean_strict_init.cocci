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

// Rule 10.1: Boolean initialization with integer literal
// Detect: bool var = 0; or bool var = 1;
// Note: Coccinelle matches type regardless of storage class specifiers
@rule1_init@
identifier v;
position p;
@@
(
bool v@p = 0;
|
bool v@p = 1;
)

@ script:python @
p << rule1_init.p;
@@

msg = "WARNING: Violation to rule 10.1 (Boolean variable initialized with integer literal, use true/false)"
coccilib.report.print_report(p[0], msg)

// Rule 10.1: Boolean assignment with integer literal
// Detect: bool_var = 0; or bool_var = 1;
@rule2_assign_base@
identifier v;
@@
(
bool v;
|
bool v = ...;
)

@rule2_assign@
identifier rule2_assign_base.v;
position p;
@@
(
v@p = 0;
|
v@p = 1;
)

@ script:python @
p << rule2_assign.p;
@@

msg = "WARNING: Violation to rule 10.1 (Boolean variable assigned integer literal, use true/false)"
coccilib.report.print_report(p[0], msg)
