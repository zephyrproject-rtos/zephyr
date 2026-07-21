.. zephyr:board:: xiao_nrf54lm20a

Overview
********

`Seeed Studio XIAO nRF54LM20A`_ is a compact, high-performance development board featuring the
Nordic nRF54LM20A chip. This SoC integrates an ultra-low power multiprotocol 2.4 GHz radio and
an MCU with a 128 MHz Arm(R) Cortex(R)-M33 processor and a RISC-V Fast Lightweight Peripheral
Processor (FLPR) coprocessor. It offers 1.5 MB RRAM and 512 KB RAM.

The board features a built-in PMIC (Nordic NPM1300) with battery charging support, an RGB LED,
an LSM6DS3TR-C 6-axis IMU, USB HS, NFCT, and an external SPI flash (PY25Q64HA, 64 Mbit).
The XIAO form factor provides 28 pins (D0-D27) with support for GPIO, I2C, SPI, UART, ADC,
PWM, and more.

Designed for exceptional ultra-low power consumption, it significantly extends battery life.
Its robust radio supports Bluetooth(R), Matter, Thread, Zigbee, and a high-throughput
2.4 GHz proprietary mode. The board is ideal for compact, secure, and energy-efficient IoT
solutions such as smart wearables, industrial sensors, and advanced human-machine interfaces.


Hardware
********

- 128 MHz Arm(R) Cortex(R)-M33 processor
- RISC-V FLPR coprocessor for peripheral processing
- 1.5 MB RRAM (non-volatile memory)
- 512 KB RAM
- Multiprotocol 2.4 GHz radio supporting Bluetooth Low Energy, 802.15.4-2020,
  and 2.4 GHz proprietary modes
- Comprehensive set of peripherals including 14-bit ADC, high-speed serial interfaces,
  PDM/DMIC, and PWM
- Built-in PMIC (Nordic NPM1300) with battery charging
- RGB LED (blue, red, green)
- LSM6DS3TR-C 6-axis IMU (accelerometer + gyroscope)
- USB HS
- NFCT (Near Field Communication)
- External SPI flash (PY25Q64HA, 64 Mbit)
- 28-pin XIAO connector (D0-D27)
- Advanced security including TrustZone(R) isolation and cryptographic engine protection


For more information about the nRF54LM20A SoC and XIAO nRF54LM20A board, refer to these
documents:

- `nRF54LM20A Website`_
- `Seeed Studio XIAO nRF54LM20A`_
- `XIAO nRF54LM20A Wiki`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

In the following table, the column **Name** contains Pin names. For example, P1_0
means Pin number 0 on PORT1, as used in the board's datasheets and manuals.

+-------+-------------+--------------------------+
| Name  | Function    | Usage                    |
+=======+=============+==========================+
| P1_22 | GPIO        | LED0 (Blue)              |
+-------+-------------+--------------------------+
| P1_23 | GPIO        | LED1 (Red)               |
+-------+-------------+--------------------------+
| P1_24 | GPIO        | LED2 (Green)             |
+-------+-------------+--------------------------+
| P0_9  | GPIO        | Button 0                 |
+-------+-------------+--------------------------+
| P1_11 | USART20_TX  | UART Console TX          |
+-------+-------------+--------------------------+
| P1_10 | USART20_RX  | UART Console RX          |
+-------+-------------+--------------------------+
| P1_8  | USART21_TX  | XIAO Serial TX           |
+-------+-------------+--------------------------+
| P1_9  | USART21_RX  | XIAO Serial RX           |
+-------+-------------+--------------------------+
| P1_3  | TWIM22_SDA  | XIAO I2C SDA             |
+-------+-------------+--------------------------+
| P1_7  | TWIM22_SCL  | XIAO I2C SCL             |
+-------+-------------+--------------------------+
| P1_4  | SPIM23_SCK  | XIAO SPI SCK             |
+-------+-------------+--------------------------+
| P1_6  | SPIM23_MOSI | XIAO SPI MOSI            |
+-------+-------------+--------------------------+
| P1_5  | SPIM23_MISO | XIAO SPI MISO            |
+-------+-------------+--------------------------+
| P0_8  | TWIM30_SDA  | IMU I2C SDA              |
+-------+-------------+--------------------------+
| P0_7  | TWIM30_SCL  | IMU I2C SCL              |
+-------+-------------+--------------------------+
| P1_0  | GPIO        | XIAO D0                  |
+-------+-------------+--------------------------+
| P1_31 | GPIO        | XIAO D1                  |
+-------+-------------+--------------------------+
| P1_30 | GPIO        | XIAO D2                  |
+-------+-------------+--------------------------+
| P1_29 | GPIO        | XIAO D3                  |
+-------+-------------+--------------------------+
| P1_3  | GPIO        | XIAO D4                  |
+-------+-------------+--------------------------+
| P1_7  | GPIO        | XIAO D5                  |
+-------+-------------+--------------------------+
| P1_8  | GPIO        | XIAO D6                  |
+-------+-------------+--------------------------+
| P1_9  | GPIO        | XIAO D7                  |
+-------+-------------+--------------------------+
| P1_4  | GPIO        | XIAO D8                  |
+-------+-------------+--------------------------+
| P1_5  | GPIO        | XIAO D9                  |
+-------+-------------+--------------------------+
| P1_6  | GPIO        | XIAO D10                 |
+-------+-------------+--------------------------+
| P1_12 | GPIO        | Power Enable             |
+-------+-------------+--------------------------+
| P1_15 | GPIO        | PMIC I2C SDA             |
+-------+-------------+--------------------------+
| P1_16 | GPIO        | PMIC I2C SCL             |
+-------+-------------+--------------------------+
| P1_13 | PDM20_CLK   | DMIC Clock               |
+-------+-------------+--------------------------+
| P1_14 | PDM20_DIN   | DMIC Data                |
+-------+-------------+--------------------------+
| P2_1  | SPIM00_SCK  | External SPI Flash SCK   |
+-------+-------------+--------------------------+
| P2_2  | SPIM00_MOSI | External SPI Flash MOSI  |
+-------+-------------+--------------------------+
| P2_4  | SPIM00_MISO | External SPI Flash MISO  |
+-------+-------------+--------------------------+


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The XIAO nRF54LM20A contains a SAMD11 with CMSIS-DAP, allowing flashing, debugging, logging, etc. over
the USB port.

Flashing
========

Connect the XIAO nRF54LM20A board to your host computer using the USB port. A USB CDC ACM serial port
should appear on the host, that can be used to view logs from the flashed application.

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: xiao_nrf54lm20a/nrf54lm20a/cpuapp
   :goals: flash

Open a serial terminal (minicom, putty, etc.) connecting to the USB CDC ACM serial port.

Reset the board and you should see the following message in the terminal:

.. code-block:: console

   Hello World! xiao_nrf54lm20a/nrf54lm20a/cpuapp


.. _Seeed Studio XIAO nRF54LM20A:
   https://www.seeedstudio.com/

.. _XIAO nRF54LM20A Wiki:
   https://wiki.seeedstudio.com/

.. _nRF54LM20A Website:
   https://www.nordicsemi.com/
