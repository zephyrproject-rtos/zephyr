.. zephyr:board:: esp32_cam

Overview
********

Ai-Thinker ESP32-CAM is an ESP32-based development board produced by `Ai-Thinker <https://en.ai-thinker.com/>`_.

Hardware
********

ESP32-CAM features the following components:

- ESP32S module
- 8MB PSRAM
- 24-pin FPC connector (for OV2640 Sensor)
- MicroSD card slot
- Flash LED

.. note::

   ESP32's GPIO4 on the ESP32 is shared between the MicroSD data pin and the onboard flash LED.

.. include:: ../../../espressif/common/soc-esp32-features.rst
   :start-after: espressif-soc-esp32-features

Supported Features
==================

.. zephyr:board-supported-hw::

System Requirements
*******************

.. include:: ../../../espressif/common/system-requirements.rst
   :start-after: espressif-system-requirements

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

.. include:: ../../../espressif/common/building-flashing.rst
   :start-after: espressif-building-flashing

.. include:: ../../../espressif/common/board-variants.rst
   :start-after: espressif-board-variants

Debugging
=========

.. include:: ../../../espressif/common/openocd-debugging.rst
   :start-after: espressif-openocd-debugging

+------------+-----------+
| ESP32 pin  | JTAG pin  |
+============+===========+
| 3V3        | VTRef     |
+------------+-----------+
| IO14       | TMS       |
+------------+-----------+
| IO12       | TDI       |
+------------+-----------+
| GND        | GND       |
+------------+-----------+
| IO13       | TCK       |
+------------+-----------+
| IO15       | TDO       |
+------------+-----------+

Sample Applications
*******************

Applications for the ``esp32_cam`` board can be built and flashed in the usual way
(see :ref:`build_an_application` and :ref:`application_run` for more details);
however, an external FTDI USB to TTL Serial Adapter is required since the board
does not have any on-board debug IC.

The following pins of the Serial Adapter must be connected to the header pins:

* VTref = VCC
* GND = GND
* TXD = U0TXD
* RXD = U0RXD
* Boot = GPIO0 (Must be low at boot)

References
**********

.. target-notes::

.. _`ESP32-CAM`: https://docs.ai-thinker.com/en/esp32-cam
