.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
.. SPDX-License-Identifier: Apache-2.0

.. zephyr:code-sample:: ti_vtm_shutdown
   :name: TI VTM thermal shutdown
   :relevant-api: sensor_interface

   Reboot the SoC from the VTM critical temperature trigger.

Overview
********

The sample programs the GT_TH2 threshold of the VTM sensor labelled
``sensor0`` to ``CONFIG_APP_CRITICAL_MC`` and registers a trigger handler. The
handler disarms the trigger and wakes a thread pinned to CPU 0 for a controlled
cold reboot. The threshold has to stay below the hardware reset threshold
(``ti,max-reset-mc``), which resets the SoC without software involvement.

The VTM alert is a level: it stays asserted while the die temperature is above
the threshold, so the handler must disarm the trigger.

Requirements
************

A TI VTM sensor child node labelled ``sensor0``. The AM62L EVM devicetree
provides it, with exactly one sensor node.

Building and Running
********************

The default image logs the temperature and reboots the board when the critical
threshold is crossed:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/ti_vtm_shutdown
   :board: am62l_evm/am62l3/a53
   :goals: build flash
   :compact:

Test mode
*********

Set ``CONFIG_APP_TEST_BELOW_MC`` to a non-zero value to program the threshold
that many millidegrees below the current die temperature. The trigger then
fires on a cool board. The sample prints ``VTM-TRIP: PASS`` or
``VTM-TRIP: FAIL``.

``CONFIG_APP_ACTION_LOG=y`` replaces the reboot with a log line.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/ti_vtm_shutdown
   :board: am62l_evm/am62l3/a53
   :goals: build flash
   :gen-args: -DCONFIG_APP_TEST_BELOW_MC=10000 -DCONFIG_APP_ACTION_LOG=y
   :compact:

Sample Output
=============

.. code-block:: console

    VTM-TRIP: die temperature 39.450 C
    VTM-TRIP: critical threshold programmed 29.450 C, reads back 29.440 C
    VTM-SHUTDOWN: critical temperature, T=39.916 C
    VTM-TRIP: PASS trigger fired after 0 ms
    VTM-SHUTDOWN: action is log only
