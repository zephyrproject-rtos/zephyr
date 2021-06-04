.. _imx8mp_evk:

NXP i.MX8MP EVK (Cortex-A53)
#################################

Overview
********

i.MX8M Plus LPDDR4 EVK board is based on NXP i.MX8M Plus applications
processor, composed of a quad Cortex®-A53 cluster and a single Cortex®-M7 core.
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

    dhcp 0x93c00000 zephyr.elf; dcache flush; icache flush; dcache off; icache off; bootelf zephyr.elf

Use this configuration to run basic Zephyr applications and kernel tests,
for example, with the :ref:`synchronization_sample`:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: imx8mp_evk
   :goals: run

This will build an image with the synchronization sample app, boot it and
display the following console output:

.. code-block:: console

      ## Starting application at 0xc0001074 ...
    *** Booting Zephyr OS build v2.6.0-rc3-8-gf689d5e7c216  ***
    Secondary CPU core 1 (MPID:0x1) is up
    thread_a: Hello World from cpu 0 on imx8mp_evk!
    thread_b: Hello World from cpu 1 on imx8mp_evk!
    thread_a: Hello World from cpu 0 on imx8mp_evk!
    thread_b: Hello World from cpu 1 on imx8mp_evk!
    thread_a: Hello World from cpu 0 on imx8mp_evk!
    thread_b: Hello World from cpu 1 on imx8mp_evk!
    thread_a: Hello World from cpu 0 on imx8mp_evk!
    thread_b: Hello World from cpu 1 on imx8mp_evk!
    thread_a: Hello World from cpu 0 on imx8mp_evk!
    thread_b: Hello World from cpu 1 on imx8mp_evk!

Use Jailhouse hypervisor, after root cell linux is up:

.. code-block:: console

    #jailhouse enable imx8mp.cell
    #jailhouse cell create imx8mp-zephyr.cell
    #jailhouse cell load 1 zephyr.bin -a 0xc0000000
    #jailhouse cell start 1

References
==========

.. _NXP website:
   https://www.nxp.com/design/development-boards/i-mx-evaluation-and-development-boards/evaluation-kit-for-the-i-mx-8m-plus-applications-processor:8MPLUSLPD4-EVK

.. _i.MX 8M Applications Processor Reference Manual:
   https://www.nxp.com/docs/en/reference-manual/IMX8MPRM.pdf
