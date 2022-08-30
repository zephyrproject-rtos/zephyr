.. _character_frame_buffer_sample:

Character frame buffer
######################

Overview
********

This sample displays character strings using the Character Frame Buffer
(CFB) subsystem framework.

Requirements
************

This sample requires a supported board and CFB-supporting
display, such as the :ref:`reel_board`.

Building and Running
********************

Build this sample application with the following commands:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/display/cfb
   :board: reel_board
   :goals: build
   :compact:

See :ref:`reel_board` on how to flash the build.
