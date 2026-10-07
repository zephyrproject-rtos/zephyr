.. zephyr:board:: mikroe_uni_clicker

Overview
********

The MikroE `UNI Clicker`_ is a development board with a socket for MikroE MCU
cards, four mikroBUS™ sockets, four user LEDs, four user buttons and a USB
connector. The MCU is not part of the board: it sits on a plug-in MCU card that
is selected through the board target.

+---------------------------------------+-----------------------------------------------+
| MCU card                              | Board target                                  |
+=======================================+===============================================+
| MCU CARD 4 for STM32 (`STM32F429NI`_) | ``mikroe_uni_clicker/stm32f429xx/mcu_card_4`` |
+---------------------------------------+-----------------------------------------------+

The MCU card must always be given; building for ``mikroe_uni_clicker/stm32f429xx``
alone fails with an error asking for the card.

Hardware
********

The UNI Clicker is described independently of the MCU card. Its LEDs, buttons
and mikroBUS sockets reference the pins of the two MCU card connectors
(``left_connector`` and ``right_connector``), which each MCU card maps to its
own MCU pins.

User LEDs and buttons
=====================

+--------+-----------+-----------------------+
| Alias  | Node      | MCU card connector    |
+========+===========+=======================+
| led0   | led_a     | ``left_connector`` 34 |
+--------+-----------+-----------------------+
| led1   | led_b     | ``left_connector`` 35 |
+--------+-----------+-----------------------+
| led2   | led_c     | ``left_connector`` 36 |
+--------+-----------+-----------------------+
| led3   | led_d     | ``left_connector`` 24 |
+--------+-----------+-----------------------+
| sw0    | button_a  | ``left_connector`` 88 |
+--------+-----------+-----------------------+
| sw1    | button_b  | ``left_connector`` 94 |
+--------+-----------+-----------------------+
| sw2    | button_c  | ``left_connector`` 93 |
+--------+-----------+-----------------------+
| sw3    | button_d  | ``left_connector`` 61 |
+--------+-----------+-----------------------+

mikroBUS sockets
================

The four sockets are described as ``mikro-bus`` GPIO nexus nodes
(``mikrobus_1_header`` to ``mikrobus_4_header``) on top of ``left_connector``.
Which SPI, I2C and UART controllers reach a socket depends on the MCU card; see
the MCU card section below. The MCU card labels them ``mikrobus_<n>_spi``,
``mikrobus_<n>_i2c`` and ``mikrobus_<n>_uart``. ``mikrobus_header``,
``mikrobus_spi``, ``mikrobus_i2c`` and ``mikrobus_uart`` refer to mikroBUS 1,
so mikroBUS shields use that socket by default.

USB
===

The UNI Clicker USB power switch and ID lines are on ``right_connector`` pins 127
and 128. Each MCU card drives them high for USB device mode with GPIO hogs; on
the MCU CARD 4 for STM32 they are PF3 and PF10.

Supported Features
==================

.. zephyr:board-supported-hw::

MCU CARD 4 for STM32
********************

- `STM32F429NI`_ Arm® Cortex®-M4 with FPU, 2 MB flash, 256 KB RAM
- 25 MHz HSE crystal, system clock configured to 168 MHz

User LEDs and buttons
=====================

+-----------+------+-----------+------+
| Node      | Pin  | Node      | Pin  |
+===========+======+===========+======+
| led_a     | PJ5  | button_a  | PF14 |
+-----------+------+-----------+------+
| led_b     | PJ6  | button_b  | PF13 |
+-----------+------+-----------+------+
| led_c     | PJ7  | button_c  | PE11 |
+-----------+------+-----------+------+
| led_d     | PH14 | button_d  | PG4  |
+-----------+------+-----------+------+

mikroBUS sockets
================

+------------+-------+------------------------+------------------+--------------+
| Socket     | CS    | SPI (SCK/MISO/MOSI)    | UART (RX/TX)     | I2C (SCL/SDA)|
+============+=======+========================+==================+==============+
| mikroBUS 1 | PB9   | SPI6: PG13/PG12/PG14   | UART8: PE0/PE1   | I2C2: PF1/PF0|
+------------+-------+------------------------+------------------+--------------+
| mikroBUS 2 | PI0   | SPI2: PI1/PI2/PI3      | UART4: PC11/PC10 | I2C1: PB6/PB7|
+------------+-------+------------------------+------------------+--------------+
| mikroBUS 3 | PH15  | SPI2: PI1/PI2/PI3      | USART6: PC7/PC6  | I2C1: PB6/PB7|
+------------+-------+------------------------+------------------+--------------+
| mikroBUS 4 | PE15  | SPI4: PE12/PE13/PE14   | USART3: PD9/PD8  | I2C2: PH4/PH5|
+------------+-------+------------------------+------------------+--------------+

mikroBUS 2 and 3 share the SPI2 and I2C1 buses. On SPI2, chip select 0 (PI0) is
mikroBUS 2 and chip select 1 (PH15) is mikroBUS 3, so a device on mikroBUS 2 uses
``reg = <0>`` and a device on mikroBUS 3 uses ``reg = <1>``.

mikroBUS 1 and 4 are both wired to I2C2, which is configured for mikroBUS 1 by
default, so there is no ``mikrobus_4_i2c`` label. To use I2C on mikroBUS 4
instead, override the pins in an overlay:

.. code-block:: devicetree

   &i2c2 {
      pinctrl-0 = <&i2c2_scl_ph4 &i2c2_sda_ph5>;
   };

The bus controllers are left disabled; enable the ones a mikroBUS click board
needs from the application.

Serial console
==============

The Zephyr console and shell are on a USB CDC ACM virtual serial port
(``board_cdc_acm_uart``) provided by the STM32 USB OTG FS controller.

Ethernet
========

The MCU CARD 4 for STM32 has an RMII Ethernet PHY, which is described in the
board devicetree. The UNI Clicker has no RJ45 connector or magnetics, so the
Ethernet controller and the PHY are disabled by default. When the PHY is
connected to an RJ45 jack with magnetics, enable them from an application
overlay and set :kconfig:option:`CONFIG_ETH_DRIVER`:

.. code-block:: devicetree

   &mac {
      status = "okay";
   };

   &mdio {
      status = "okay";
   };

   &eth_phy {
      status = "okay";
   };

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The board is flashed and debugged with a SEGGER J-Link probe over SWD.

Applications for the ``mikroe_uni_clicker/stm32f429xx/mcu_card_4`` board target
can be built and flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: mikroe_uni_clicker/stm32f429xx/mcu_card_4
   :goals: build flash

References
**********

.. target-notes::

.. _UNI Clicker:
   https://www.mikroe.com/uni-clicker

.. _STM32F429NI:
   https://www.st.com/en/microcontrollers-microprocessors/stm32f429ni.html
