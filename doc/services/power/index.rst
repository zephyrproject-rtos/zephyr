.. _power_energy_model:

Power Energy Model
##################

The power energy model provides a common helper for comparing the expected
energy cost of one CPU P-state candidate. It is intended for policies that
already have a candidate timeline and need a single comparable score for that
candidate.

The model does not select CPU frequencies, does not select PM states, and does
not collect workload information. Those decisions remain with the policy that
calls the model. The model only evaluates the facts supplied by the caller.

The first implementation returns relative scores rather than calibrated energy
units. A platform can later replace the score inputs with calibrated power and
transition-energy data without changing the policy structure.

Inputs
******

Each call to :c:func:`power_energy_eval_pstate` evaluates one candidate P-state
using :c:struct:`power_energy_eval_input`.

.. list-table::
   :header-rows: 1

   * - Field
     - Meaning
   * - ``from_pstate``
     - Current P-state, used to describe the transition source.
   * - ``to_pstate``
     - Candidate target P-state being evaluated.
   * - ``idle_state``
     - PM idle state predicted for the idle window after the workload finishes.
   * - ``estimated_cycles``
     - Estimated CPU work for the decision window.
   * - ``exec_time_us``
     - Time needed to execute ``estimated_cycles`` at the candidate frequency.
   * - ``transition_time_us``
     - Time needed to switch from the current P-state to the candidate P-state.
   * - ``idle_time_us``
     - Idle window remaining before the deadline after execution and transition.
   * - ``min_frequency_hz``
     - Lowest non-zero allowed P-state frequency, used as the active-score baseline.

Score Components
****************

The model calculates three comparable score components:

.. code-block:: text

   active_score     = exec_time_us * active_power_scale(to_pstate)
   transition_score = transition_time_us * transition_power_scale
   idle_score       = idle_time_us * idle_power_scale(idle_state)
   total_score      = active_score + transition_score + idle_score

``active_power_scale`` is derived from the candidate frequency relative to the
lowest non-zero allowed P-state frequency. ``idle_power_scale`` is derived from
the predicted PM state; deeper idle states use lower relative power scales. The
transition score penalizes candidates with non-trivial switching latency.

The result is written to :c:struct:`power_energy_eval_result`. A lower
``total_energy`` value is a better score for the same decision window.

API Reference
*************

.. doxygengroup:: subsys_power_energy_model
