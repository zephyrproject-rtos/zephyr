// Copyright: (C) 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

virtual report

@initialize:python
  depends on report
@
@@
identifiers = {}

def check_identifier(id, pos):
    if id in identifiers:
       msg="WARNING: Violation to rule 5.1 or 5.2 (First 31 chars shall be distinct) %s" % (id)
       coccilib.report.print_report(pos[0], msg)
       coccilib.report.print_report(identifiers[id][0], msg)
    else:
       identifiers[id] = pos

@r_idlen@
type T;
identifier I;
constant C;
position p;
@@

(
  T I@p (...);
|
  I@p (...)
|
  T I@p = C;
|
  T I@p;
)

@script:python depends on report@
id << r_idlen.I;
pos << r_idlen.p;
@@

if (len(id) > 31):
   check_identifier(id[:31], pos)
