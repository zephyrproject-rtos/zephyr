// Check violations for rule 5.7
// https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_05_07.c
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
@@
(
struct t *v@p;
|
struct t v@p;
|
union t v@p;
)

@ script:python @
t << common_case.t;
v << common_case.v;
p << common_case.p;
@@

msg = "WARNING: Violation to rule 5.7 (Tag name should be unique) tag: {}".format(v)
if t == v:
	coccilib.report.print_report(p[0], msg)

@per_type@
type T;
identifier v;
position p;
@@
(
T v@p;
|
T *v@p;
)

@ script:python @
t << per_type.T;
v << per_type.v;
p << per_type.p;
@@

msg = "WARNING: Violation to rule 5.7 (Tag name should be unique) tag: {}".format(v)
if t == v:
	coccilib.report.print_report(p[0], msg)

@function_match@
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
v << function_match.v;
t << function_match.T2;
p << function_match.p;
@@

msg = "WARNING: Violation to rule 5.7 (Tag name should be unique) tag: {}".format(v)
if v == t.split(" ")[-1]:
 	coccilib.report.print_report(p[0], msg)
