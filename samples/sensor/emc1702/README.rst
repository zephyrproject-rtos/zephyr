.. zephyr:code-sample:: emc1702
   :name: EMC1702 High-Side Current Sense and Temperature Monitor
   :relevant-api: sensor_interface

   Get die temperature, ambient temperature, bus voltage, current and power from
   an EMC1702 sensor.

Overview
********

This sample application reads the internal die temperature, the external diode
(ambient) temperature, the bus voltage on SENSE+, the current through an external
sense resistor and the power computed by the device, once per second, and prints
the decoded values to the console.

The EMC1702 reports current as the voltage developed across an external sense
resistor, so the driver needs to know both the resistor value and the selected
full-scale range in order to convert the raw readings into amps. Power is
reported by the device as a ratio, which the driver scales using the same two
values. All of them are set in the devicetree, see `Devicetree Configuration`_.

Requirements
************

This sample requires a board with an EMC1702 connected over I2C. It was
developed with the `Current 3 Click`_ board, which carries the EMC1702 with a
10 mOhm sense resistor and ADDR_SEL strapped to ground, giving I2C address
0x4C.

.. _Current 3 Click: https://www.mikroe.com/current-3-click

The external (ambient) temperature channel requires a diode wired to the DP/DN
pins. When no diode is present, remove the ``external-diode-is-present``
property from the devicetree node: the driver then masks the external channel in
hardware, skips the diode fault check and reports ``-ENOTSUP`` for
:c:enumerator:`SENSOR_CHAN_AMBIENT_TEMP`.

Wiring
******

The EMC1702 is supplied from 3 V to 5.5 V, and the monitored bus can be in the
3 V to 24 V range. The sense resistor is connected between SENSE+ and SENSE-,
with SENSE+ on the supply side.

The default overlay wires the device to I2C0 of the
:zephyr:board:`esp32s3_devkitc`:

.. list-table:: Wiring Configuration
   :widths: auto
   :header-rows: 1

   * - EMC1702 Signal
     - ESP32-S3 Pin
     - Required

   * - VDD
     - 3V3
     - Yes

   * - GND
     - GND
     - Yes

   * - SMDATA
     - GPIO8
     - Yes

   * - SMCLK
     - GPIO9
     - Yes

   * - DP / DN
     - external diode
     - Only for the ambient temperature channel

The SMBus lines need pull-up resistors. The overlay configures the pins with
``bias-pull-up`` and ``drive-open-drain``, which is enough when the board does
not provide external pull-ups.

Devicetree Configuration
************************

The properties below must match the hardware, otherwise the current and power
readings will be wrong even though the device is detected correctly:

``sense-resistor-milliohms``
   Value of the external sense resistor, in milliohms.

``current-sense-range-mv``
   Full-scale range of the sense voltage: 10, 20, 40 or 80 mV. Together with the
   sense resistor this sets the full-scale current, for example 10 mV over
   10 mOhm gives 1 A.

``external-diode-is-present``
   Present only when a diode is wired to DP/DN.

The remaining properties (conversion rate, sampling time, averaging queues,
resistance error correction) are optional and documented in the
:dtcompatible:`microchip,emc1702` binding.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/emc1702
   :board: esp32s3_devkitc/esp32s3/procpu
   :goals: build flash

Sample Output
=============

When monitoring a 3.3 V bus drawing about 90 mA through a 10 mOhm sense
resistor, the output looks similar to the following, repeated every second:

.. code-block:: console

   *** Booting Zephyr OS build v4.4.0 ***
   EMC1702 ready, sampling every 1000 ms
   [     0 ms]  Tdie= 28.50 C  Tamb= 27.25 C  Vbus= 3.304 V  I= 0.0898 A  P= 0.2968 W
   [  1000 ms]  Tdie= 28.62 C  Tamb= 27.25 C  Vbus= 3.304 V  I= 0.0902 A  P= 0.2981 W
   [  2000 ms]  Tdie= 28.62 C  Tamb= 27.37 C  Vbus= 3.301 V  I= 0.0898 A  P= 0.2965 W

The current is bidirectional, so a negative value indicates current flowing in
the reverse direction:

.. code-block:: console

   [  3000 ms]  Tdie= 28.62 C  Tamb= 27.37 C  Vbus= 3.301 V  I= -0.0898 A  P= 0.2965 W

References
**********

 - `EMC1702 product page <https://www.microchip.com/en-us/product/emc1702>`_
 - `EMC1702 datasheet <https://ww1.microchip.com/downloads/aemDocuments/documents/OTH/ProductDocuments/DataSheets/EMC1702-Data-Sheet-DS20006455A.pdf>`_
