.. zephyr:code-sample:: cpu_freq_timing_noise
   :name: CPU frequency scaling timing noise sample
   :relevant-api: subsys_cpu_freq

   Show how randomized P-state selection can disrupt a simple cycle-count
   password guessing experiment.

Overview
********

This sample implements a deliberately non-constant-time password check and a
naive timing experiment that ranks character guesses by cycle count. Building
the sample in the default configuration demonstrates how that experiment can
recover the password when the CPU frequency is stable.

Rebuilding with ``timing_noise_prj.conf`` enables the timing noise policy,
which periodically selects a random P-state. That frequency jitter tends to
scramble the ranking used by this particular experiment, so the guessed
string stops matching the secret. That outcome is specific to this
measurement setup. It does not mean the underlying leak is gone, nor that
an attacker is defeated in general.

Constant-time password checking would still be the correct fix. This sample
exists to illustrate how randomized P-state selection can disrupt a simple
cycle-count ranking.

.. code-block:: console

   west build -b frdm_mcxn236 samples/subsys/cpu_freq/timing_noise \
     -- -DEXTRA_CONF_FILE=timing_noise_prj.conf

Building and Running
********************

Build without the timing noise policy (stable frequency; experiment succeeds):

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/cpu_freq/timing_noise
   :board: frdm_mcxn236
   :goals: build
   :compact:

Build with the timing noise policy enabled:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/cpu_freq/timing_noise
   :board: frdm_mcxn236
   :goals: build
   :compact:
   :gen-args: -DEXTRA_CONF_FILE=timing_noise_prj.conf

Flash and run:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/cpu_freq/timing_noise
   :board: frdm_mcxn236
   :goals: flash
   :compact:

Sample Output
=============

Default mode (stable frequency; ranking recovers the secret)
------------------------------------------------------------

.. code-block:: none

   pos 0 try A -> 79 cycles
   pos 0 try B -> 83 cycles
   pos 0 try C -> 91 cycles
   ...
   Recovered so far: P

   pos 1 try A -> 87 cycles
   pos 1 try B -> 92 cycles
   ...
   Recovered so far: Pa


With timing noise (this experiment's ranking collapses)
-------------------------------------------------------

.. code-block:: none

   pos 0 try A -> 101 cycles
   pos 0 try B -> 100 cycles
   pos 0 try C -> 102 cycles
   ...
   Recovered so far: PxSaaZ

   (no stable ordering between candidates)
