.. zephyr:board:: imx952_evk

Overview
********

The i.MX952 EVK board is a platform designed to show the
most commonly used features of the i.MX 952 automotive applications processor.
It is an entry-level development board, which helps developers to get familiar
with the processor before investing a large amount of resources in more
specific designs. The i.MX952 device on the board comes in a compact
19 x 19 mm/15 x 15 mm package.

Hardware
********

- i.MX 952 automotive applications processor

  - The processor integrates up to four Arm Cortex-A55 cores, and supports
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

.. zephyr:board-supported-hw::

System Clock
------------

This board configuration uses a system clock frequency of 24 MHz for Cortex-A55.
Cortex-A55 Core runs up to 1.7 GHz.
Cortex-M7 Core runs up to 800MHz in which SYSTICK runs on same frequency.

Serial Port
-----------

This board configuration uses a single serial communication channel with the
CPU's UART1 for Cortex-A55, UART3 for Cortex-M7.

Programming and Debugging (M7)
******************************

The i.MX System Manager (SM) is used on i.MX952, which is an application that runs on
Cortex-M33 processor. The Cortex-M33 is the boot core, runs the boot ROM which loads
the SM (and other boot code), and then branches to the SM. The SM then configures some
aspects of the hardware such as isolation mechanisms and then starts other cores in the
system. After starting these cores, it enters a service mode where it provides access
to clocking, power, sensor, and pin control via a client RPC API based on ARM's
`System Control and Management Interface (SCMI)`_.

To program M7, an i.MX container image ``flash.bin`` must be made, which contains
multiple elements required, like ELE+V2X firmware, System Manager, TCM OEI, Cortex-M7
image and so on.

SPSDK runner is used to build ``flash.bin``, and it requires fetching binary blobs, which
can be achieved by running the following command:

.. code-block:: console

   west blobs fetch hal_nxp

.. note::

   It is recommended running the command above after :file:`west update`.

Two methods to build and program ``flash.bin``.

1. If :kconfig:option:`CONFIG_BOARD_NXP_SPSDK_IMAGE` is not available for the board variant,
the steps making flash.bin and programming should refer to ``Getting Started with
MCUXpresso SDK for IMX952LPD5EVK-19.pdf`` in i.MX952 `MCUX SDK release`_. Note that
for the DDR variant, one should use the Makefile targets containing the ``ddr`` keyword.
See ``4.2 Run an example application``, just rename ``zephyr.bin`` to ``m7_image.bin``
to make flash.bin and program to SD/eMMC.

2. If :kconfig:option:`CONFIG_BOARD_NXP_SPSDK_IMAGE` is available and enabled for the board variant,
``flash.bin`` will be built automatically. The programming could be through below commands.
Before that, switch SW7[1:4] should be configured to 0b1001 for usb download mode
to boot, and USB1 and DBG ports should be connected to PC. There are 4 serial ports
enumerated (115200 8n1), and we use the first for M7 and the fourth for M33 System Manager.
(The flasher is spsdk which already installed via scripts/requirements.txt.
On linux host, usb device permission should be configured per Installation Guide
of https://spsdk.readthedocs.io)

.. code-block:: none

   # load and run without programming. for next flashing, execute 'reset' in the
   # fourth serail port
   $ west flash

   # program to SD card, then set SW7[1:4]=0b1011 to reboot
   $ west flash --bootdevice sd

   # program to emmc card, then set SW7[1:4]=0b1010 to reboot
   $ west flash --bootdevice=emmc

Zephyr supports two M7-based i.MX952 boards: ``imx952_evk/mimx9529/m7`` and
``imx952_evk/mimx9529/m7/ddr``. The main difference between them is the memory
used. ``imx952_evk/mimx9529/m7`` uses TCM (ITCM for code and, generally, read-only
data and DTCM for R/W data), while ``imx952_evk/mimx9529/m7/ddr`` uses DDR.

1. Building the :zephyr:code-sample:`hello_world` application for the TCM-based board

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: imx952_evk/mimx9529/m7
   :goals: build

2. Building the :zephyr:code-sample:`hello_world` application for the DDR-based board

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: imx952_evk/mimx9529/m7/ddr
   :goals: build

After making flash.bin and program to SD/eMMC, open a serial terminal, and reset the
board. For the ``imx952_evk/mimx9529/m7`` board you should see something like:

.. code-block:: console

   *** Booting Zephyr OS build v3.6.0-4569-g483c01ca11a7 ***
   Hello World! imx952_evk/mimx9529/m7

while, for the ``imx952_evk/mimx9529/m7/ddr`` board, you should get the following output:

.. code-block:: console

   *** Booting Zephyr OS build v3.6.0-4569-g483c01ca11a7 ***
   Hello World! imx952_evk/mimx9529/m7/ddr

.. _System Control and Management Interface (SCMI):
   https://developer.arm.com/documentation/den0056/latest/

.. _i.MX Linux BSP release:
   https://www.nxp.com/design/design-center/software/embedded-software/i-mx-software/embedded-linux-for-i-mx-applications-processors:IMXLINUX

.. _MCUX SDK release:
   https://mcuxpresso.nxp.com/

.. include:: ../../common/board-footer.rst.inc

.. _NXP website:
   https://www.nxp.com/products/processors-and-microcontrollers/arm-processors/i-mx-applications-processors/i-mx-9-processors/i-mx-95-applications-processor-family-high-performance-safety-enabled-platform-with-eiq-neutron-npu:iMX95
