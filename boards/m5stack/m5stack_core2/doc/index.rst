.. _m5stack_core2:

M5Stack Core2
#############

Overview
********

M5Stack Core2 is an ESP32-based development board from M5Stack. It is the successor for the Core module.

M5Stack Core2 features the following integrated components:

- ESP32-D0WDQ6-V3 chip (240MHz dual core, 600 DMIPS, 520KB SRAM, Wi-Fi)
- PSRAM 8MB
- Flash 16MB
- LCD IPS TFT 2", 320x240 px screen (ILI9342C)
- Touch screen (FT6336U)
- PMU AXP192
- Audio NS4168 amplifier (1W-092 speaker)
- Vibration motor
- RTC BM8563
- USB CP2104
- SD-Card slot
- Grove connector
- IMO 6-axis IMU MPU6886
- MIC SPM1423
- Battery 390mAh 3,7V

.. figure:: img/m5stack_core2.webp
        :align: center
        :alt: M5Stack-Core2
        :width: 400 px

        M5Stack-Core2 module

Functional Description
**********************

The following table below describes the key components, interfaces, and controls
of the M5Stack Core2 board.

.. _M5Core2 Schematic: https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/schematic/Core/CORE2_V1.0_SCH.pdf
.. _MPU-ESP32: https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/core/esp32_datasheet_en_v3.9.pdf
.. _TOUCH-FT6336U: https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/core/Ft6336GU_Firmware%20外部寄存器_20151112-%20EN.xlsx
.. _SND-NS4168: https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/core/NS4168_CN_datasheet.pdf
.. _MPU-6886: https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/core/MPU-6886-000193%2Bv1.1_GHIC_en.pdf
.. _LCD-ILI9342C: https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/core/ILI9342C-ILITEK.pdf
.. _SPM-1423: https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/core/SPM1423HM4H-B_datasheet_en.pdf
.. _RTC-BM8563: https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/core/BM8563_V1.1_cn.pdf
.. _SY7088: https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/core/SY7088-Silergy.pdf
.. _PMU-AXP192: https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/core/AXP192_datasheet_en.pdf
.. _VIB-1072_RFN01: https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/core/1027RFN01-33d.pdf

+------------------+--------------------------------------------------------------------------+-----------+
| Key Component    | Description                                                              | Status    |
+==================+==========================================================================+===========+
| ESP32-D0WDQ6-V2  | This `MPU-ESP32`_ module provides complete Wi-Fi and Bluetooth           | supported |
| module           | functionalities and integrates a 16-MB SPI flash.                        |           |
+------------------+--------------------------------------------------------------------------+-----------+
| 32.768 kHz RTC   | External precision 32.768 kHz crystal oscillator serves as a clock with  | supported |
|                  | low-power consumption while the chip is in Deep-sleep mode.              |           |
+------------------+--------------------------------------------------------------------------+-----------+
| Status LED       | One user LED connected to the GPIO pin.                                  | supported |
+------------------+--------------------------------------------------------------------------+-----------+
| USB Port         | USB interface. Power supply for the board as well as the                 | supported |
|                  | communication interface between a computer and the board.                |           |
|                  | Contains: TypeC x 1, GROVE(I2C+I/O+UART) x 1                             |           |
+------------------+--------------------------------------------------------------------------+-----------+
| Reset button     | Reset button                                                             | supported |
+------------------+--------------------------------------------------------------------------+-----------+
| Power Switch     | Power on/off button.                                                     | supported |
+------------------+--------------------------------------------------------------------------+-----------+
| LCD screen       | Built-in LCD TFT display \(`LCD-ILI9342C`_, 2", 320x240 px\)             | supported |
|                  | controlled via SPI interface                                             |           |
+------------------+--------------------------------------------------------------------------+-----------+
| SD-Card slot     | SD-Card connection via SPI-mode.                                         | supported |
+------------------+--------------------------------------------------------------------------+-----------+
| 6-axis IMU       | The `MPU-6886`_ is a 6-axis motion tracker (6DOF IMU) device that        | supported |
| MPU6886          | combines a 3-axis gyroscope and a 3-axis accelerometer.                  |           |
|                  | For details please refer to :ref:`m5stack_core2_ext`                     |           |
+------------------+--------------------------------------------------------------------------+-----------+
| Grove port       | Note: Grove port requires 5V to be enabled via `bus_5v` regulator        | supported |
+------------------+--------------------------------------------------------------------------+-----------+
| Built-in         | The `SPM-1423`_ I2S driven microphone.                                   | todo      |
| microphone       |                                                                          |           |
+------------------+--------------------------------------------------------------------------+-----------+
| Built-in speaker | 1W speaker for audio output via I2S interface.                           | todo      |
+------------------+--------------------------------------------------------------------------+-----------+
| Battery-support  | Power supply via battery is supported automatically. But there is no     | todo      |
|                  | possibility to query current battery status.                             |           |
+------------------+--------------------------------------------------------------------------+-----------+

Supported Features
==================

The Zephyr m5stack_core2 board configuration supports the following hardware features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port-polling;                |
|           |            | serial port-interrupt               |
+-----------+------------+-------------------------------------+
| PINMUX    | on-chip    | pinmux                              |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| I2C       | on-chip    | i2c                                 |
+-----------+------------+-------------------------------------+
| SPI       | on-chip    | spi                                 |
+-----------+------------+-------------------------------------+
| CLOCK     | on-chip    | reset and clock control             |
+-----------+------------+-------------------------------------+
| COUNTER   | on-chip    | rtc                                 |
+-----------+------------+-------------------------------------+
| WATCHDOG  | on-chip    | independent watchdog                |
+-----------+------------+-------------------------------------+
| PWM       | on-chip    | pwm                                 |
+-----------+------------+-------------------------------------+
| ADC       | on-chip    | adc                                 |
+-----------+------------+-------------------------------------+
| DAC       | on-chip    | dac                                 |
+-----------+------------+-------------------------------------+
| die-temp  | on-chip    | die temperature sensor              |
+-----------+------------+-------------------------------------+


Start Application Development
*****************************

Before powering up your M5Stack Core2, please make sure that the board is in good
condition with no obvious signs of damage.

System requirements
===================

Prerequisites
-------------

Espressif HAL requires WiFi and Bluetooth binary blobs in order work. Run the command
below to retrieve those files.

.. code-block:: console

   west blobs fetch hal_espressif

.. note::

   It is recommended running the command above after :file:`west update`.

Building & Flashing
-------------------

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: m5stack_core2/esp32/procpu
   :goals: build

The usual ``flash`` target will work with the ``m5stack_core2`` board
configuration. Here is an example for the :ref:`hello_world`
application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: m5stack_core2/esp32/procpu
   :goals: flash

The baud rate of 921600bps is set by default. If experiencing issues when flashing,
try using different values by using ``--esp-baud-rate <BAUD>`` option during
``west flash`` (e.g. ``west flash --esp-baud-rate 115200``).

You can also open the serial monitor using the following command:

.. code-block:: shell

   west espressif monitor

After the board has automatically reset and booted, you should see the following
message in the monitor:

.. code-block:: console

   ***** Booting Zephyr OS vx.x.x-xxx-gxxxxxxxxxxxx *****
   Hello World! m5stack_core2

Debugging
---------

M5Stack Core2 debugging is not supported due to pinout limitations.

Related Documents
*****************

- `M5Stack-Core2 schematic <https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/schematic/Core/CORE2_V1.0_SCH.pdf>`_ (PDF)
- `ESP32-PICO-D4 Datasheet <https://www.espressif.com/sites/default/files/documentation/esp32-pico-d4_datasheet_en.pdf>`_ (PDF)
- `M5StickC PLUS docs <https://docs.m5stack.com/en/core/m5stickc_plus>`_
- `ESP32 Datasheet <https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf>`_ (PDF)
- `ESP32 Hardware Reference <https://docs.espressif.com/projects/esp-idf/en/latest/esp32/hw-reference/index.html>`_
