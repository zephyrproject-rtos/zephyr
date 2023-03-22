.. _beagleplay_cc1352:

BeaglePlay (CC1352)
###################

Overview
********

BeagleBoard.org BeaglePlay is an open hardware single board computer based on a TI Sitara AM6254
quad-core ARM Cortex-A53 SoC with an external TI SimpleLink multi-standard CC1352P7 wireless MCU
providing long-range, low-power connectivity.


.. figure:: img/beagleplay.png
   :width: 400px
   :align: center
   :alt: BeagleBoard.org BeaglePlay

   BeagleBoard.org BeaglePlay

Hardware
********

* Processors
  * TI Sitara AM6252 SoC
    * 4x ARM Cortex-A53
    * ARM Cortex-R5
    * ARM Cortex-M4
    * Dual-core 32-bit RISC Programmble Real-Time Unit (PRU)
  * TI SimpleLink CC1352P7 Wireless MCU
    * ARM Cortex-M4F programmable MCU
    * ARM Cortex-M0+ software-defined radio processor
* Memory
  * 2GB DDR4
  * 16GB eMMC flash
  * I2C EEPROM
* Wired connectivity
  * Gigabit Ethernet (RJ45)
  * Single-pair Ethernet with 5V/250mA PoDL output (RJ11)
  * HDMI
  * USB Type-A (host)
  * USB Type-C (client/power)
* Wireless connectivity
  * TI WL1807 2.4GHz/5GHz WiFi
  * BLE/SubG via CC1352P7
* Expansion
  * mikroBUS
  * Grove
  * QWIIC

BeaglePlay ARM Cortex-A53 CPUs typically run Linux, while the CC1352P7 Cortex-M4 typically runs Zephyr.


Supported Features
==================

The board configuration supports the following hardware features:

+-----------+------------+----------------------+
| Interface | Controller | Driver/Component     |
+===========+============+======================+
| GPIO      | on-chip    | gpio                 |
+-----------+------------+----------------------+
| MPU       | on-chip    | arch/arm             |
+-----------+------------+----------------------+
| NVIC      | on-chip    | arch/arm             |
+-----------+------------+----------------------+
| PINMUX    | on-chip    | pinmux               |
+-----------+------------+----------------------+
| UART      | on-chip    | serial               |
+-----------+------------+----------------------+

Connections and IOs
===================

CC1352 reset is connected to AM62 GPIO0_14.

+-------+--------------+-------------------------------------+
| Pin   | Function     | Usage                               |
+=======+==============+=====================================+
| DIO5  | N/C          |                                     |
+-------+--------------+-------------------------------------+
| DIO6  | N/C          |                                     |
+-------+--------------+-------------------------------------+
| DIO7  | N/C          |                                     |
+-------+--------------+-------------------------------------+
| DIO8  | N/C          |                                     |
+-------+--------------+-------------------------------------+
| DIO9  | N/C          |                                     |
+-------+--------------+-------------------------------------+
| DIO10 | N/C          |                                     |
+-------+--------------+-------------------------------------+
| DIO11 | N/C          |                                     |
+-------+--------------+-------------------------------------+
| DIO12 | CC1352_RX    | AM62 UART6_TXD                      |
+-------+--------------+-------------------------------------+
| DIO13 | CC1352_TX    | AM62 UART6_RXD                      |
+-------+--------------+-------------------------------------+
| DIO14 | N/C          |                                     |
+-------+--------------+-------------------------------------+
| DIO15 | CC1352_BOOT  | AM62 GPIO0_13                       |
+-------+--------------+-------------------------------------+
| DIO16 | CC1352_TDO   | TAG-CONNECT TDO                     |
+-------+--------------+-------------------------------------+
| DIO17 | CC1352_TDI   | TAG-CONNECT TDI                     |
+-------+--------------+-------------------------------------+
| DIO18 | N/C          |                                     |
+-------+--------------+-------------------------------------+
| DIO19 | N/C          |                                     |
+-------+--------------+-------------------------------------+
| DIO20 | N/C          |                                     |
+-------+--------------+-------------------------------------+
| DIO21 | N/C          |                                     |
+-------+--------------+-------------------------------------+
| DIO22 | N/C          |                                     |
+-------+--------------+-------------------------------------+
| DIO23 | N/C          |                                     |
+-------+--------------+-------------------------------------+
| DIO24 | N/C          |                                     |
+-------+--------------+-------------------------------------+
| DIO25 | N/C          |                                     |
+-------+--------------+-------------------------------------+
| DIO26 | N/C          |                                     |
+-------+--------------+-------------------------------------+
| DIO27 | LED1         | CC1352_LED1 yellow LED9             |
+-------+--------------+-------------------------------------+
| DIO28 | LED2         | CC1352_LED2 yellow LED8             |
+-------+--------------+-------------------------------------+
| DIO29 | RF_PA        | Antenna mux PA enable               |
+-------+--------------+-------------------------------------+
| DIO30 | RF_SUB1G     | Antenna mux SubG enable             |
+-------+--------------+-------------------------------------+

Programming and Debugging
*************************

Flashing
========

To flash, disable the existing driver that ties up the serial port and use
the customized BSL Python script.

* https://docs.beagleboard.org/latest/boards/beagleplay/demos-and-tutorials/zephyr-cc1352-development.html

Debugging
=========

For debugging, you can use the serial port or JTAG. You can use OpenOCD
over the Tag-Connect header on the board.

* https://docs.beagleboard.org/latest/accessories/cables.html#tagconnect-jtag

References
**********

BeagleBoard.org BeaglePlay reference:
  https://beagleplay.org
