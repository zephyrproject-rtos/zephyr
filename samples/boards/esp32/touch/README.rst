.. zephyr:code-sample:: esp32Touch
   :name: ESP32 Touch
   :relevant-api: esp32-touch

   Detect input using the touch peripheral of an ESP32.

Overview
********

This application sets the status of an LED in response to 'touch' events on an input pin using the 
:ref:`Touch API <esp32-touch-sensor-input_8h.html>`.

The ESP32 SoCs, with the exception of the ESP32-C series, features a peripheral making it trivial 
to detects capacitive touch events. Enabled pins are assigned unique key codes in the devicetree, 
together with a sensitivity setting. When a touch input occurs a corresponding event is generated. 
The event structure contains the assigned key code making it is possible to work out which touch 
input has generated the event.

Supported SoCs
**************

The following SoCs are supported by this sample code so far:

* ESP32
* ESP32-S2
* ESP32-S3

Note: The ESP32-C3 does not have this peripheral.

Wiring:
*******
It is suggested:
#. A short wire is connected to the ``touch1`` pin but directly touching the pin
should achieve the same result.
#. Have an LED connected via a GPIO pin (these are called "User LEDs" on many of
   Zephyr's :ref:`boards`).
#. Have the LED configured using the ``led0`` devicetree alias.

Building and Running
********************

To build and flash this sample for the :ref:`seeed xiao esp32s3`:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/esp32Touch
   :board: xiao_esp32s3
   :goals: build flash
   :compact:

Change ``xiao_esp32s3`` appropriately for other supported boards.

After flashing, touch the wire to change the LED status.
The console outputs detected event details.

.. code-block:: console
   *** Booting Zephyr OS build v3.6.0-1129-g4f3db943b9b3 ***
   Startup of board: xiao_esp32s3
   Input event: 1, value: 1, code: 11
   Input event: 1, value: 0, code: 11
   Input event: 1, value: 1, code: 11
