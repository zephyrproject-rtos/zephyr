.. zephyr:code-sample:: status-led
   :name: Status LED
   :relevant-api: status_led_interface input_interface

   Report the status of a fake operation on a single LED, driven by a button.

Overview
********

This sample uses the :ref:`status LED <led_player_api>` subsystem to report the
status of a fake operation on a single LED, using a single button:

* The LED is on while the device is idle.
* A short press on the button starts the operation: the LED blinks slowly while
  waiting for it to complete. Another short press cancels it, and the LED is
  turned back on.
* The operation never completes: after 5 seconds, a fake error is raised. The
  error status has a higher priority than the waiting one, and its animation, a
  double flash, plays over the waiting animation until the error is cleared
  with a long press (at least 1 second) on the button. Short presses are ignored
  meanwhile.
* Once the error is cleared, the LED blinks slowly again, as the operation is
  still waiting. The error is not raised again: a short press cancels the
  operation.

Requirements
************

The board must define:

* a ``led0`` devicetree alias, pointing to a LED driven by an LED driver, such
  as a ``gpio-leds`` node.
* a ``sw0`` devicetree alias, pointing to a button of a ``gpio-keys`` node.

Building and Running
********************

Build and flash as follows, changing ``cc1352p7_lp`` for your board:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/led/status_led
   :board: cc1352p7_lp
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

   <inf> status_led_sample: Short press the button to start a fake operation
   <inf> status_led_sample: Waiting, short press the button to cancel
   <inf> status_led_sample: Fake error, long press the button to clear it
   <inf> status_led_sample: Error cleared, short press the button to cancel
   <inf> status_led_sample: Canceled
