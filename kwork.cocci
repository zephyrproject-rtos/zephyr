// Copyright (c) 2021 Nordic Semiconductor ASA
// SPDX-License-Identifer: Apache-2.0

// Replace use of legacy k_work API with new API.
//
// Invoke: spatch --sp-file kwork.cocci --very-quiet  --include-headers --macro-file $ZEPHYR_BASE/scripts/coccinelle/macros.h  --dir .

@patch_ds@
@@
 struct
-k_delayed_work
+k_work_delayable

@patch_fn_name@
@@
(
-k_work_pending
+k_work_is_pending
|
-k_delayed_work_pending
+k_work_delayable_is_pending
|
-k_delayed_work_init
+k_work_init_delayable
|
// Behavioral equivalent is reschedule
-k_delayed_work_submit
+k_work_reschedule
|
// Behavioral equivalent is reschedule
-k_delayed_work_submit_to_queue
+k_work_reschedule_for_queue
|
-k_delayed_work_cancel
+k_work_cancel_delayable
)
 (...)

@patch_fn_remaining_get@
expression E;
@@
(
-k_delayed_work_remaining_get(E)
+k_ticks_to_ms_ceil32(k_work_delayable_remaining_get(E))
|
-k_delayed_work_expires_ticks(E)
+k_work_delayable_expires_get(E)
|
-k_delayed_work_remaining_ticks(E)
+k_work_delayable_remaining_get(E)
)

@patch_fn_q_start@
@@
-k_work_q_start
+k_work_queue_start
 (...
+, NULL
 )
