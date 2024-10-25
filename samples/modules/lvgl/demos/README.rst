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
* Widgets
      Shows how the widgets look like out of the box using the built-in material theme.
* Flex Layout
      Showcases the use of the flex layout.
* Keypad and Encoder
      Shows how to control widget with a keypad and hardware encoder.
* Render
      Collection of multiple rendering tests.
* Scroll
      Shows the scroll behaviour of a panel with a large list.
* Multilang
      Shows a UI with multilanguage options, supporting unicode characters.

More details can be found in `LVGL demos Readme`_.

Requirements
************

* A board with display, ideally with 480x272 resolution or higher.
* A pointer input device: touchpad, mouse, or touch screen capable display, compatible with :dtcompatible:`zephyr,lvgl-pointer-input`.

Note that other input devices types are not demonstrated in these demos, namely keyboards, keypads (:dtcompatible:`zephyr,lvgl-keypad-input`), rotary encoders (:dtcompatible:`zephyr,lvgl-encoder-input`) and hardware buttons (:dtcompatible:`zephyr,lvgl-button-input`).

Building and Running
********************

Example building for :zephyr:board:`mimxrt1060_evk`:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/lvgl/demos
   :board: mimxrt1060_evk
   :goals: build flash

These demos can be built for simulated display environment as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/lvgl/demos
   :host-os: unix
   :board: native_sim
   :gen-args: -DCONFIG_LV_Z_DEMO_MUSIC=y
   :goals: run
   :compact:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/lvgl/demos
   :host-os: unix
   :board: native_sim
   :gen-args: -DCONFIG_LV_Z_DEMO_BENCHMARK=y
   :goals: run
   :compact:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/lvgl/demos
   :host-os: unix
   :board: native_sim
   :gen-args: -DCONFIG_LV_Z_DEMO_STRESS=y
   :goals: run
   :compact:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/lvgl/demos
   :host-os: unix
   :board: native_sim
   :gen-args: -DCONFIG_LV_Z_DEMO_WIDGETS=y
   :goals: run
   :compact:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/lvgl/demos
   :host-os: unix
   :board: native_sim
   :gen-args: -DCONFIG_LV_Z_DEMO_FLEX_LAYOUT=y
   :goals: run
   :compact:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/lvgl/demos
   :host-os: unix
   :board: native_sim
   :gen-args: -DCONFIG_LV_Z_DEMO_KEYPAD_AND_ENCODER=y
   :goals: run
   :compact:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/lvgl/demos
   :host-os: unix
   :board: native_sim
   :gen-args: -DCONFIG_LV_Z_DEMO_RENDER=y
   :goals: run
   :compact:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/lvgl/demos
   :host-os: unix
   :board: native_sim
   :gen-args: -DCONFIG_LV_Z_DEMO_SCROLL=y
   :goals: run
   :compact:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/lvgl/demos
   :host-os: unix
   :board: native_sim
   :gen-args: -DCONFIG_LV_Z_DEMO_MULTILANG=y
   :goals: run
   :compact:

Alternatively, if building from a 64-bit host machine, the previous target
board argument may also be replaced by ``native_sim/native/64``.

References
**********

.. target-notes::

.. _LVGL demos Readme: https://github.com/zephyrproject-rtos/lvgl/blob/zephyr/demos/README.md
