// Copyright (c) 2019 Nordic Semiconductor ASA
// SPDX-License-Identifer: Apache-2.0

// Replace integer constant timeouts with K_MSEC variants
//
// Some existing code assumes that timeout parameters are provided as
// integer milliseconds, when they were intended to be timeout values
// produced by specific constants and macros.  Convert the integer
// literals to the desired equivalent.

// Handle k_timer_start delay parameters
@delay_id@
expression T, P;
position p;
identifier D;
@@
k_timer_start@p(T, D, P)

@@
expression T, P;
position p != delay_id.p;
@@
k_timer_start@p(T,
- 0
+ K_NO_WAIT
 , P)

@@
expression T, P;
position p != delay_id.p;
@@
k_timer_start@p(T,
- -1
+ K_FOREVER
 , P)

@@
expression T, P;
constant int D;
position p != delay_id.p;
@@
k_timer_start@p(T,
- D
+ K_MSEC(D)
 , P)

// Handle timeouts at the end of the argument list
@end_id@
identifier f =~ "^k_(timer_start|queue_get|futex_wait|stack_pop|delayed_work_submit_to_queue|mutex_lock|sem_take|(msgq|mbox|pipe)_(block_)?(put|get)|mem_(slab|pool)_alloc|poll|thread_deadline_set)$";
position p;
identifier D;
@@
f@p(..., D)

@@
identifier f =~ "^k_(timer_start|queue_get|futex_wait|stack_pop|delayed_work_submit_to_queue|mutex_lock|sem_take|(msgq|mbox|pipe)_(block_)?(put|get)|mem_(slab|pool)_alloc|poll|thread_deadline_set)$";
position p != end_id.p;
@@
f@p(...,
- 0
+ K_NO_WAIT
 )

@@
identifier f =~ "^k_(timer_start|queue_get|futex_wait|stack_pop|delayed_work_submit_to_queue|mutex_lock|sem_take|(msgq|mbox|pipe)_(block_)?(put|get)|mem_(slab|pool)_alloc|poll|thread_deadline_set)$";
position p != end_id.p;
@@
f@p(...,
- -1
+ K_FOREVER
 )

@@
identifier f =~ "^k_(timer_start|queue_get|futex_wait|stack_pop|delayed_work_submit_to_queue|mutex_lock|sem_take|(msgq|mbox|pipe)_(block_)?(put|get)|mem_(slab|pool)_alloc|poll|thread_deadline_set)$";
position p != end_id.p;
constant int D;
@@
f@p(...,
- D
+ K_MSEC(D)
 )
