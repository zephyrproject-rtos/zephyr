.. zephyr:board:: imx95_evk_15x15

Overview
********

The i.MX95 EVK (IMX95LP4XEVK-15) board is a platform designed to show the
most commonly used features of the i.MX 95 applications processor.
It is an entry-level development board, which helps developers to get familiar
with the processor before investing a large amount of resources in more
specific designs. The i.MX 95 device on the board comes in a compact
15 x 15 mm package.

Hardware
********

- i.MX 95 applications processor

  - The processor integrates up to six Arm Cortex-A55 cores, and supports
    functional safety with built-in Arm Cortex-M33 and -M7 cores

- DRAM memory: 8-Gbit LPDDR4x DRAM
- eMMC: 64 GB Micron eMMC
- USB interface: Two USB ports: Type-A and Type-C
- Audio codec interface

  - One audio codec WM8962B
  - One 3.5 mm 4-pole CTIA standard audio jack
  - One 4-pin connector to connect speaker

- Ethernet interface

  - ENET2 controller

    - Supports 100 Mbit/s or 1000 Mbit/s RGMII Ethernet with one RJ45
      connector connected with an external PHY, RTL8211

  - ENET1 controller

    - Supports 100 Mbit/s or 1000 Mbit/s RGMII Ethernet with one RJ45
      connector connected with an external PHY, RTL8211

- M.2 interface: One Wi-Fi/Bluetooth Murata Type-2EL module based on NXP AW612
  chip supporting 1x1 Wi-Fi 6 and Bluetooth 5.3

- MIPI CSI interface: Connects to one 22-pin or 36-pin miniSAS connector using x4 lane
  configuration
- MIPI CSIDSI interface: Connects to one 36-pin miniSAS connector using x4 lane
  configuration
- LVDS interface: two mini-SAS connectors each with x4-lane configuration
- CAN interface: One 4-pin CAN headers for external connection
- SD card interface: one 4-bit SD3.0 microSD card
- I2C interface: I2C1 to I2C6 controllers
- FT4232H I2C interface: PCT2075 temperature sensor and current monitoring devices
- ADC interface: two 4-channel ADC header
- Debug interface

  - One USB-to-UART/MPSSE device, FT4232H
  - One USB 2.0 Type-C connector (J31) for FT4232H provides quad serial ports

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
------------

This board configuration uses a system clock frequency of 24 MHz for Cortex-A55.
Cortex-A55 Core runs up to 1.8 GHz.

Serial Port
-----------

This board configuration uses a single serial communication channel with the
CPU's UART1 for Cortex-A55.

Programming and Debugging (A55)
*******************************

Use this configuration to run basic Zephyr applications and kernel tests,
for example, with the :zephyr:code-sample:`synchronization` sample:

1. Build and run the Non-SMP application

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: imx95_evk_15x15/mimx9596/a55
   :goals: build

This will build an image (zephyr.bin) with the synchronization sample app.

Copy the compiled ``zephyr.bin`` to the first FAT partition of the SD card and
plug the SD card into the board. Power it up and stop the u-boot execution at
prompt.

Use U-Boot to load and kick zephyr.bin to Cortex-A55 Core1:

.. code-block:: console

    fatload mmc 1:1 0xd0000000 zephyr.bin; dcache flush; icache flush; cpu 1 release 0xd0000000


Or use the following command to kick zephyr.bin to Cortex-A55 Core0:

.. code-block:: console

    fatload mmc 1:1 0xd0000000 zephyr.bin; dcache flush; icache flush; go 0xd0000000


It will display the following console output:

.. code-block:: console

    *** Booting Zephyr OS build v3.6.0-4569-g483c01ca11a7 ***
    thread_a: Hello World from cpu 0 on imx95_evk_15x15!
    thread_b: Hello World from cpu 0 on imx95_evk_15x15!
    thread_a: Hello World from cpu 0 on imx95_evk_15x15!
    thread_b: Hello World from cpu 0 on imx95_evk_15x15!
    thread_a: Hello World from cpu 0 on imx95_evk_15x15!

.. _System Control and Management Interface (SCMI):
   https://developer.arm.com/documentation/den0056/latest/

.. _i.MX Linux BSP release:
   https://www.nxp.com/design/design-center/software/embedded-software/i-mx-software/embedded-linux-for-i-mx-applications-processors:IMXLINUX

.. _MCUX SDK release:
   https://mcuxpresso.nxp.com/

References
==========

More information can refer to NXP official website:
`NXP website`_.

.. _NXP website:
   https://www.nxp.com/products/processors-and-microcontrollers/arm-processors/i-mx-applications-processors/i-mx-9-processors/i-mx-95-applications-processor-family-high-performance-safety-enabled-platform-with-eiq-neutron-npu:iMX95
