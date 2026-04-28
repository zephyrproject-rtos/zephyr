.. zephyr:code-sample:: mtch9010
   :name: MTCH9010 Liquid Leak Detector Testbench
   :relevant-api: sensor_interface

   Configure an MTCH9010 and receive data from it periodically.

Description
***********

This sample application configures the MTCH9010 in either
capacitive or conductive sensing mode, triggers a sample
once per second, and reports the data received from the
sensor node.

Requirements
************

This sample application uses the `MTCH9010 Evaluation board (EV24U22A)`_
wired to the EXT2 header. A valid UART bus and :dtcompatible:`wake-gpios` are required
in this device configuration. :dtcompatible:`heartbeat-gpios` and
and :dtcompatible:`output-gpios`, if not specified, will cause the warnings to be
emitted and/or the relevant values being invalid, however the example
will function.

.. _MTCH9010 Evaluation board (EV24U22A): https://www.microchip.com/en-us/development-tool/EV24U22A

MTCH9010 Evaluation Board Setup
*******************************

**Important: The MTCH9010 can only be configured once per
Power-on-Reset (POR). Press the RESET button on this board
before resetting the MCU.**

The MTCH9010 Evaluation Board has DIP switches to allow for multiple different
evaluation conditions. For this example, place the switches and jumpers in the
following configuration for the SAM L21:

.. list-table:: DIP Switches and Jumper Positions
   :widths: 1 1
   :header-rows: 1

   * - DIP Switch or Jumper
     - Position

   * - Power Supply
     - 3.3V

   * - OUTPUT
     - N/A

   * - LOCK
     - OFF

   * - ECFG
     - ON

   * - MODE
     - N/A

   * - USB Bridge
     - OFF

   * - Sleep Period
     - N/A

For more information about the function of each switch,
please consult the `User Manual`_.

.. _User Manual: https://onlinedocs.microchip.com/oxy/GUID-346BCDAA-3BE2-4A0F-8F60-13EDF8CC90A1-en-US-1/index.html

Wiring
******

.. list-table:: Wiring Configuration
   :widths: auto
   :header-rows: 1

   * - MTCH9010 Signal
     - GPIO Pin
     - Pin Direction
     - Required for example

   * - VDD
     - VCC
     - N/A
     - Yes

   * - GND
     - GND
     - N/A
     - Yes

   * - UART TX
     - PA18
     - GPIO_OUTPUT
     - Yes

   * - UART RX
     - PA19
     - GPIO_INPUT
     - Yes

   * - Wake
     - PB14
     - GPIO_OUTPUT
     - Yes

   * - Output
     - PB15
     - GPIO_INPUT
     - No

   * - Heartbeat
     - PA20
     - GPIO_INPUT
     - No

References
**********

For more information about the MTCH9010, please see
https://www.microchip.com/en-us/product/mtch9010.

Building and Running
********************

To build the application, a board with the MTCH9010 connected to
a non-console UART bus is required. Two example overlays are
provided for the :zephyr:board:`saml21_xpro`.

.. tabs::

   .. tab:: Capacitive Liquid Mode

      .. zephyr-app-commands::
         :zephyr-app: samples/sensor/mtch9010
         :gen-args: -DDTC_OVERLAY_FILE=capacitive.overlay
         :board: saml21_xpro
         :goals: build flash

   .. tab:: Conductive Liquid Mode

      .. zephyr-app-commands::
         :zephyr-app: samples/sensor/mtch9010
         :gen-args: -DDTC_OVERLAY_FILE=conductive.overlay
         :board: saml21_xpro
         :goals: build flash

Sample Output
*************

*Note: The output above the "Booting Zephyr..." line are the literal values
being sent to the MTCH9010 to configure it.*

.. code-block:: console

   [00:00:00.010,000] <inf> mtch9010.0: "0"
   [00:00:00.011,000] <inf> mtch9010.0: "0"
   [00:00:00.013,000] <inf> mtch9010.0: "1"
   [00:00:00.014,000] <inf> mtch9010.0: "2"
   [00:00:00.197,000] <inf> mtch9010.0: "0"
   [00:00:00.199,000] <inf> mtch9010.0: "100"
   *** Booting Zephyr OS build v4.3.0-2498-gcefd950c5cc5 ***
   SENSOR_CHAN_MTCH9010_SW_OUT_STATE = 0
   SENSOR_CHAN_MTCH9010_OUT_STATE = 0
   SENSOR_CHAN_MTCH9010_REFERENCE_VALUE = 508
   SENSOR_CHAN_MTCH9010_THRESHOLD_VALUE = 100
   SENSOR_CHAN_MTCH9010_MEAS_RESULT = 508
   SENSOR_CHAN_MTCH9010_MEAS_DELTA = 0
   SENSOR_CHAN_MTCH9010_HEARTBEAT_ERROR_STATE = 0

   SENSOR_CHAN_MTCH9010_SW_OUT_STATE = 0
   SENSOR_CHAN_MTCH9010_OUT_STATE = 0
   SENSOR_CHAN_MTCH9010_REFERENCE_VALUE = 508
   SENSOR_CHAN_MTCH9010_THRESHOLD_VALUE = 100
   SENSOR_CHAN_MTCH9010_MEAS_RESULT = 508
   SENSOR_CHAN_MTCH9010_MEAS_DELTA = 0
   SENSOR_CHAN_MTCH9010_HEARTBEAT_ERROR_STATE = 0

<Repeats once per second>
