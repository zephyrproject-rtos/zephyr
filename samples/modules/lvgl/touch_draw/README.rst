.. zephyr:code-sample:: lvgl-touch-draw
   :name: LVGL touch draw demo
   :relevant-api: display_interface input_interface

   Allow "drawing" on the screen with use of a touch input

Overview
********

This sample application features drawing functionality based on touch input.
Received touch input will be drawn onto the surface of the screen.
This feature can be generally helpful for testing proper functionality of the
touch interface.

When the board features a user-button (``sw0``) pushing this button will clear
the screen again.

Requirements
************

This demo requires a board or a shield with a display and touch-input device,
for example:

- :ref:`m5stack_core2`

Please make sure (:dtcompatible:`zephyr,lvgl-pointer-input`) is defined in
device tree.

Memory requirements
===================
The sample application requires an LVGL memory pool of at least 32 kBytes.

Building and Running
********************

Example building for :ref:`m5stack_core2`:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/lvgl/touch_draw
   :board: m5stack_core2
   :goals: build flash

References
**********

.. target-notes::

.. _LVGL Web Page: https://lvgl.io/
