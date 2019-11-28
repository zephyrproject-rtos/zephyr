// Copyright (c) 2019 Nordic Semiconductor ASA
// SPDX-License-Identifer: Apache-2.0

// Replace integer constant timeouts with K_MSEC variants
//
// Some existing code assumes that timeout parameters are provided as
// integer milliseconds, when they were intended to be timeout values
// produced by specific constants and macros.  Convert the integer
// literals to the desired equivalent.
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

// ** Convert integer delays in K_THREAD_DEFINE to the appropriate macro

// Identify declarers where an identifier is used for the delay
@r_thread_decl_id@
declarer name K_THREAD_DEFINE;
identifier C;
position p;
@@
K_THREAD_DEFINE@p(..., C);

// Select declarers with constant literal delay and replace with
// appropriate macro
@depends on patch@
declarer name K_THREAD_DEFINE;
constant C;
position p != r_thread_decl_id.p;
@@
K_THREAD_DEFINE@p(...,
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
 );

// Identify declarers where an identifier is used for the delay
@r_thread_decl_const
 depends on report@
declarer name K_THREAD_DEFINE;
constant C;
position p != r_thread_decl_id.p;
@@
K_THREAD_DEFINE@p(..., C);


@script:python
 depends on report
@
C << r_thread_decl_const.C;
p << r_thread_decl_const.p;
@@
msg = "WARNING: replace constant {} with timeout in K_THREAD_DEFINE".format(C)
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
