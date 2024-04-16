.. zephyr:code-sample:: lvgl-demos
   :name: LVGL demos
   :relevant-api: display_interface

   Run LVGL built-in demos.

Overview
********

A sample showcasing upstream LVGL demos.

* Music
      The music player demo shows what kind of modern, smartphone-like user interfaces can be created on LVGL.
* Benchmark
      The benchmark demo tests the performance in various cases. For example rectangle, border, shadow, text, image blending, image transformation, blending modes, etc.
* Stress
      A stress test for LVGL. It contains a lot of object creation, deletion, animations, styles usage, and so on. It can be used if there is any memory corruption during heavy usage or any memory leaks.

Requirements
************

* A board with display, ideally with 480x272 resolution or higher.

Building and Running
********************

These demos can be built as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/lvgl/demos
   :host-os: unix
   :board: native_posix
   :gen-args: -DCONFIG_LV_Z_DEMO_MUSIC=y
   :goals: run
   :compact:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/lvgl/demos
   :host-os: unix
   :board: native_posix
   :gen-args: -DCONFIG_LV_Z_DEMO_BENCHMARK=y
   :goals: run
   :compact:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/lvgl/demos
   :host-os: unix
   :board: native_posix
   :gen-args: -DCONFIG_LV_Z_DEMO_STRESS=y
   :goals: run
   :compact:
