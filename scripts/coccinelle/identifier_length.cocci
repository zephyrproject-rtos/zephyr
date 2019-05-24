// Copyright: (C) 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

virtual report

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
   msg="WARNING: Identifier %s length %d > 31" % (id, len(id))
   coccilib.report.print_report(pos[0], msg)
