.. zephyr:code-sample:: lvgl-multi-display
   :name: LVGL Multi-display
   :relevant-api: display_interface

   Run different LVGL demos on multiple displays.

Overview
********

A sample showcasing LVGL multi-display support in Zephyr.

By default, it runs the Music demo on the first display, and the Widgets demo on the other ones
(order as defined in the "displays" property of "zephyr,displays" compatible node in deviceTree).
Which demos are run can be changed by modifying the value of CONFIG_LV_Z_DEMO_FIRST_DISP## and
CONFIG_LV_Z_DEMO_OTHER_DISPS## Kconfig symbols.

* Music
      The music player demo shows what kind of modern, smartphone-like user interfaces can be
      created on LVGL.
* Benchmark
      The benchmark demo tests the performance in various cases. For example rectangle, border,
      shadow, text, image blending, image transformation, blending modes, etc.
* Stress
      A stress test for LVGL. It contains a lot of object creation, deletion, animations, styles
      usage, and so on. It can be used if there is any memory corruption during heavy usage or any
      memory leaks.
* Widgets
      Shows how the widgets look like out of the box using the built-in material theme.

More details on the demos can be found in `LVGL demos Readme`_.

Requirements
************

* A board with two displays or more, ideally with 480x272 resolution or higher.

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
