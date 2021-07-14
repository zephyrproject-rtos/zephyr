// http://cwe.mitre.org/data/definitions/467.html
//
// Confidence: Moderate
// Copyright: (C) 2021 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

virtual report


@initialize:python@
@@

@base@
type T;
T *x;
position p;
@@
sizeof@p(x)

@ script:python @
p << base.p;
@@

msg = "WARNING: passing a pointer to sizeof()"
coccilib.report.print_report(p[0], msg)
