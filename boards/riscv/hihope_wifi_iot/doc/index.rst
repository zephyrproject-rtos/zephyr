.. _hihope_wifi_iot:

HiHope WiFi-IoT
###############

Overview
********

HiHope Pegasus WiFi-IoT Development Suite is a set of development boards provided
by HopeRun, based on HiSilicon Hi3861V100.

Hi3861 is a highly integrated 2.4GHz WiFi chip that integrates IEEE 802.11 b/g/n
baseband and RF circuits, including power amplifier PA, low noise amplifier LNA,
RF balun, antenna switch, and power management module.

The Hi3861 WiFi baseband implements OFDM technology, and is backward compatible
with DSSS and CCK technologies, supporting IEEE 802.11 b/g/n protocols.
It supports 20MHz standard bandwidth and 5MHz/10MHz narrow bandwidth, providing
a maximum physical layer rate of 72.2Mbps.

The Hi3861 chip integrates a high-performance 32-bit microprocessor, providing
rich peripheral interfaces such as SPI, UART, I2C, I²S, PWM, GPIO, and
multi-channel ADC. It also supports the SDIO2.0 interface, with a clock speed of
up to 50MHz.

The Hi3861 series includes the Hi3861V100 and Hi3861LV100 models. The chip has
built-in SRAM and Flash, can run independently, and supports running programs on
Flash.

Hardware
********

- 32-bit core RISC-V CPU with maximum clock speed of 160MHz
- 352KB of internal SRAM
- 2MB of internal Flash
- 2.4GHz IEEE 802.11 b/g/n baseband and RF circuits
- Various peripherals:

  - 1 x SDIO 2.0 Slave
  - 2 x SPI
  - 2 x I²C
  - 3 x UART
  - 15 x GPIO
  - 7 x ADC input
  - 6 x PWM
  - 1 x I²S

Supported Features
==================

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| IO MUX    | on-chip    | pinctrl                             |
+-----------+------------+-------------------------------------+
| Timer     | on-chip    | systick                             |
+-----------+------------+-------------------------------------+

Other hardware features are not currently supported by Zephyr.

Programming and Debugging
*************************

Building & Flashing
===================

Here is an example for building the :ref:`blinky-sample` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: hihope_wifi_iot
   :goals: build

To flash the application, you need to specify the serial port of the board.

.. code-block:: console

   west flash --port <path/to/port>

References
**********

- `HiSilicon Hi3861V100 <https://www.hisilicon.com/en/products/smart-iot/ShortRangeWirelessIOT/Hi3861V100>`_
- `HiHope Pegasus WiFi-IoT Development Suite <http://www.hihope.org/pro/pro1.aspx?mtt=8>`_
- `Hi3861V100/Hi3861LV100/Hi3881V100 WiFi SoC User Manual <https://gitee.com/HiSpark/hi3861_hdu_iot_application/raw/master/doc/manual/Hi3861V100%EF%BC%8FHi3861LV100%EF%BC%8FHi3881V100%20WiFi%E8%8A%AF%E7%89%87%20%E7%94%A8%E6%88%B7%E6%8C%87%E5%8D%97.pdf>`_
