// Check violations for rule 5.7
// https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_21_02.c
//
// Confidence: Moderate
// Copyright: (C) 2020 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

virtual report

@initialize:python@
@@

@common_case@
position p;
identifier t, v;
expression E;
type T;
@@
(
struct t *v@p;
|
struct t v@p;
|
union t v@p;
|
T v@p;
|
T *v@p;
|
struct t *v@p = E;
|
struct t v@p = E;
|
union t v@p = E;
|
T v@p = E;
|
T *v@p = E;
)

@ script:python @
v << common_case.v;
p << common_case.p;
@@

msg = "WARNING: Violation to rule 21.2 (Should not used a reserved identifier) - {}".format(v)
with open("scripts/coccinelle/symbols.txt", "r") as fp:
       symbols = fp.read().splitlines()
       if v in symbols:
               coccilib.report.print_report(p[0], msg)

@function_match@
type T;
identifier f;
position p;
@@
T f@p(...) {
...
}

@ script:python @
v << function_match.f;
@@

msg = "WARNING: Violation to rule 21.2 (Should not used a reserved identifier) - {}".format(v)
with open("scripts/coccinelle/symbols.txt", "r") as fp:
       symbols = fp.read().splitlines()
       if v in symbols:
               coccilib.report.print_report(p[0], msg)

@function_parameter@
type T1, T2;
identifier function, v;
position p;
parameter list[n] P1;
parameter list[n1] P2;
@@
T1 function(P1, T2 *v@p, P2) {
...
}

@ script:python @
v << function_parameter.v;
p << function_parameter.p;
@@

msg = "WARNING: Violation to rule 21.2 (Should not used a reserved identifier) - {}".format(v)
with open("scripts/coccinelle/symbols.txt", "r") as fp:
       symbols = fp.read().splitlines()
       if v in symbols:
               coccilib.report.print_report(p[0], msg)
