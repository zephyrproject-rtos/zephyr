.. zephyr:code-sample:: lvgl-screen-transparency
   :name: LVGL screen transparency
   :relevant-api: display_interface

   Rendering to screens with transparency support using LVGL.

Overview
********

A sample application that demonstrates how to use LVGL to render to
screens that support transparency, like OSD overlays.

Requirements
************

* A board with a display that supports ``ARGB8888`` color.

.. _lvgl_screen_transparency_building_and_running:

Building and Running
********************

The demo can be built as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/lvgl/screen_transparency
   :host-os: unix
   :board: native_sim
   :goals: run
   :compact:
