.. zephyr:board:: esp32_cam

Overview
********

Ai-Thinker ESP32-CAM is an ESP32-based development board produced by `Ai-Thinker <https://en.ai-thinker.com/>`_.

ESP32-CAM features the following components:

- ESP32S module
- 8MB PSRAM
- 24-pin FPC connector (for OV2640 Sensor)
- MicroSD card slot
- Flash LED

.. note::

   ESP32's GPIO4 on the ESP32 is shared between the MicroSD data pin and the onboard flash LED.

For more information, check the datasheet at `ESP32 Datasheet`_ or the technical reference
manual at `ESP32 Technical Reference Manual`_.

Supported Features
******************

.. zephyr:board-supported-hw::

System requirements
*******************

Espressif HAL requires WiFi and Bluetooth binary blobs in order work. Run the command
below to retrieve those files.

.. code-block:: console

   west blobs fetch hal_espressif

.. note::

   It is recommended running the command above after :file:`west update`.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

.. include:: ../../../espressif/common/building-flashing.rst
   :start-after: espressif-building-flashing

.. include:: ../../../espressif/common/board-variants.rst
   :start-after: espressif-board-variants

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

Debugging
=========

ESP32 support on OpenOCD is available at `OpenOCD ESP32`_.

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

Further documentation can be obtained from the SoC vendor in `JTAG debugging for ESP32`_.

Here is an example for building the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: esp32_cam/esp32/procpu
   :goals: build flash

References
**********

.. target-notes::

.. _`ESP32-CAM`: https://docs.ai-thinker.com/en/esp32-cam
.. _`ESP32 Datasheet`: https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf
.. _`ESP32 Technical Reference Manual`: https://espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf
.. _`JTAG debugging for ESP32`: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/jtag-debugging/index.html
.. _`OpenOCD ESP32`: https://github.com/espressif/openocd-esp32/releases
