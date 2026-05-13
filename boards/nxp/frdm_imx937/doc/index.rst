.. zephyr:board:: frdm_imx937

Overview
********

The FRDM-IMX937 board is a platform designed to show the
most commonly used features of the i.MX 937 automotive applications processor.
It is an entry-level development board, which helps developers to get familiar
with the processor before investing a large amount of resources in more
specific designs. The i.MX937 device on the board comes in a compact
15 x 15 mm package.

Hardware
********

- i.MX 937 automotive applications processor

   - The processor integrates up to four Arm Cortex-A55 cores, and supports
     functional safety with built-in Arm Cortex-M33 and Cortex-M7 cores

- DRAM memory: 8 GB LPDDR5 (Micron MT62F2G32D4DS-020 WT)
- Mass storage:

   - 32 GB eMMC5.1 (Sandisk SDINBDV4-32G)
   - MicroSD card connector (SD3.0 supported)

- USB interface: Two USB controllers and PHYs: USB1 and USB2

   - 1x USB Type-C port connected to USB1 controller (supports USB 2.0 standard)
   - 1x USB Type-A port connected to USB2 controller (supports USB 2.0 standard)

- Ethernet interface

   - Two ENET controllers
   - Two tri-speed (10 Mbit/s / 100 Mbit/s / 1000 Mbit/s) RGMII interfaces
     supported through two YT8521S Ethernet PHYs
   - Two RJ45 connectors with link and activity status indicators

- MIPI CSI interface: MIPI-CSI controller connects to two 22-pin FPC
   connectors, configurable as 1x4 lane mode or 2x2 lane mode

- MIPI DSI interface: MIPI-DSI controller connects to a 22-pin FPC
   connector using a x4 lane configuration

- LVDS interface: LVDS TX display controller connects to a 40-pin
   connector using a x4 lane configuration

- HDMI interface: LVDS TX display controller connects to an HDMI
   connector through a x4 lane LVDS to HDMI converter chip (IT6263)

- CAN interface: CAN controller with 1x high-speed CAN transceiver
   connection using TJA1051GT/3

- I/O expander: CAN, I2C, analog-to-digital converter (ADC)

   - One 10-pin (2x5), 2.54 mm connector provides:
   - One high-speed CAN transceiver interface using TJA1051GT/3
   - 1x I2C interface
   - 2x ADC inputs

- M.2 interface:

   - One M.2 Key-M 75-pin connector supporting x1 PCIe interface
   - One M.2 Key-E 75-pin connector supporting PCIe, USB, SDIO, SAI,
     UART, I2C, and SPI interfaces

- Audio:

   - MQS
   - One 3.5 mm, 4-pole audio jack for connecting to a stereo speaker or
     headphones
   - PDM interface: Supports two digital microphones (DMIC), each
     providing a single-bit PDM output

- Debug interface: USB-to-UART device, CH344Q

   - One USB 2.0 Type-C connector on the CH344Q provides four COM ports:
   - One port dedicated to ARM Cortex-A55 system debug
   - One port dedicated to ARM Cortex-M33 system debug
   - One port dedicated to ARM Cortex-M7 system debug

- EXPI: One 2x20-pin expansion interface supporting SAI, UART, I2C,
   SPI, PDM and GPIO expansion

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

The i.MX System Manager (SM) is used on i.MX937, which is an application that runs on
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
MCUXpresso SDK for FRDM-IMX937.pdf`` in i.MX937 `MCUX SDK release`_. Note that
for the DDR variant, one should use the Makefile targets containing the ``ddr`` keyword.
See ``4.2 Run an example application``, just rename ``zephyr.bin`` to ``m7_image.bin``
to make flash.bin and program to SD/eMMC.

Zephyr supports two M7-based i.MX937 boards: ``frdm_imx937/mimx9375/m7`` and
``frdm_imx937/mimx9375/m7/ddr``. The main difference between them is the memory
used. ``frdm_imx937/mimx9375/m7`` uses TCM (ITCM for code and, generally, read-only
data and DTCM for R/W data), while ``frdm_imx937/mimx9375/m7/ddr`` uses DDR.

1. Building the :zephyr:code-sample:`hello_world` application for the TCM-based board

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_imx937/mimx9375/m7
   :goals: build

2. Building the :zephyr:code-sample:`hello_world` application for the DDR-based board

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_imx937/mimx9375/m7/ddr
   :goals: build

After making flash.bin and program to SD/eMMC, open a serial terminal, and reset the
board. For the ``frdm_imx937/mimx9375/m7`` board you should see something like:

.. code-block:: console

   *** Booting Zephyr OS build v3.6.0-4569-g483c01ca11a7 ***
   Hello World! frdm_imx937/mimx9375/m7

while, for the ``frdm_imx937/mimx9375/m7/ddr`` board, you should get the following output:

.. code-block:: console

   *** Booting Zephyr OS build v3.6.0-4569-g483c01ca11a7 ***
   Hello World! frdm_imx937/mimx9375/m7/ddr

.. _System Control and Management Interface (SCMI):
   https://developer.arm.com/documentation/den0056/latest/

.. _i.MX Linux BSP release:
   https://www.nxp.com/design/design-center/software/embedded-software/i-mx-software/embedded-linux-for-i-mx-applications-processors:IMXLINUX

.. _MCUX SDK release:
   https://mcuxpresso.nxp.com/

.. include:: ../../common/board-footer.rst.inc

.. _NXP website:
   https://www.nxp.com/products/processors-and-microcontrollers/arm-processors/i-mx-applications-processors/i-mx-9-processors/i-mx-95-applications-processor-family-high-performance-safety-enabled-platform-with-eiq-neutron-npu:iMX95
