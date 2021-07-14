.. _imx8mm_evk_a53:

NXP i.MX8MM EVK (Cortex-A53)
#################################

Overview
********

i.MX8M Mini LPDDR4 EVK board is based on NXP i.MX8M Mini applications
processor, composed of a quad Cortex®-A53 cluster and a single Cortex®-M4 core.
Zephyr OS is ported to run on the Cortex®-A53 core.

- Board features:

  - RAM: 2GB LPDDR4
  - Storage:

    - SanDisk 16GB eMMC5.1
    - Micron 32MB QSPI NOR
    - microSD Socket
  - Wireless:

    - WiFi: 2.4/5GHz IEEE 802.11b/g/n
    - Bluetooth: v4.1
  - USB:

    - OTG - 2x type C
  - Ethernet
  - PCI-E M.2
  - Connectors:

    - 40-Pin Dual Row Header
  - LEDs:

    - 1x Power status LED
    - 1x UART LED
  - Debug

    - JTAG 20-pin connector
    - MicroUSB for UART debug, two COM ports for A53 and M4

More information about the board can be found at the
`NXP website`_.

Supported Features
==================

The Zephyr mimx8mm_evk_a53 board configuration supports the following hardware
features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| GIC-v3    | on-chip    | interrupt controller                |
+-----------+------------+-------------------------------------+
| ARM TIMER | on-chip    | system clock                        |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port                         |
+-----------+------------+-------------------------------------+

Devices
========
System Clock
------------

This board configuration uses a system clock frequency of 8 MHz.

Serial Port
-----------

This board configuration uses a single serial communication channel with the
CPU's UART2.

Programming and Debugging
*************************

Use U-Boot to load and kick zephyr.bin:

.. code-block:: console

    dhcp 0x93c00000 zephyr.bin; dcache flush; icache flush; dcache off; icache off; go 0x93c008a0

Use this configuration to run basic Zephyr applications and kernel tests,
for example, with the :ref:`synchronization_sample`:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: mimx8mm_evk_a53
   :goals: run

This will build an image with the synchronization sample app, boot it and
display the following console output:

.. code-block:: console

       ## Starting application at 0x40000788 ...
    *** Booting Zephyr OS build zephyr-v2.5.0-523-ge61ec4f277bf  ***
    thread_a: Hello World from cpu 0 on mimx8mm_evk_a53!
    thread_b: Hello World from cpu 0 on mimx8mm_evk_a53!
    thread_a: Hello World from cpu 0 on mimx8mm_evk_a53!
    thread_b: Hello World from cpu 0 on mimx8mm_evk_a53!
    thread_a: Hello World from cpu 0 on mimx8mm_evk_a53!

References
==========

.. _NXP website:
   https://www.nxp.com/design/development-boards/i.mx-evaluation-and-development-boards/evaluation-kit-for-thebr-i.mx-8m-mini-applications-processor:8MMINILPD4-EVK

.. _i.MX 8M Applications Processor Reference Manual:
   https://www.nxp.com/webapp/Download?colCode=IMX8MMRM
