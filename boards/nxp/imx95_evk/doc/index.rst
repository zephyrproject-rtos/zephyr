.. _imx95_evk:

NXP i.MX95 EVK (Cortex-A55)
############################

Overview
********

i.MX95 MPU is composed of one cluster of 6x Cortex®-A55 cores and a single
Cortex®-M7 core and a Cortex®-M33 core. Zephyr OS is ported to run on one of
the Cortex®-A55 core.

- Board features:

  - RAM: 16GB LPDDR5
  - Storage:

    - Micron 64GB eMMC
    - microSD Socket
  - Wireless:

    - One Wi-Fi/Bluetooth Murata Type-2EL module based on NXP AW693 chip
      supporting 2x2 Wi-Fi 6 and Bluetooth 5.2
  - USB:

    - Two USB ports: Type A (USB 2.0) and Type C (USB 3.0)
  - Ethernet
  - PCI-E:

    - M.2
    - x4 slot
  - LEDs:

    - 4x Power status LED
    - 3x UART LED
  - Debug

    - JTAG 20-pin connector
    - MicroUSB for UART debug, three COM ports for A55, M7 and M33


Supported Features
==================

The Zephyr imx95_evk board configuration supports the following hardware
features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| GIC-v4    | on-chip    | interrupt controller                |
+-----------+------------+-------------------------------------+
| ARM TIMER | on-chip    | system clock                        |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port                         |
+-----------+------------+-------------------------------------+

Devices
========
System Clock
------------

This board configuration uses a system clock frequency of 24 MHz.
Cortex-A55 Core runs up to 1.8 GHz.

Serial Port
-----------

This board configuration uses a single serial communication channel with the
CPU's UART1.

Programming and Debugging
*************************

Copy the compiled ``zephyr.bin`` to the first FAT partition of the SD card and
plug the SD card into the board. Power it up and stop the u-boot execution at
prompt.

Use U-Boot to load and kick zephyr.bin to Cortex-A55 Core1:

.. code-block:: console

    fatload mmc 1:1 0xd0000000 zephyr.bin; dcache flush; icache flush; dcache off; icache off; cpu 1 release 0xd0000000


Or use the following command to kick zephyr.bin to Cortex-A55 Core0:

.. code-block:: console

    fatload mmc 1:1 0xd0000000 zephyr.bin; dcache flush; icache flush; dcache off; icache off; go 0xd0000000


Use this configuration to run basic Zephyr applications and kernel tests,
for example, with the :zephyr:code-sample:`synchronization` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: imx95_evk/mimx9596/a55
   :goals: run

This will build an image with the synchronization sample app, boot it and
display the following ram console output:

.. code-block:: console

    *** Booting Zephyr OS build v3.6.0-4569-g483c01ca11a7 ***
    thread_a: Hello World from cpu 0 on imx95_evk!
    thread_b: Hello World from cpu 0 on imx95_evk!
    thread_a: Hello World from cpu 0 on imx95_evk!
    thread_b: Hello World from cpu 0 on imx95_evk!
    thread_a: Hello World from cpu 0 on imx95_evk!

References
==========

More information can refer to NXP official website:
`NXP website`_.

.. _NXP website:
   https://www.nxp.com/products/processors-and-microcontrollers/arm-processors/i-mx-applications-processors/i-mx-9-processors/i-mx-95-applications-processor-family-high-performance-safety-enabled-platform-with-eiq-neutron-npu:iMX95
