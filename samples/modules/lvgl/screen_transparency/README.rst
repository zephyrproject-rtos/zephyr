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

The area behind the transparent screen depends on the display. The SDL
(``native_sim``) display composites the framebuffer over its own transparency
checkerboard, so transparency is visible out of the box. Other displays have no
such background, so the sample draws an opaque black/white checkerboard on the
bottom layer itself; this is controlled by
:kconfig:option:`CONFIG_APP_DRAW_BACKGROUND_CHECKERBOARD`, which defaults to
enabled on every display except SDL. Disable it on boards where a separate
hardware layer (e.g. a video plane) provides the background — the
on-screen-display (OSD) use case.
