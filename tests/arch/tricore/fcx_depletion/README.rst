.. _tricore_fcx_depletion:

TriCore CSA Reclaim Under Free-List Depletion
#############################################

Overview
********

This test drives the CSA free-list head pointer (``FCX``) close to the limit
pointer (``LCX``, the free-context-depletion trigger) and reclaims a deep chain
there, checking that ``z_tricore_reclaim_csa`` behaves correctly under CSA
pressure.

A holder thread recurses -- each frame forces a real ``CALL`` that consumes an
upper-context CSA -- until only ``MARGIN`` CSAs remain before the FCD limit,
then blocks, pinning a deep chain. ``main`` aborts the holder, so the reclaim
walks that deep chain and returns it to a nearly empty free list, then aborts
the dead thread again. ``MARGIN`` stays above the 8-CSA FCD reserve the
abort/reclaim call chain itself needs. A low-priority canary computes a
fixed-result checksum throughout; a reclaim that corrupts a CSA outside the
dying thread breaks the canary's resumed context.

The free list is deliberately small (``CONFIG_TRICORE_CSA_COUNT=96``).

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: tests/arch/tricore/fcx_depletion
   :board: kit_a2g_tc375_lite/tc375tp/cpu0
   :goals: build flash
   :compact:

TriCore hardware only (verified on TC375, TC397 and TC4D7).

Expected Result
***************

``min_free`` reaches the configured margin and the canary is never corrupted:

.. code-block:: console

   START - test_reclaim_near_depletion
   done min_free=20 canary_ok=91459 canary_bad=0
    PASS - test_reclaim_near_depletion
   PROJECT EXECUTION SUCCESSFUL
