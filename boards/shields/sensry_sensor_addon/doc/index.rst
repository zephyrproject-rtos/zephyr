.. SPDX-FileCopyrightText: Copyright (c) 2026 Sensry GmbH
.. SPDX-License-Identifier: Apache-2.0

.. _sensry_sensor_addon:

Sensry Sensor Addon
###################

Overview
********

The Sensry Sensor Addon connects to the three 2 x 10-pin GPIO headers of a
supported Nordic development kit. It combines motion, environmental, light
and sound sensors with a real-time clock and GPIO expander.

.. list-table::
   :widths: 50 50
   :align: center

   * - .. figure:: sensry_sensor_addon.webp
          :alt: Sensry Sensor Addon top view
          :width: 280px
          :align: center

          Top view

     - .. figure:: sensry_sensor_addon_bottom.webp
          :alt: Sensry Sensor Addon bottom view
          :width: 280px
          :align: center

          Bottom view

Supported devices
*****************

The shield provides the following devices:

.. list-table:: Devices and devicetree node labels
   :header-rows: 1
   :widths: 40 35 25

   * - Device
     - Devicetree compatible
     - Node label
   * - BMI270 accelerometer and gyroscope
     - ``bosch,bmi270``
     - ``imu``
   * - LIS2MDL magnetometer
     - ``st,lis2mdl``
     - ``magnetometer``
   * - BME680 temperature, pressure, humidity and gas resistance
     - ``bosch,bme680``
     - ``environmental_sensor``
   * - OPT3002 light sensor
     - ``ti,opt3001``
     - ``light_sensor``
   * - PDM microphone controller
     - ``nordic,nrf-pdm``
     - ``microphone``
   * - Real-time clock
     - ``nxp,pcf85063a``
     - ``external_rtc``
   * - PCA9554-compatible GPIO expander
     - ``nxp,pca9554``
     - ``i2s_gpio_controller``

Compatibility
*************

A board-specific configuration is included for:

* :zephyr:board:`nrf54l15dk`: ``nrf54l15dk/nrf54l15/cpuapp``.

The shield bus labels, controller nodes and shield header contacts are:

.. list-table::
   :header-rows: 1
   :widths: 25 22 53

   * - Shield label
     - Devicetree node
     - Shield header contacts
   * - ``sensry_i2c``
     - ``&i2c22``
     - SDA P1 pin 15, SCL P1 pin 14
   * - ``sensry_spi``
     - ``&spi21``
     - SCK P2 pin 9, MOSI P2 pin 11, MISO P2 pin 12
   * - ``sensry_pdm``
     - ``&pdm21``
     - CLK P1 pin 7, DIN P1 pin 8

nRF54L15 DK configuration
=========================

SPI21 uses P2.06, P2.08 and P2.09 for SCK, MOSI and MISO. SPI22 cannot route
to these pins, and cross-domain SPI routing requires constant-latency mode.
The DK LEDs and PWM20 are disabled because they share shield pins. UART20 is
disabled to free the microphone pins; the console uses VCOM0 via UART30.

The TPM is not currently supported by the shield configuration. Its chip
select on P1.06 is held inactive by a GPIO hog; otherwise, the TPM drives the
shared SPI MISO line and sensor probing fails. Supporting the TPM requires
changing the board-specific configuration to remove the hog and configure the
TPM chip select.

Build and run
*************

Board support and examples
==========================

Board support and ready-to-use application examples are available on the
`Sensry Sensor Addon website <https://sensor-addon.sensry.io/>`_.

Sensor shell example
====================

Set ``--shield sensry_sensor_addon`` when you invoke ``west build``. For example,
build and flash the sensor shell sample:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/sensor_shell
   :board: nrf54l15dk/nrf54l15/cpuapp
   :shield: sensry_sensor_addon
   :gen-args: -DCONFIG_I2C_RTIO=y
   :goals: build flash

``CONFIG_I2C_RTIO`` enables the I2C transfers used by the BME680 asynchronous
sensor API. No sample-specific devicetree overlay is needed.

References
**********

* `Sensry <https://sensor-addon.sensry.io/>`_
