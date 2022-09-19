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

The Zephyr mimx8mp_evk_a53 board configuration supports the following hardware
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
CPU's UART4.

Programming and Debugging
*************************

Copy the compiled ``zephyr.bin`` to the first FAT partition of the SD card and
plug the SD card into the board. Power it up and stop the u-boot execution at
prompt.

Use U-Boot to load and kick non-smp zephyr.bin:

.. code-block:: console

    mw 303d0518 f 1; fatload mmc 1:1 0xc0000000 zephyr.bin; dcache flush; icache flush; dcache off; icache off; go 0xc0000000

Or kick SMP zephyr.bin:

.. code-block:: console

    mw 303d0518 f 1; fatload mmc 1:1 0xc0000000 zephyr.bin; dcache flush; icache flush; dcache off; icache off; cpu release 2 0xc0000000

Use this configuration to run basic Zephyr applications and kernel tests,
for example, with the :ref:`synchronization_sample`:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: mimx8mp_evk_a53
   :goals: run

This will build an image with the synchronization sample app, boot it and
display the following console output:

.. code-block:: console

    *** Booting Zephyr OS build zephyr-v3.1.0-3575-g44dd713bd883  ***
    thread_a: Hello World from cpu 0 on mimx8mp_evk_a53!
    thread_b: Hello World from cpu 0 on mimx8mp_evk_a53!
    thread_a: Hello World from cpu 0 on mimx8mp_evk_a53!
    thread_b: Hello World from cpu 0 on mimx8mp_evk_a53!
    thread_a: Hello World from cpu 0 on mimx8mp_evk_a53!

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
