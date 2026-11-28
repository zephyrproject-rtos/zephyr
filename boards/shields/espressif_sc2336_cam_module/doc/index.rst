.. _espressif_sc2336_cam_module:

Espressif SC2336 Camera Module
##############################

Overview
********

The Espressif SC2336 camera module carries a SmartSens SC2336 2 megapixel CMOS image sensor with
a 2-lane MIPI CSI-2 output. It is shipped with the ESP32-P4 Function EV Board. Its camera adapter
board provides the 24 MHz sensor clock and a 15-pin FFC connector with the pinout popularized by
Raspberry Pi, linked to the board through a forward-direction ribbon cable, whose contacts are on
the same side at both ends.

The sensor outputs raw Bayer frames, which the image signal processor of the ESP32-P4 converts
to RGB565.

Requirements
************

The camera module is compatible with boards featuring a 15-pin FFC connector with the
:dtcompatible:`raspberrypi,csi-connector` pinout and the ``csi_interface``, ``csi_ep_in``,
``csi_capture_port`` and ``csi_i2c`` devicetree node labels, such as
:zephyr:board:`esp32p4x_function_ev_board`.

Usage
*****

Set ``SHIELD`` to ``espressif_sc2336_cam_module`` when building an application:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/video/capture
   :board: esp32p4x_function_ev_board/esp32p4/hpcore
   :shield: espressif_sc2336_cam_module
   :goals: build

References
**********

- `ESP32-P4 Function EV Board user guide <https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32p4/esp32-p4x-function-ev-board/user_guide.html>`_

- `Camera adapter board schematic <https://dl.espressif.com/dl/schematics/esp32-p4-function-ev-board-camera-subboard-schematics.pdf>`_
