.. _led_is31fl3194_sample:

IS31FL3194 Sample Application
#############################

Overview
********

This sample application demonstrates basic usage of the IS31FL3194 LED driver.

Building and Running
********************

The sample application is located at ``samples/drivers/led_is31fl3194/``
in the Zephyr source tree.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/led_is31fl3194
   :board: <board>
   :goals: flash
   :compact:

Refer to your :ref:`board's documentation <boards>` for alternative
flash instructions if your board doesn't support the ``flash`` target.

When you connect to your board's serial console, you should see the
following output:

.. code-block:: none

   *** Booting Zephyr OS build zephyr-v3.0.0-3944-g1d81c380a6aa  ***
   [00:00:00.270,996] <inf> main: Running fade in and fade out with led Blue LED
   [00:00:03.267,852] <inf> main: Running fade in and fade out with led GREEN LED
   [00:00:06.264,678] <inf> main: Running fade in and fade out with led Red LED
