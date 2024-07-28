.. _imx95_evk:

NXP i.MX95 EVK
##############

Overview
********

The i.MX95 EVK (IMX95LPD5EVK-19) board is a platform designed to show the
most commonly used features of the i.MX 95 automotive applications processor.
It is an entry-level development board, which helps developers to get familiar
with the processor before investing a large amount of resources in more
specific designs. The i.MX95 device on the board comes in a compact
19 x 19 mm package.

Hardware
********

- i.MX 95 automotive applications processor

  - The processor integrates up to six Arm Cortex-A55 cores, and supports
    functional safety with built-in Arm Cortex-M33 and -M7 cores

- DRAM memory: 128-Gbit LPDDR5 DRAM
- eMMC: 64 GB Micron eMMC
- SPI NOR flash memory: 1 Gbit octal flash memory
- USB interface: Two USB ports: Type-A and Type-C
- Audio codec interface

  - One audio codec WM8962BECSN/R with one TX and RX lane
  - One 3.5 mm 4-pole CTIA standard audio jack
  - One 4-pin connector to connect speaker

- Ethernet interface

  - ENET2 controller

    - Connects to a 60-pin Ethernet connector
    - Supports Ethernet PHY daughter cards that can be configured to operate
      at 100 Mbit/s or 1000 Mbit/s

  - ENET1 controller

    - Supports 100 Mbit/s or 1000 Mbit/s RGMII Ethernet with one RJ45
      connector connected with an external PHY, RTL8211

  - 10 Gbit Ethernet controller

    - Supports XFI and USXGMII interfaces with one 10 Gbit RJ45 ICM connected
      with an external PHY, Marvell AQR113C

- M.2 interface: One Wi-Fi/Bluetooth Murata Type-2EL module based on NXP AW693
  chip supporting 2x2 Wi-Fi 6 and Bluetooth 5.2

- MIPI CSI interface: Connects to one 36-pin miniSAS connector using x4 lane
  configuration
- MIPI CSIDSI interface: Connects to one 36-pin miniSAS connector using x4 lane
  configuration
- LVDS interface: two mini-SAS connectors each with x4-lane configuration
- CAN interface: Two 4-pin CAN headers for external connection
- SD card interface: one 4-bit SD3.0 microSD card
- I2C interface: I2C1 to I2C7 controllers
- FT4232H I2C interface: PCT2075 temperature sensor and current monitoring devices
- DMIC interface: two digital microphones (DMIC) providing a single-bit PDM output
- ADC interface: two 4-channel ADC header
- Audio board interface

  - Supports PCIe x4 slot for Quantum board connection
  - Supports PCIe x8 slot for Audio I/O board connection

- Debug interface

  - One USB-to-UART/MPSSE device, FT4232H
  - One USB 2.0 Type-C connector (J31) for FT4232H provides quad serial ports

Supported Features
==================

The Zephyr imx95_evk_m7 board configuration supports the following hardware features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | interrupt controller                |
+-----------+------------+-------------------------------------+
| SYSTICK   | on-chip    | systick                             |
+-----------+------------+-------------------------------------+
| CLOCK     | on-chip    | clock_control                       |
+-----------+------------+-------------------------------------+
| PINMUX    | on-chip    | pinmux                              |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port                         |
+-----------+------------+-------------------------------------+

The Zephyr imx95_evk_a55 board configuration supports the following hardware features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| GIC-v4    | on-chip    | interrupt controller                |
+-----------+------------+-------------------------------------+
| ARM TIMER | on-chip    | system clock                        |
+-----------+------------+-------------------------------------+
| CLOCK     | on-chip    | clock_control                       |
+-----------+------------+-------------------------------------+
| PINMUX    | on-chip    | pinmux                              |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port                         |
+-----------+------------+-------------------------------------+

System Clock
------------

This board configuration uses a system clock frequency of 24 MHz for Cortex-A55.
Cortex-A55 Core runs up to 1.8 GHz.
Cortex-M7 Core runs up to 800MHz in which SYSTICK runs on same frequency.

Serial Port
-----------

This board configuration uses a single serial communication channel with the
CPU's UART1 for Cortex-A55, UART3 for Cortex-M7.

Programming and Debugging (A55)
*******************************

Use this configuration to run basic Zephyr applications and kernel tests,
for example, with the :zephyr:code-sample:`synchronization` sample:

1. Build and run the Non-SMP application

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: imx95_evk/mimx9596/a55
   :goals: run

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
    thread_a: Hello World from cpu 0 on imx95_evk!
    thread_b: Hello World from cpu 0 on imx95_evk!
    thread_a: Hello World from cpu 0 on imx95_evk!
    thread_b: Hello World from cpu 0 on imx95_evk!
    thread_a: Hello World from cpu 0 on imx95_evk!

2. Build and run the SMP application

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: imx95_evk/mimx9596/a55/smp
   :goals: run

This will build an image (zephyr.bin) with the synchronization sample app.

Copy the compiled ``zephyr.bin`` to the first FAT partition of the SD card and
plug the SD card into the board. Power it up and stop the u-boot execution at
prompt.

Use the following command to kick zephyr.bin to Cortex-A55 Core0:

.. code-block:: console

    fatload mmc 1:1 0xd0000000 zephyr.bin; dcache flush; icache flush; go 0xd0000000


It will display the following console output:
.. code-block:: console

    *** Booting Zephyr OS build v3.7.0-rc3-15-g2f0beaea144a ***
    Secondary CPU core 1 (MPID:0x100) is up
    Secondary CPU core 2 (MPID:0x200) is up
    Secondary CPU core 3 (MPID:0x300) is up
    Secondary CPU core 4 (MPID:0x400) is up
    Secondary CPU core 5 (MPID:0x500) is up
    thread_a: Hello World from cpu 0 on imx95_evk!
    thread_b: Hello World from cpu 4 on imx95_evk!
    thread_a: Hello World from cpu 0 on imx95_evk!
    thread_b: Hello World from cpu 3 on imx95_evk!
    thread_a: Hello World from cpu 0 on imx95_evk!
    thread_b: Hello World from cpu 1 on imx95_evk!
    thread_a: Hello World from cpu 0 on imx95_evk!
    thread_b: Hello World from cpu 5 on imx95_evk!
    thread_a: Hello World from cpu 0 on imx95_evk!
    thread_b: Hello World from cpu 2 on imx95_evk!

Programming and Debugging (M7)
******************************

The i.MX System Manager (SM) is used on i.MX95, which is an application that runs on
Cortex-M33 processor. The Cortex-M33 is the boot core, runs the boot ROM which loads
the SM (and other boot code), and then branches to the SM. The SM then configures some
aspects of the hardware such as isolation mechanisms and then starts other cores in the
system. After starting these cores, it enters a service mode where it provides access
to clocking, power, sensor, and pin control via a client RPC API based on ARM's
`System Control and Management Interface (SCMI)`_.

To program M7, an i.MX container image ``flash.bin`` must be made, which contains
multiple elements required, like ELE+V2X firmware, System Manager, TCM OEI, Cortex-M7
image and so on.

The steps making flash.bin and programming should refer to ``Getting Started with
MCUXpresso SDK for IMX95LPD5EVK-19.pdf`` in i.MX95 `MCUX SDK release`_.

See ``4.2 Run an example application``, just rename ``zephyr.bin`` to ``m7_image.bin``
to make flash.bin and program to SD/eMMC.

Here is an example for the :ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: imx95_evk/mimx9596/m7
   :goals: build

After making flash.bin and program to SD/eMMC, open a serial terminal, reset the board,
and you should see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v3.6.0-4569-g483c01ca11a7 ***
   Hello World! imx95_evk/mimx9596/m7

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
