.. zephyr:board:: imx95_evk

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

.. zephyr:board-supported-hw::

System Clock
------------

This board configuration uses a system clock frequency of 24 MHz for Cortex-A55.
Cortex-A55 Core runs up to 1.8 GHz.
Cortex-M7 Core runs up to 800MHz in which SYSTICK runs on same frequency.

Serial Port
-----------

This board configuration uses a single serial communication channel with the
CPU's UART1 for Cortex-A55, UART3 for Cortex-M7.

TPM
---

Two channels are enabled on TPM2 for PWM for M7. Signals can be observerd with
oscilloscope.
Channel 2 signal routed to resistance R881.
Channel 3 signal routed to resistance R882.

SPI
---

The EVK board need to be reworked to solder R1217/R1218/R1219/R1220 with 0R resistances.
SPI1 on J35 is enabled for M7.

Ethernet
--------

NETC driver supports to manage the Physical Station Interface (PSI).
The first ENET1 port could be enabled on M7 DDR and A55 platforms.

For A55 Core, NETC depends on GIC ITS, so need to make sure to allocate heap memory to
be larger than 851968 byes by setting CONFIG_HEAP_MEM_POOL_SIZE.

Programming and Debugging (A55)
*******************************

.. zephyr:board-supported-runners::

There are multiple methods to program and run Zephyr on the A55 core:

Option 1. Boot Zephyr by Using SPSDK Runner
===========================================

SPSDK runner leverages SPSDK tools (https://spsdk.readthedocs.io), it builds an
bootable flash image ``flash.bin`` which includes all necessary firmware components,
such as ELE+V2X firmware, System Manager, TCM OEI, TF-A images etc. Using west flash
command will download the boot image flash.bin to DDR memory, SD card or eMMC flash.
By using flash.bin, as no U-Boot image is available, so TF-A will boot up Zephyr on
the first Cortex-A55 Core directly.

In order to use SPSDK runner, it requires fetching binary blobs, which can be achieved
by running the following command:

.. code-block:: console

   west blobs fetch hal_nxp

.. note::

   It is recommended running the command above after :file:`west update`.

SPSDK runner is enabled by configure item :kconfig:option:`CONFIG_BOARD_NXP_SPSDK_IMAGE`, currently
it is not enabled by default for i.MX95 EVK board, so use this configuration to enable
it, for example, with the :zephyr:code-sample:`synchronization` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: imx95_evk/mimx9596/a55
   :goals: build
   :gen-args: -DCONFIG_BOARD_NXP_SPSDK_IMAGE=y

If :kconfig:option:`CONFIG_BOARD_NXP_SPSDK_IMAGE` is available and enabled for the board variant,
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
   $ west flash -r spsdk

   # program to SD card, then set SW7[1:4]=0b1011 to reboot
   $ west flash -r spsdk --bootdevice sd

   # program to emmc card, then set SW7[1:4]=0b1010 to reboot
   $ west flash -r spsdk --bootdevice=emmc


Option 2. Boot Zephyr by Using U-Boot Command
=============================================

U-Boot "go" command can be used to start Zephyr on A55 core0 and U-Boot "cpu" command
is used to load and kick Zephyr to the other A55 secondary Cores. Currently "cpu" command
is supported in : `Real-Time Edge U-Boot`_ (use the branch "uboot_vxxxx.xx-y.y.y,
xxxx.xx is uboot version and y.y.y is Real-Time Edge Software version, for example
"uboot_v2023.04-2.9.0" branch is U-Boot v2023.04 used in Real-Time Edge Software release
v2.9.0), and pre-build images and user guide can be found at `Real-Time Edge Software`_.

.. _Real-Time Edge U-Boot:
   https://github.com/nxp-real-time-edge-sw/real-time-edge-uboot
.. _Real-Time Edge Software:
   https://www.nxp.com/rtedge

Step 1: Download Zephyr Image into DDR Memory
---------------------------------------------

Firstly need to download Zephyr binary image into DDR memory, it can use tftp:

.. code-block:: console

    tftp 0xd0000000 zephyr.bin

Or copy the Zephyr image ``zephyr.bin`` SD card and plug the card into the board, for example
if copy to the FAT partition of the SD card, use the following U-Boot command to load the image
into DDR memory (assuming the SD card is dev 1, fat partition ID is 1, they could be changed
based on actual setup):

.. code-block:: console

    fatload mmc 1:1 0xd0000000 zephyr.bin;

Step 2: Boot Zephyr
-------------------

Use this configuration to run basic Zephyr applications and kernel tests,
for example, with the :zephyr:code-sample:`synchronization` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: imx95_evk/mimx9596/a55
   :goals: build

This will build an image (zephyr.bin) with the synchronization sample app.

Then use the following command to boot Zephyr on the core0:

.. code-block:: console

    dcache off; icache flush; go 0xd0000000;

Or use "cpu" command to boot from secondary Core, for example Core1:

.. code-block:: console

    dcache flush; icache flush; cpu 1 release 0xd0000000

It will display the following console output:

.. code-block:: console

    *** Booting Zephyr OS build v3.6.0-4569-g483c01ca11a7 ***
    thread_a: Hello World from cpu 0 on imx95_evk!
    thread_b: Hello World from cpu 0 on imx95_evk!
    thread_a: Hello World from cpu 0 on imx95_evk!
    thread_b: Hello World from cpu 0 on imx95_evk!
    thread_a: Hello World from cpu 0 on imx95_evk!

Option 3. Boot Zephyr by Using Remoteproc under Linux
=====================================================

When running Linux on the A55 core, it can use the remoteproc framework to load and boot Zephyr,
refer to Real-Time Edge user guide for more details. Pre-build images and user guide can be found
at `Real-Time Edge Software`_.

Option 4. Boot Zephyr by Using JLink Runner
===========================================

The board support using JLink runner to flash and debug Zephyr, connect the EVK board's JTAG connector
to the host computer using a J-Link debugger.

If run and debug Zephyr on the Core0, just power up the board and stop the board at U-Boot command line.

Then use "west flash -r jlink" command to load the zephyr.bin image from the host computer and start
Zephyr, or use "west debug -r jlink" to debug Zephyr.

Flash and Run
-------------

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: imx95_evk/mimx9596/a55
   :goals: flash
   :flash-args: -r jlink

Debug
-----

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: imx95_evk/mimx9596/a55
   :goals: debug
   :debug-args: -r jlink

Notes
-----

If run and debug Zephyr on the other Cortex-A Core except Core0, for example Core2, need to execute the
following command under U-Boot command line to release specific CPU Core to a dead loop state (U-Boot
needs to support "cpu" command, refer to "Option 2. Boot Zephyr by Using U-Boot Command" to get specific
U-Boot version).

.. code-block:: console

    mw.l 0xD0000000 14000000; dcache flush; icache flush; cpu 2 release 0xD0000000

And also need to modify jlink device by using the following update, change device ID MIMX9556_A55_0 to
specified ID, for example MIMX9556_A55_2 for CPU Core2:

.. code-block:: console

    diff --git a/boards/nxp/imx95_evk/board.cmake b/boards/nxp/imx95_evk/board.cmake
    index daca6ade79f..c58de8c2431 100644
    --- a/boards/nxp/imx95_evk/board.cmake
    +++ b/boards/nxp/imx95_evk/board.cmake
    @@ -21,6 +21,6 @@ if(CONFIG_BOARD_NXP_SPSDK_IMAGE OR (DEFINED ENV{USE_NXP_SPSDK_IMAGE}
    endif()

    if(CONFIG_SOC_MIMX9596_A55)
    -  board_runner_args(jlink "--device=MIMX9556_A55_0" "--no-reset" "--flash-sram")
    +  board_runner_args(jlink "--device=MIMX9556_A55_2" "--no-reset" "--flash-sram")
    include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
    endif()

Then use "west flash -r jlink" command to load the zephyr.bin image from the host computer and start
Zephyr, or use "west debug -r jlink" to debug Zephyr.

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

SPSDK runner is used to build ``flash.bin``, and it requires fetching binary blobs, which
can be achieved by running the following command:

.. code-block:: console

   west blobs fetch hal_nxp

.. note::

   It is recommended running the command above after :file:`west update`.

Two methods to build and program ``flash.bin``.

1. If :kconfig:option:`CONFIG_BOARD_NXP_SPSDK_IMAGE` is not available for the board variant,
the steps making flash.bin and programming should refer to ``Getting Started with
MCUXpresso SDK for IMX95LPD5EVK-19.pdf`` in i.MX95 `MCUX SDK release`_. Note that
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

Zephyr supports two M7-based i.MX95 boards: ``imx95_evk/mimx9596/m7`` and
``imx95_evk/mimx9596/m7/ddr``. The main difference between them is the memory
used. ``imx95_evk/mimx9596/m7`` uses TCM (ITCM for code and, generally, read-only
data and DTCM for R/W data), while ``imx95_evk/mimx9596/m7/ddr`` uses DDR.

1. Building the :zephyr:code-sample:`hello_world` application for the TCM-based board

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: imx95_evk/mimx9596/m7
   :goals: build

2. Building the :zephyr:code-sample:`hello_world` application for the DDR-based board

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: imx95_evk/mimx9596/m7/ddr
   :goals: build

After making flash.bin and program to SD/eMMC, open a serial terminal, and reset the
board. For the ``imx95_evk/mimx9596/m7`` board you should see something like:

.. code-block:: console

   *** Booting Zephyr OS build v3.6.0-4569-g483c01ca11a7 ***
   Hello World! imx95_evk/mimx9596/m7

while, for the ``imx95_evk/mimx9596/m7/ddr`` board, you should get the following output:

.. code-block:: console

   *** Booting Zephyr OS build v3.6.0-4569-g483c01ca11a7 ***
   Hello World! imx95_evk/mimx9596/m7/ddr

.. _System Control and Management Interface (SCMI):
   https://developer.arm.com/documentation/den0056/latest/

.. _i.MX Linux BSP release:
   https://www.nxp.com/design/design-center/software/embedded-software/i-mx-software/embedded-linux-for-i-mx-applications-processors:IMXLINUX

.. _MCUX SDK release:
   https://mcuxpresso.nxp.com/

.. include:: ../../common/board-footer.rst
   :start-after: nxp-board-footer

.. _NXP website:
   https://www.nxp.com/products/processors-and-microcontrollers/arm-processors/i-mx-applications-processors/i-mx-9-processors/i-mx-95-applications-processor-family-high-performance-safety-enabled-platform-with-eiq-neutron-npu:iMX95
