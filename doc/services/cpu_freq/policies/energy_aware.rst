.. _energy_aware_policy:

Energy-aware CPU Frequency Scaling Policy
#########################################

The energy-aware CPU frequency scaling policy selects a CPU P-state by comparing
candidate P-states against an estimated workload, a deadline, the P-state
transition cost, and the predicted idle opportunity after the workload finishes.

Unlike reactive policies that select a P-state from an observed load percentage,
this policy builds a small timeline for each candidate P-state:

.. code-block:: text

   finish_time_us = exec_time_us + transition_time_us
   idle_time_us   = deadline_us - finish_time_us

Candidates that cannot finish before the deadline are rejected. Legal candidates
are scored with the :ref:`power_energy_model`, and the policy selects the legal
candidate with the lowest total score. If no candidate can meet the deadline,
the policy falls back to the P-state with the fastest finish time.

Configuration
*************

Enable the policy with :kconfig:option:`CONFIG_CPU_FREQ_POLICY_ENERGY_AWARE`.
The policy also selects the CPU workload estimator and the power energy model.

The fallback decision window is controlled by
:kconfig:option:`CONFIG_CPU_FREQ_POLICY_ENERGY_AWARE_DEADLINE_US`. If this value
is zero, the policy uses :kconfig:option:`CONFIG_CPU_FREQ_INTERVAL_MS` as the
fallback window. When PM has a known earlier event, the policy uses that event as
the tighter deadline.

Decision Flow
*************

For each candidate P-state, the policy performs these steps:

#. Convert the estimated workload to execution time using the candidate frequency.
#. Add the P-state transition latency to calculate the candidate finish time.
#. Reject the candidate if the finish time is greater than the deadline.
#. Calculate the remaining idle window.
#. Ask PM which idle state is expected for that idle window by calling
   :c:func:`pm_policy_state_for_residency`.
#. Call :c:func:`power_energy_eval_pstate` to calculate the candidate score.
#. Select the legal candidate with the lowest score, using the lower-frequency
   candidate as a tie breaker.

The policy uses PM only for prediction. PM still owns the final idle-state
decision when the system actually enters idle.

P-state Data
************

The energy-aware policy requires P-state frequency and transition-latency data.
The generic :c:struct:`pstate` contains ``frequency_hz`` and
``transition_latency_us`` fields populated from devicetree performance-state
nodes. SoC-specific P-state bindings can still extend the generic P-state data
with driver-specific configuration.

Relationship to the Energy Model
********************************

The energy-aware policy is the decision maker. It gathers the workload,
deadline, P-state, and PM prediction inputs, builds the candidate timeline, and
compares scores.

The energy model is the calculator. It evaluates one prepared candidate and
returns comparable active, transition, idle, and total scores. This keeps the
policy flow separate from the scoring formula and allows future platforms to
replace relative scores with calibrated power data.
