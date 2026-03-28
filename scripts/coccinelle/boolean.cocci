// Check violations for MISRA C:2012 Rule 14.4
// https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_14_04.c
//
// Rule 14.4: Controlling expression shall have essentially Boolean type
//
// NOTE: This script is NOT enabled in CI (guideline_check.py) because it
// causes timeouts on large files due to expensive type resolution needed
// to check if variables in if/while conditions are boolean-typed.
// See: https://github.com/zephyrproject-rtos/zephyr/pull/34773
//
// For Rule 10.1 (bool = 0/1), use boolean_strict_init.cocci instead,
// which uses simple pattern matching and is fast enough for CI.
//
// Confidence: Moderate
// Copyright: (C) 2021 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

virtual report


@initialize:python@
@@

@rule1_base@
identifier function, v;
type T1, T2;
parameter list[n] P1;
parameter list[n1] P2;
@@
(
T1 function(P1, T2 v, P2) {...}
|
T1 function(P1, T2 *v, P2) {...}
)

@ script:python @
t << rule1_base.T2;
v << rule1_base.v;
@@

if t == "bool":
	cocci.include_match(False)

@rule1@
identifier rule1_base.v;
position p;
@@
(
while (v@p) {...}
|
if (v@p) {...}
)

@ script:python @
p << rule1.p;
@@

msg = "WARNING: Violation to rule 14.4 (Controlling expression shall have essentially Boolean type)"
coccilib.report.print_report(p[0], msg)

@rule2_base@
identifier v;
type T;
@@
T v;
...

@ script:python @
t << rule2_base.T;
v << rule2_base.v;
@@

if t == "bool":
	cocci.include_match(False)


@rule2@
position p;
identifier rule2_base.v;
@@
while (v@p) {...}

@ script:python @
p << rule2.p;
@@

msg = "WARNING: Violation to rule 14.4 (Controlling expression shall have essentially Boolean type)"
coccilib.report.print_report(p[0], msg)

@rule3@
position p;
constant c;
@@
(
while (c@p) {...}
|
while (!c@p) {...}
|
if (c@p) {...}
|
if (!c@p) {...}
)

@ script:python @
p << rule3.p;
@@

msg = "WARNING: Violation to rule 14.4 (Controlling expression shall have essentially Boolean type)"
coccilib.report.print_report(p[0], msg)
