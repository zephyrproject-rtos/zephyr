.. zephyr:code-sample:: lvgl-multi-display
   :name: LVGL Multi-display
   :relevant-api: display_interface

   Run two different LVGL demos on two different displays.

Overview
********

A sample showcasing LVGL multi-display support in Zephyr.

It runs Widgets demo on the first display, and Flex Layout demo on the second one (order as defined in displays property of "zephyr,displays" compatible node in deviceTree).

* Widgets
      Shows how the widgets look like out of the box using the built-in material theme.
* Flex Layout
      Showcases the use of the flex layout.

More details on the demos can be found in `LVGL demos Readme`_.

Requirements
************

* A board with two displays, ideally with 480x272 resolution or higher.

Building and Running
********************

This sample can be built for simulated display environment on Linux as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/lvgl/multi_display
   :host-os: unix
   :board: native_sim/native/64
   :goals: run
   :compact:

References
**********

.. target-notes::

.. _LVGL demos Readme: https://github.com/zephyrproject-rtos/lvgl/blob/zephyr/demos/README.md
