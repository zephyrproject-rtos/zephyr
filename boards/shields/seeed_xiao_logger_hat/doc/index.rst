.. _seeed_xiao_logger_hat:

Seeed XIAO Logger Hat
#####################

Overview
********

The Seeed XIAO Logger Hat is an expansion board designed for
Seeed Studio XIAO boards. It provides additional peripherals connected
through the I2C interface.

The shield includes:

* Sensirion SHT4x temperature and humidity sensor
* NXP PCF8563 real-time clock (RTC)
* ROHM BH1750 light sensor
* voltage divider for battery voltage measurement
* sensor power control GPIO

Supported boards
****************

This shield is compatible with Seeed Studio XIAO boards supported by
Zephyr.

Currently supported:

   +----------------------+-----------------------------+
   | All features         | No battery monitoring       |
   +======================+=============================+
   | seed_xiao_esp32c6    | seeed_xiao_samd21           |
   +----------------------+-----------------------------+
   | seed_xiao_esp32c3    | seeed_xiao_rp2040           |
   +----------------------+-----------------------------+
   | seeed_xiao_rp2350    |                             |
   +----------------------+-----------------------------+
   | seeed_xiao_mg24      |                             |
   +----------------------+-----------------------------+

Hardware configuration
**********************

The shield uses the following peripherals:

I2C
===

The following devices are connected to the I2C bus:

+----------------------+----------------+
| Device               | Address        |
+======================+================+
| Sensirion SHT4x      | 0x44           |
+----------------------+----------------+
| NXP PCF8563 RTC      | 0x51           |
+----------------------+----------------+
| ROHM BH1750          | 0x23           |
+----------------------+----------------+

Sensor power control
====================

The sensors power supply can be controlled using a GPIO pin.

The shield defines a ``enable-sensors`` alias in the device tree.
The GPIO assignment is done with xiao_d nexus node

Configuration
*************

Enable the shield when building an application:

.. code-block:: console

   west build -b <board> --shield seeed_xiao_logger_hat

Dependencies
************

Kconfig options has been added to enable the following features:

* I2C support
* Sensor subsystem
* Sensirion SHT4x driver
* RTC subsystem
* NXP PCF8563 RTC driver
* ROHM BH1750 driver
* ADC support (for battery voltage measurement)
* Voltage divider

References
**********

* Seeed Studio XIAO Logger documentation - https://github.com/potblitd/XIAO-log/tree/main
