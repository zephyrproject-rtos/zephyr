.. zephyr:code-sample:: biometrics-async
   :name: Asynchronous biometrics
   :relevant-api: biometrics_interface

   Enroll biometric templates and receive asynchronous identification events.

Overview
********

This sample queries a device's asynchronous capabilities, registers an event
callback, and provides shell commands for device-managed enrollment, asynchronous
identification, and stopping. Enrollment stores a new template and prints its
assigned ID and detected modality. Unsupported operations are reported before
starting a request.

Requirements
************

A biometric device supporting asynchronous enrollment or identification, with a
devicetree alias named ``biometrics`` referring to it. The shell requires a console.

The default ``app.overlay`` configures a Hi-Link AI-10 module on ``uart1`` at
115200 baud using ``uart1_default`` pin control. Adapt the UART and pin configuration
to your board and wiring.
For another sensor, provide an overlay defining the device and the ``biometrics`` alias.
Enrollment requires support for automatic template ID allocation.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/biometrics/async
   :board: <your_board>
   :goals: build flash
   :compact:

At startup, the sample prints which operations the device supports.

* Run ``biometrics_async enroll`` and present a supported biometric sample within
  20 seconds.
* After enrollment ends, run ``biometrics_async identify`` for repeated recognition
  events. Each attempt has a 10-second timeout; matching continues between attempts.
* Run ``biometrics_async identify once`` for one attempt. After MATCH or NO_MATCH,
  STOPPED reports completion; its status is zero if cleanup succeeds.
* Run ``biometrics_async stop`` to stop active continuous identification before
  enrolling again or managing templates.

Cancellation does not delete a template that the device has already stored.

The existing ``biometrics`` shell commands provide template listing and deletion;
use the device name printed at startup and shell help for their arguments.
