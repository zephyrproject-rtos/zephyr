.. SPDX-FileCopyrightText: Copyright (c) 2026 Sensry GmbH
.. SPDX-License-Identifier: Apache-2.0

.. _sensry_sensor_addon:

Sensry Sensor Addon
###################

Overview
********

The Sensry Sensor Addon is designed for Nordic Semiconductor nRF54 Series DKs, using
the Nordic Expansion Board header and native Zephyr drivers for plug-and-play
sensor integration.

It combines a BMI270 IMU, LIS2MDL magnetometer, BME680 environmental sensor,
light sensor, PDM microphone, real-time clock and GPIO expander.

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

.. list-table:: Devices, Zephyr drivers and devicetree node labels
   :header-rows: 1
   :widths: 40 35 25

   * - Device
     - Zephyr driver
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
   * - PDM microphone
     - ``nordic,nrf-pdm``
     - ``microphone``
   * - PCF85063A real-time clock
     - ``nxp,pcf85063a``
     - ``external_rtc``
   * - PCA9554-compatible GPIO expander
     - ``nxp,pca9554``
     - ``i2s_gpio_controller``

Compatibility
*************

The shield uses the Nordic Expansion Board header. A board-specific configuration
is included for:

* :zephyr:board:`nrf54l15dk`: ``nrf54l15dk/nrf54l15/cpuapp``.

Other nRF54 Series DKs, including the :zephyr:board:`nrf54lm20dk`, require a
board-specific overlay that maps the sensor interfaces and chip-select signals
to the host pins.

Each supported host has a board-specific overlay in the shield's ``boards/``
directory. It maps ``sensry_i2c``, ``sensry_spi`` and ``sensry_pdm`` to host
controllers, configures their pins, then includes ``sensry_sensor_addon.dtsi``
with the shared shield devices. No changes to the host board sources are needed.

The console uses the DK's VCOM0 interface.

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
