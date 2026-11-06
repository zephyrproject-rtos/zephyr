.. zephyr:code-sample:: usb-uhc-lifecycle
   :name: UHC optional lifecycle hooks
   :relevant-api: uhc_interface

   Smoke-test optional UHC hooks and transfer completion on ``native_sim``.

Overview
********

Exercises default implementations of optional UHC callbacks (add endpoints,
steady-state checks, assign address fallback) and ``uhc_xfer_return()`` on the
virtual UHC. Matches the coverage intent of ``tests/drivers/usb/uhc_api``.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/uhc_lifecycle
   :board: native_sim
   :goals: build run
   :compact:
