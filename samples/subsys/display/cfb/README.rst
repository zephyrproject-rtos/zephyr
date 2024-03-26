.. zephyr:code-sample:: compact-frame-buffer
   :name: Compact frame buffer
   :relevant-api: compactframebuffer

   Display character strings using the Compact Frame Buffer (CFB) subsystem.

Overview
********

This sample displays character strings using the Compact Frame Buffer
(CFB) subsystem.

Requirements
************

This sample requires a board that have a display,
such as the :ref:`reel_board`.

Building and Running
********************

Build this sample application with the following commands:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/display/cfb
   :board: reel_board
   :goals: build
   :compact:

See :ref:`reel_board` on how to flash the build.
