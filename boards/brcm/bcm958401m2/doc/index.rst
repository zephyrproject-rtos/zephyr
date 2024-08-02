.. _bcm958401m2:

Broadcom BCM958401M2
####################

Overview
********
The Broadcom BCM958401M2 board utilizes the Valkyrie BCM58400 SoC to
provide support for PCIe offload engine functionality.

Hardware
********
The BCM958401M2 is a PCIe card with the following physical features:

* PCIe Gen3 interface
* RS232 UART (optionally populated)
* JTAG (optionally populated)

Supported Features
==================
The Broadcom BCM958401M2 board configuration supports the following hardware
features:

+-----------+------------+--------------------------------------+
| Interface | Controller | Driver/Component                     |
+===========+============+======================================+
| NVIC      | on-chip    | nested vectored interrupt controller |
+-----------+------------+--------------------------------------+
| UART      | on-chip    | serial port                          |
+-----------+------------+--------------------------------------+

Other hardware features have not been enabled yet for this board.

The default configuration can be found in
:zephyr_file:`boards/brcm/bcm958401m2/bcm958401m2_defconfig`

Connections and IOs
===================


Programming and Debugging
*************************

Flashing
========

The flash on board is not supported by Zephyr at this time.
Board is booted over PCIe interface.

Debugging
=========
The bcm958401m2 board includes pads for soldering a JTAG connector.
Zephyr applications running on the M7 core can also be tested by observing UART console output.


References
**********
