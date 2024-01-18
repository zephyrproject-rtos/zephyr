.. zephyr:code-sample:: video-arducam_mega_full-featured
   :name: Arducam mega camera full-featured sample

   Capture the image frame and send to the host over
   the serial port.

Description
***********

This sample's function is to capture frames from Arducam mega camera and
send them to the host over the UART , Users can get arducam mega full
function's experience through the host.


Requirements
************

This sample requires a Arducam mega camera and USB-UART module.

- :ref:`rpi_pico`
- `Arducam mega camera module`_

Wiring
******

On :ref:`rpi_pico`, The Arducam mega camera module should be connected SPI0.
The PC should be connected to UART0 using a USB to UART cable.

   .. note:: Be careful during connection!

    Use separate wires to connect SPI pins with pins on the rpi_pico board.
    Wiring connection is described in the table below.

    +-----------------+----------------+
    | Arducam mega    | rpi_pico board |
    | camera connector| SPI connector  |
    +=================+================+
    |      VCC        |      VCC       |
    +-----------------+----------------+
    |      GND        |      GND       |
    +-----------------+----------------+
    |      SCK        |      P18       |
    +-----------------+----------------+
    |      MISO       |      P16       |
    +-----------------+----------------+
    |      MOSI       |      P19       |
    +-----------------+----------------+
    |      CS         |      P17       |
    +-----------------+----------------+

    For USB to UART cable, connect the rpi_pico board as below:

    +-------------+----------------+
    | USB to UART | rpi_pico board |
    | cable       | UART connector |
    +=============+================+
    |     RX      |       P0       |
    +-------------+----------------+
    |     TX      |       P1       |
    +-------------+----------------+
    |     GND     |       GND      |
    +-------------+----------------+

Building and Running
********************

For :ref:`rpi_pico`, build this sample application with the following commands:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/video/arducam_mega_full-featured
   :board: pico
   :goals: build
   :compact:

Using UF2
---------

You can flash the Raspberry Pi Pico with a UF2 file. By default, building
an app for this board will generate a `build/zephyr/zephyr.uf2` file.
If the Pico is powered on with the `BOOTSEL` button pressed, it will appear
on the host as a mass storage device. The UF2 file should be drag-and-dropped
to the device, which will flash the Pico.

Sample Usge
=============

You can refer to the `Arducam mega GUI Guide`_ website to install and use
the host to enjoy all the functions of the Arducam mega camera.

References
**********

.. _Arducam mega camera module: https://www.arducam.com/camera-for-any-microcontroller/
.. _Arducam mega GUI Guide: https://www.arducam.com/docs/arducam-mega/arducam-mega-getting-started/packs/GuiTool.html
