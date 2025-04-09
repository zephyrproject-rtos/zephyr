.. zephyr:code-sample:: renesas-elc
   :name: Renesas ELC Sample

   Integrating the Renesas ELC with PWM Functionality

Overview
********

This sample application demonstrates the integration of the Renesas Event Link
Controller (ELC) with PWM functionality in a Zephyr-based system.

It initializes the PWM generator, PWM capture, and ELC devices from the device
tree. Then it configures the PWM generator, verifies its signal via capture,
uses ELC to stop the generator, verifies the stop operation via a timeout, and
finally restarts the generator using ELC to confirm that the control flow works
as expected.

Hardware Setup
**************

* **Board Requirements:**

  - At least one PWM output channel
  - At least one PWM capture channel
  - A Renesas ELC node in the device tree

* **Wiring:**

  - The PWM output pin must be connected to the PWM capture pin for the sample
    to verify the generated signal via capture.
  - Refer to your board documentation or device tree to identify the correct
    pins and ensure they are connected properly.

Building and Running
********************

To build and flash this sample for the :zephyr:board:`ek_ra8m1`:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/renesas/elc
   :board: ek_ra8m1
   :goals: build flash
   :compact:

Change ``ek_ra8m1`` appropriately for other supported boards.

Sample Output
=============

Expected console output:

.. code-block:: console

   *** Booting Zephyr OS build a187e16d5baa ***
   PWM generator device: pwm7@40322700
   PWM capture device: pwm9@40322900
   ELC device: elc@40201000

   PWM generator configured: period=1000000 ns, pulse=500000 ns
   PWM captured: period=999991 ns

   Generate ELC software event to stop PWM generator.
   PWM generator stopped by the ELC link as expected.

   Generate ELC software event to start PWM generator.
   PWM captured: period=999991 ns
   PWM generator started by the ELC link as expected.
