// Copyright (c) 2019-2020 Nordic Semiconductor ASA
// SPDX-License-Identifer: Apache-2.0

// Convert legacy integer timeouts to timeout API
//
// Some existing code assumes that timeout parameters are provided as
// integer milliseconds, when they were intended to be timeout values
// produced by specific constants and macros.  Convert integer
// literals and parameters to the desired equivalent
//
// A few expressions that are clearly integer values are also
// converted.
//
// Options: --include-headers

virtual patch
virtual report

// ** Handle timeouts at the last position of kernel API arguments

// Base rule provides the complex identifier regular expression
@r_last_timeout@
identifier last_timeout =~ "(?x)^k_
( delayed_work_submit(|_to_queue)
| futex_wait
| mbox_data_block_get
| (mbox|msgq)_get
| mem_(pool|slab)_alloc
| mutex_lock
| pipe_(get|put)
| poll
| queue_get
| sem_take
| sleep
| stack_pop
| thread_create
| timer_start
| work_poll_submit(|_to_queue)
)$";
@@
last_timeout(...)

// Identify call sites where an identifier is used for the timeout
@r_last_timeout_id
 extends r_last_timeout
@
identifier D;
position p;
@@
last_timeout@p(..., D)

// Select call sites where a constant literal (not identifier) is used
// for the timeout and replace the constant with the appropriate macro

@r_last_timeout_const_patch
 extends r_last_timeout
 depends on patch
@
constant C;
position p != r_last_timeout_id.p;
@@
last_timeout@p(...,
(
- 0
+ K_NO_WAIT
|
- -1
+ K_FOREVER
|
- C
+ K_MSEC(C)
)
 )

@r_last_timeout_const_report
 extends r_last_timeout
 depends on report
@
constant C;
position p != r_last_timeout_id.p;
@@
last_timeout@p(..., C)

@script:python
 depends on report
@
fn << r_last_timeout.last_timeout;
p << r_last_timeout_const_report.p;
C << r_last_timeout_const_report.C;
@@
msg = "WARNING: replace constant {} with timeout in {}".format(C, fn)
coccilib.report.print_report(p[0], msg);

// ** Handle call sites where a timeout is specified by an expression
// ** scaled by MSEC_PER_SEC and replace with the corresponding
// ** K_SECONDS() expression.

@r_last_timeout_scaled_patch
 extends r_last_timeout
 depends on patch
@
// identifier K_MSEC =~ "^K_MSEC$";
symbol K_MSEC;
identifier MSEC_PER_SEC =~ "^MSEC_PER_SEC$";
expression V;
position p;
@@
last_timeout@p(...,
(
- MSEC_PER_SEC
+ K_SECONDS(1)
|
- V * MSEC_PER_SEC
+ K_SECONDS(V)
|
- K_MSEC(MSEC_PER_SEC)
+ K_SECONDS(1)
|
- K_MSEC(V * MSEC_PER_SEC)
+ K_SECONDS(V)
)
 )

@r_last_timeout_scaled_report_req
 extends r_last_timeout
 depends on report
@
identifier MSEC_PER_SEC =~ "^MSEC_PER_SEC$";
expression V;
position p;
@@
last_timeout@p(...,
(
  MSEC_PER_SEC
| V * MSEC_PER_SEC
)
 )

@r_last_timeout_scaled_report_opt
 extends r_last_timeout
 depends on report
@
identifier MSEC_PER_SEC =~ "^MSEC_PER_SEC$";
expression V;
position p;
@@
last_timeout@p(...,
(
  K_MSEC(MSEC_PER_SEC)
| K_MSEC(V * MSEC_PER_SEC)
)
 )

@script:python
 depends on report
@
fn << r_last_timeout.last_timeout;
p << r_last_timeout_scaled_report_req.p;
@@
msg = "WARNING: use K_SECONDS() for timeout in {}".format(fn)
coccilib.report.print_report(p[0], msg);

@script:python
 depends on report
@
fn << r_last_timeout.last_timeout;
p << r_last_timeout_scaled_report_opt.p;
@@
msg = "NOTE: use K_SECONDS() for timeout in {}".format(fn)
coccilib.report.print_report(p[0], msg);

// ** Handle call sites where an integer parameter is used in a
// ** position that requires a timeout value.

@r_last_timeout_int_param_patch
 extends r_last_timeout
 depends on patch
 @
identifier FN;
identifier P;
typedef int32_t, uint32_t;
@@
 FN(...,
(int
|int32_t
|uint32_t
)
 P, ...) {
 ...
 last_timeout(...,
-P
+K_MSEC(P)
 )
 ...
 }

@r_last_timeout_int_param_report
 extends r_last_timeout
 depends on report
 @
identifier FN;
identifier P;
position p;
typedef int32_t, uint32_t;
@@
 FN(...,
(int
|int32_t
|uint32_t
)
 P, ...) {
 ...
 last_timeout@p(..., P)
 ...
 }

@script:python
 depends on report
@
param << r_last_timeout_int_param_report.P;
fn << r_last_timeout.last_timeout;
p << r_last_timeout_int_param_report.p;
@@
msg = "WARNING: replace integer parameter {} with timeout in {}".format(param, fn)
coccilib.report.print_report(p[0], msg);

// ** Convert timeout-valued delays in K_THREAD_DEFINE with durations
// ** in milliseconds

// Select declarers where the startup delay is a timeout expression
// and replace with the corresponding millisecond duration.
@r_thread_decl_patch
 depends on patch@
declarer name K_THREAD_DEFINE;
identifier K_NO_WAIT =~ "^K_NO_WAIT$";
identifier K_FOREVER =~ "^K_FOREVER$";
expression E;
position p;
@@
K_THREAD_DEFINE@p(...,
(
- K_NO_WAIT
+ 0
|
- K_FOREVER
+ -1
|
- K_MSEC(E)
+ E
)
 );

//
@r_thread_decl_report
 depends on report@
declarer name K_THREAD_DEFINE;
identifier K_NO_WAIT =~ "^K_NO_WAIT$";
identifier K_FOREVER =~ "^K_FOREVER$";
expression V;
position p;
@@
K_THREAD_DEFINE@p(...,
(
 K_NO_WAIT
|
 K_FOREVER
|
 K_MSEC(V)
)
 );


@script:python
 depends on report
@
p << r_thread_decl_report.p;
@@
msg = "WARNING: replace timeout-valued delay with millisecond duration".format()
coccilib.report.print_report(p[0], msg);

// ** Handle k_timer_start where the second (not last) argument is a
// ** constant literal.

// Select call sites where an identifier is used for the duration timeout
@r_timer_duration@
expression T;
identifier D;
expression I;
position p;
@@
k_timer_start@p(T, D, I)

// Select call sites where a constant literal (not identifier) is used
// for the timeout and replace the constant with the appropriate macro
@depends on patch@
expression T;
constant C;
expression I;
position p != r_timer_duration.p;
@@
k_timer_start@p(T,
(
- 0
+ K_NO_WAIT
|
- -1
+ K_FOREVER
|
- C
+ K_MSEC(C)
)
, I)

@r_timer_duration_report
 depends on report
@
expression T;
constant C;
expression I;
position p != r_timer_duration.p;
@@
k_timer_start@p(T, C, I)

@script:python
 depends on report
@
p << r_timer_duration_report.p;
C << r_timer_duration_report.C;
@@
msg = "WARNING: replace constant {} with duration timeout in k_timer_start".format(C)
coccilib.report.print_report(p[0], msg);
