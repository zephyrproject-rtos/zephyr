.. zephyr:code-sample:: cpu_freq_pressure
   :name: Pressure based CPU frequency scaling
   :relevant-api: subsys_cpu_freq

   Dynamically scale CPU frequency using the pressure policy.

Overview
********

This sample demonstrates the :ref:`CPU frequency subsystem's <cpu_freq>` :ref:`pressure policy
<pressure_policy>`, which adjusts CPU frequency based on thread queue pressure. When more runnable
threads are competing for CPU time, the policy selects a higher performance state (P-state); when
fewer threads are active, it scales the frequency down.

The sample creates three threads at different priorities (low, medium, and high) and cycles through
three modes to simulate varying levels of system load:

#. **Mode 1**: Only the low-priority thread is running (low pressure).
#. **Mode 2**: Low and medium-priority threads are running (moderate pressure).
#. **Mode 3**: All three threads are running (high pressure).

Debug logging is enabled so the pressure evaluation and P-state selection are visible on the
console.

Building and Running
********************

Build and run this sample as follows, changing ``native_sim`` for your board:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/cpu_freq/pressure
   :board: native_sim
   :goals: run
   :compact:

Sample Output
=============

.. code-block:: none

   [00:00:00.000,000] <inf> cpu_freq: CPU frequency subsystem initialized with interval 500 ms
   *** Booting Zephyr OS build v4.4.0-rc1-171-ga6b41c41d385 ***
   [00:00:00.000,000] <inf> cpu_freq_pressure_sample: Starting CPU Freq Pressure Policy Sample!
   [00:00:00.000,000] <inf> cpu_freq_pressure_sample: Mode 1: Low only
   [00:00:00.500,000] <dbg> cpu_freq_policy_pressure: thread_eval_cb: Evaluating thread: 0x430728 with prio: 1 status: 16
   [00:00:00.500,000] <dbg> cpu_freq_policy_pressure: thread_eval_cb: Evaluating thread: 0x430810 with prio: 5 status: 16
   [00:00:00.500,000] <dbg> cpu_freq_policy_pressure: thread_eval_cb: Evaluating thread: 0x4308f8 with prio: 10 status: 128
   [00:00:00.500,000] <dbg> cpu_freq_policy_pressure: thread_eval_cb: Evaluating thread: 0x431078 with prio: 15 status: 0
   [00:00:00.500,000] <dbg> cpu_freq_policy_pressure: thread_eval_cb: Evaluating thread: 0x431160 with prio: 0 status: 4
   [00:00:00.500,000] <dbg> cpu_freq_policy_pressure: get_normalized_sys_pressure: System pressure is: 3% (raw: 1 / max: 28)
   [00:00:00.500,000] <dbg> cpu_freq_policy_pressure: cpu_freq_policy_select_pstate: CPU0 Pressure: 3%
   [00:00:00.500,000] <dbg> cpu_freq_policy_pressure: cpu_freq_policy_select_pstate: Pressure Policy: Selected P-state 2 with load_threshold=0%
   [00:00:00.500,000] <dbg> native_sim_cpu_freq: cpu_freq_pstate_set: SoC setting performance state: 2
   [00:00:00.500,000] <dbg> native_sim_cpu_freq: cpu_freq_pstate_set: SoC setting P-state 2: Ultra-low Power Mode

     ...

   [00:00:01.510,000] <inf> cpu_freq_pressure_sample: Mode 2: Low + Medium
   [00:00:02.000,000] <dbg> cpu_freq_policy_pressure: thread_eval_cb: Evaluating thread: 0x430728 with prio: 1 status: 16
   [00:00:02.000,000] <dbg> cpu_freq_policy_pressure: thread_eval_cb: Evaluating thread: 0x430810 with prio: 5 status: 128
   [00:00:02.000,000] <dbg> cpu_freq_policy_pressure: thread_eval_cb: Evaluating thread: 0x4308f8 with prio: 10 status: 128
   [00:00:02.000,000] <dbg> cpu_freq_policy_pressure: thread_eval_cb: Evaluating thread: 0x431078 with prio: 15 status: 0
   [00:00:02.000,000] <dbg> cpu_freq_policy_pressure: thread_eval_cb: Evaluating thread: 0x431160 with prio: 0 status: 4
   [00:00:02.000,000] <dbg> cpu_freq_policy_pressure: get_normalized_sys_pressure: System pressure is: 25% (raw: 7 / max: 28)
   [00:00:02.000,000] <dbg> cpu_freq_policy_pressure: cpu_freq_policy_select_pstate: CPU0 Pressure: 25%
   [00:00:02.000,000] <dbg> cpu_freq_policy_pressure: cpu_freq_policy_select_pstate: Pressure Policy: Selected P-state 1 with load_threshold=20%
   [00:00:02.000,000] <dbg> native_sim_cpu_freq: cpu_freq_pstate_set: SoC setting performance state: 1
   [00:00:02.000,000] <dbg> native_sim_cpu_freq: cpu_freq_pstate_set: SoC setting P-state 1: Low Power Mode


      ...

   [00:00:03.020,000] <inf> cpu_freq_pressure_sample: Mode 3: Low + Medium + High
   [00:00:03.500,000] <dbg> cpu_freq_policy_pressure: thread_eval_cb: Evaluating thread: 0x430728 with prio: 1 status: 128
   [00:00:03.500,000] <dbg> cpu_freq_policy_pressure: thread_eval_cb: Evaluating thread: 0x430810 with prio: 5 status: 128
   [00:00:03.500,000] <dbg> cpu_freq_policy_pressure: thread_eval_cb: Evaluating thread: 0x4308f8 with prio: 10 status: 128
   [00:00:03.500,000] <dbg> cpu_freq_policy_pressure: thread_eval_cb: Evaluating thread: 0x431078 with prio: 15 status: 0
   [00:00:03.500,000] <dbg> cpu_freq_policy_pressure: thread_eval_cb: Evaluating thread: 0x431160 with prio: 0 status: 4
   [00:00:03.500,000] <dbg> cpu_freq_policy_pressure: get_normalized_sys_pressure: System pressure is: 60% (raw: 17 / max: 28)
   [00:00:03.500,000] <dbg> cpu_freq_policy_pressure: cpu_freq_policy_select_pstate: CPU0 Pressure: 60%
   [00:00:03.500,000] <dbg> cpu_freq_policy_pressure: cpu_freq_policy_select_pstate: Pressure Policy: Selected P-state 0 with load_threshold=50%
   [00:00:03.500,000] <dbg> native_sim_cpu_freq: cpu_freq_pstate_set: SoC setting performance state: 0
   [00:00:03.500,000] <dbg> native_sim_cpu_freq: cpu_freq_pstate_set: SoC setting P-state 0: Nominal Mode

      ...
