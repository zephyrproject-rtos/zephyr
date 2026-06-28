.. zephyr:code-sample:: cpu_freq_side_channel_defense
   :name: CPU frequency scaling side-channel defense sample
   :relevant-api: subsys_cpu_freq

   Demonstrate how the CPU frequency scaling side-channel defense policy can defend against a timing
   based side-channel attack.

Overview
********

This sample demonstrates how the CPU frequency scaling side-channel defense policy can defend against
certain side-channel attacks. Albeit naiive, this sample shows how randomly modifying the
clock frequency can thwart certain side-channel attacks.

The sample implements a simple timing attack to brute-force a password. The sample purposefully
implements a non-constant time password checking function in order to create an attack surface.
Building the sample in the default configuration will demonstrate how the main program can guess the
password relatively quickly.

.. code-block:: console

   west build -b frdm_mcxn236 samples/subsys/cpu_freq/side_channel_defense \
     -- -DEXTRA_CONF_FILE=defense_prj.conf

The exploit will no longer be effective once defense mode is enabled.

As soon as the frequency scaling goes into effect, the exploit will incorrectly guess a character
(as the processing time will change suddenly) which then results in all further guesses being
incorrect.

Building and Running
********************

Build with the side-channel defense policy enabled:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/cpu_freq/side_channel_defense
   :board: frdm_mcxn236
   :goals: build
   :compact:
   :gen-args: -DEXTRA_CONF_FILE=defense_prj.conf

Flash and run:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/cpu_freq/side_channel_defense
   :board: frdm_mcxn236
   :goals: flash
   :compact:

Sample Output
=============

Default Mode (exploit easily possible)
--------------------------

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


Defense Mode (guessing collapses)
------------------------------

.. code-block:: none

   pos 0 try A -> 101 cycles
   pos 0 try B -> 100 cycles
   pos 0 try C -> 102 cycles
   ...
   Recovered so far: PxSaaZ

   (no stable ordering between candidates)
