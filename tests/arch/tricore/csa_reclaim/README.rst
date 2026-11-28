.. _tricore_csa_reclaim:

TriCore CSA Save/Reclaim on Thread Termination
##############################################

Overview
********

TriCore saves thread context into linked Context Save Areas (CSAs) drawn from a
free list. When a thread terminates, its CSA chain must be returned to that free
list exactly once, and from the correct thread. This test stresses the two
paths ``z_tricore_reclaim_csa`` must get right:

* **ISR self-abort** (``test_isr_self_abort_churn``). A periodic ``k_timer``
  expiry runs in the system-clock ISR and aborts the thread it interrupted -- a
  genuine ISR-context self-abort. The victims continuously recurse and unwind,
  so their CSA chains are deep and constantly recycled. The live chain must be
  saved into and reclaimed from the aborting thread's own struct, not the dummy
  ``_current``; a wrong reclaim strips other threads' context bits and traps
  (CTYP / FCU) or hangs.

* **Double reclaim** (``test_double_abort_dead``). A thread is terminated by one
  of three modes (natural return, self-abort, cross-abort) and then aborted
  again while dead. The second reclaim must be a no-op; otherwise it double-frees
  CSAs and corrupts the free list. CSA-allocating churn after each abort forces
  any corruption to fault immediately.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: tests/arch/tricore/csa_reclaim
   :board: kit_a2g_tc375_lite/tc375tp/cpu0
   :goals: build flash
   :compact:

TriCore hardware only (verified on TC375, TC397 and TC4D7). Runs under
time-slicing. Run with the D2 fix (see ``d2_switch``) in place so a corrupted
return value cannot mask a CSA result.

Expected Result
***************

.. code-block:: console

   START - test_double_abort_dead
   double-abort survived 200 rounds x 3 modes
    PASS - test_double_abort_dead
   START - test_isr_self_abort_churn
   ISR self-abort churn survived: killed_isr=48
    PASS - test_isr_self_abort_churn
   PROJECT EXECUTION SUCCESSFUL
