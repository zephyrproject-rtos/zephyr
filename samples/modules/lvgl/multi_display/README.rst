.. zephyr:code-sample:: lvgl-multi-display
   :name: LVGL Multi-display
   :relevant-api: display_interface

   Run two different LVGL demos on two different displays.

Overview
********

A sample showcasing LVGL multi-display support in Zephyr.

It runs Benchmark demo on the first display, and Stress demo on the second one (order as defined in displays property of "zephyr,displays" compatible node in deviceTree).

* Benchmark
      The benchmark demo tests the performance in various cases. For example rectangle, border, shadow, text, image blending, image transformation, blending modes, etc.
* Stress
      A stress test for LVGL. It contains a lot of object creation, deletion, animations, styles usage, and so on. It can be used if there is any memory corruption during heavy usage or any memory leaks.

More details on the demos can be found in `LVGL demos Readme`_.

Requirements
************

* A board with two displays, ideally with 480x272 resolution or higher.

Building and Running
********************

This sample can be built for simulated display environment as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/lvgl/multi_display
   :host-os: unix
   :board: native_sim
   :goals: run
   :compact:

Alternatively, if building from a 64-bit host machine, the previous target
board argument may also be replaced by ``native_sim/native/64``.

References
**********

.. target-notes::

.. _LVGL demos Readme: https://github.com/zephyrproject-rtos/lvgl/blob/zephyr/demos/README.md
