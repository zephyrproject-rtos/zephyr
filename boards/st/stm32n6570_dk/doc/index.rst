.. zephyr:board:: stm32n6570_dk

Overview
********

The STM32N6570_DK Discovery kit is a complete demonstration and development platform
for the Arm® Cortex®‑M55 core‑based STM32N657X0H3Q microcontroller.

The STM32N6570_DK Discovery kit includes a full range of hardware features that help
the user evaluate many peripherals, such as USB Type-C®, Octo‑SPI flash memory and
Hexadeca‑SPI PSRAM devices, Ethernet, camera module, LCD, microSD™, audio codec,
digital microphones, ADC, flexible extension connectors, and user button.
The four flexible extension connectors feature easy and unlimited expansion capabilities
for specific applications such as wireless connectivity, analog applications, and sensors.

The STM32N657X0H3Q microcontroller features one USB 2.0 high‑speed/full‑speed
Device/Host/OTG controller, one USB 2.0 high‑speed/full‑speed Device/Host/OTG controller
with UCPD (USB Type-C® Power Delivery), one Ethernet with TSN (time-sensitive networking),
four I2Cs, two I3Cs, six SPIs (of which four I2S‑capable), two SAIs, with four DMIC support,
five USARTs, five UARTs (ISO78916 interface, LIN, IrDA, up to 12.5 Mbit/s), one LPUART,
two SDMMCs (MMC version 4.0, CE-ATA version 1.0, and SD version 1.0.1), three CAN FD
with TTCAN capability, JTAG and SWD debugging support, and Embedded Trace Macrocell™ (ETM).

The STM32N6570_DK Discovery kit integrates an STLINK-V3EC embedded in-circuit debugger and
programmer for the STM32 MCU, with a USB Virtual COM port bridge and the comprehensive MCU Package.

Hardware
********

- STM32N657X0H3Q Arm® Cortex®‑M55‑based microcontroller featuring ST Neural-ART Accelerator,
  H264 encoder, NeoChrom 2.5D GPU, and 4.2 Mbytes of contiguous SRAM, in a VFBGA264 package
- 5" LCD module with capacitive touch panel
- USB Type-C® with USB 2.0 HS interface, dual‑role‑power (DRP)
- USB Type-A with USB 2.0 HS interface, host, 0.5 A max
- 1‑Gbit Ethernet with TSN (time-sensitive networking) compliant with IEEE‑802.3‑2002
- SAI audio codec
- One MEMS digital microphone
- 1‑Gbit Octo‑SPI flash memory
- 256-Mbit Hexadeca‑SPI PSRAM
- Two user LEDs
- User, tamper, and reset push-buttons
- Board connectors:

  - USB Type-C®
  - USB Type-A
  - Ethernet RJ45
  - Camera module
  - microSD™ card
  - LCD
  - Stereo headset jack including analog microphone input
  - Audio MEMS daughterboard expansion connector
  - ARDUINO® Uno R3 expansion connector
  - STMod+ expansion connector

- On-board STLINK-V3EC debugger/programmer with USB re-enumeration capability:
  Virtual COM port, and debug port

For more details, please refer to:

* `STM32N6570_DK website`_
* `STM32N657X0 on www.st.com`_
* `STM32N657 reference manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Video
=====

STM32N6570-DK features a CSI camera module with a high-resolution 5‑Mpx CMOS RGB image sensor.
This camera outputs images in RAW Bayer format which require signal processing to be displayed with
real life colors. This Image Signal Processing could be done with a dedicated `STM32 ISP module`_.

USB
===

The USB pin assignments on the STM32N657XX microcontroller are immutable. This means that the specific
pins designated for USB functionality are fixed and cannot be changed or reassigned to other functions,
ensuring consistent and reliable USB communication.

USB PIN (IOs)
=============

+------------------+--------------------------------------+
| Name             | Description                          |
+==================+======================================+
| OTG1_HSDM        | USB OTG1 High-Speed Data- (negative) |
+------------------+--------------------------------------+
| OTG1_HSDP        | USB OTG1 High-Speed Data+ (positive) |
+------------------+--------------------------------------+
| OTG1_ID          | USB OTG1 ID Pin                      |
+------------------+--------------------------------------+
| OTG1_TXRTUNE     | USB OTG1 Transmit Retune             |
+------------------+--------------------------------------+
| OTG2_HSDM        | USB OTG2 High-Speed Data- (negative) |
+------------------+--------------------------------------+
| OTG2_HSDP        | USB OTG2 High-Speed Data+ (positive) |
+------------------+--------------------------------------+
| OTG2_ID          | USB OTG2 ID Pin                      |
+------------------+--------------------------------------+
| OTG2_TXRTUNE     | USB OTG2 Transmit Retune             |
+------------------+--------------------------------------+

Connections and IOs
===================

STM32N6570_DK Board has 12 GPIO controllers. These controllers are responsible
for pin muxing, input/output, pull-up, etc.

For more details please refer to `STM32N6570_DK User Manual`_.

Default Zephyr Peripheral Mapping:
----------------------------------

- ADC1_INP10 : PA9
- ADC1_INP11 : PA10
- FDCAN1_TX : PH2
- FDCAN1_RX : PD0
- I2C1_SCL : PH9
- I2C1_SDA : PC1
- I2C4_SCL : PE13
- I2C4_SDA : PE14
- LD1 : PO1
- LD2 : PG10
- SDMMC2_CK : PC2
- SDMMC2_CMD : PC3
- SDMMC2_D0 : PC4
- SDMMC2_D1 : PC5
- SDMMC2_D2 : PC0
- SDMMC2_D3 : PE4
- SPI5_SCK : PE15
- SPI5_MOSI : PG2
- SPI5_MISO : PH8
- SPI5_NSS : PA3
- USART_1_TX : PE5
- USART_1_RX : PE6
- USART_2_TX : PD5
- USART_2_RX : PF6
- XSPI1_NCS1 : PO0
- XSPI1_DQS0 : PO2
- XSPI1_DQS1 : PO3
- XSPI1_CLK : PO4
- XSPI1_IO0 : PP0
- XSPI1_IO1 : PP1
- XSPI1_IO2 : PP2
- XSPI1_IO3 : PP3
- XSPI1_IO4 : PP4
- XSPI1_IO5 : PP5
- XSPI1_IO6 : PP6
- XSPI1_IO7 : PP7
- XSPI1_IO8 : PP8
- XSPI1_IO9 : PP9
- XSPI1_IO10 : PP10
- XSPI1_IO11 : PP11
- XSPI1_IO12 : PP12
- XSPI1_IO13 : PP13
- XSPI1_IO14 : PP14
- XSPI1_IO15 : PP15
- XSPI2_NCS1 : PN1
- XSPI2_DQS0 : PN0
- XSPI2_CLK : PN6
- XSPI2_IO0 : PN2
- XSPI2_IO1 : PN3
- XSPI2_IO2 : PN4
- XSPI2_IO3 : PN5
- XSPI2_IO4 : PN8
- XSPI2_IO5 : PN9
- XSPI2_IO6 : PN10
- XSPI2_IO7 : PN11

System Clock
------------

STM32N6570_DK System Clock could be driven by internal or external oscillator,
as well as main PLL clock. By default System clock is driven by PLL clock at
400MHz, driven by 64MHz high speed internal oscillator.

Serial Port
-----------

STM32N6570_DK board has 10 U(S)ARTs. The Zephyr console output is assigned to
USART1. Default settings are 115200 8N1.

Board variants
**************

Three variants are available with STM32N6570_DK:

- Default variant. Available as a chainloaded application which should be loaded by a
  bootloader, it has access to the whole AXISRAM1 and AXISRAM2 regions. It is expected to
  be built using ``--sysbuild`` option exclusively.
- ``fsbl``: First Stage Boot Loader (FSBL) which is available as an application loaded by the
  Boot ROM and flashed using ST-Link. This is typically a bootloader image. It runs
  in RAM LOAD mode on second half of AXISRAM2. 511K are available for the whole image.
- ``sb``: First Stage Boot Loader - Serial Boot. Equivalent to the FSBL image, but could be
  loaded using USB and doesn't require switching the bootpins. This is the most practical
  for developments steps.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

STM32N6570_DK board includes an ST-LINK/V3 embedded debug tool interface.
This probe allows to flash and debug the board using various tools.


Flashing or loading
===================

The board is configured to be programmed using west `STM32CubeProgrammer`_ runner,
so its :ref:`installation <stm32cubeprog-flash-host-tools>` is needed.
Version 2.18.0 or later of `STM32CubeProgrammer`_ is required.

.. note::
   Firmware is run in secure mode of execution, which requires a signature.
   After build, the build system  will automatically generate a signed version of the
   binary using `STM32CubeProgrammer`_ utility ``STM32_SigningTool_CLI``.
   This utility is installed along with `STM32CubeProgrammer`_, but make sure it is
   available in your ``PATH`` variable.

To program the board, there are two options:

- Program the firmware in external flash. At boot, it will then be loaded on RAM
  and executed from there.
- Optionally, it can also be taken advantage from the serial boot interface provided
  by the boot ROM. In that case, firmware is directly loaded in RAM and executed from
  there. It is not retained.

Programming an application to STM32N6570_DK
-------------------------------------------

Here is an example to build and run :zephyr:code-sample:`hello_world` application.

First, connect the STM32N6570_DK to your host computer using the ST-Link USB port.

   .. tabs::

      .. group-tab:: Application image

         Build and flash an application loaded by MCUBoot.

         .. zephyr-app-commands::
            :zephyr-app: samples/hello_world
            :board: stm32n6570_dk
            :west-args: --sysbuild
            :goals: build flash

         .. note::
             By default, application runs in XIP mode. To use RAMLOAD mode, build
	     using the following command instead:

                      .. zephyr-app-commands::
                         :zephyr-app: samples/hello_world
                         :board: stm32n6570_dk
                         :west-args: --sysbuild -- -DCONFIG_XIP=n -DSB_CONFIG_MCUBOOT_MODE_RAM_LOAD=y
                         :goals: build flash

      .. group-tab:: FSBL - ST-Link

         Build and flash an application using ``stm32n6570_dk/stm32n657xx/fsbl`` target.

         .. zephyr-app-commands::
            :zephyr-app: samples/hello_world
            :board: stm32n6570_dk//fsbl
            :goals: build flash

         .. note::
            For flashing, before powering the board, set the boot pins in the following configuration:

            * BOOT0: 0
            * BOOT1: 1

            After flashing, to run the application, set the boot pins in the following configuration:

            * BOOT1: 0

	    Power off and on the board again.

      .. group-tab:: FSBL - Serial Boot Loader (USB)

         Additionally, connect the STM32N6570_DK to your host computer using the USB port.
         In this configuration, ST-Link is used to power the board and for serial communication
         over the Virtual COM Port.

         .. note::
            Before powering the board, set the boot pins in the following configuration:

            * BOOT0: 1
            * BOOT1: 0

         Build and load an application using ``stm32n6570_dk/stm32n657xx/sb`` target (you
         can also use the shortened form: ``stm32n6570_dk//sb``)

         .. zephyr-app-commands::
            :zephyr-app: samples/hello_world
            :board: stm32n6570_dk//sb
            :goals: build flash


Run a serial host program to connect with your Disco board:

.. code-block:: console

   $ minicom -D /dev/ttyACM0

You should see the following message on the console:

.. code-block:: console

   Hello World! stm32n6570_dk/stm32n657xx


Debugging
=========

You can debug an application in the usual way using the :ref:`ST-LINK GDB Server <runner_stlink_gdbserver>`.
Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: stm32n6570_dk
   :maybe-skip-config:
   :goals: debug

.. note::
   To enable debugging, before powering on the board, set the boot pins in the following configuration:

   * BOOT0: 0
   * BOOT1: 1

Another solution for debugging is to use STM32CubeIDE:

* Go to :menuselection:`File --> Import` and select :menuselection:`C/C++ --> STM32 Cortex-M Executable`.
* In the :guilabel:`Executable` field, browse to your ``<ZEPHYR_PATH>/build/zephyr/zephyr.elf``.
* In :guilabel:`MCU` field, select ``STM32N657X0HxQ``.
* Click on :guilabel:`Finish`.
* Finally, click on :guilabel:`Debug` to start the debugging session.

Running tests with twister
==========================

Due to the BOOT switches manipulation required when flashing the board using ``stm32n6570_dk``
board target, it is only possible to run twister tests campaign on ``stm32n6570_dk/stm32n657xx/sb``
board target which doesn't require BOOT pins changes to load and execute binaries.
To do so, it is advised to use Twister's hardware map feature with the following settings:

.. code-block:: yaml

   - platform: stm32n6570_dk/stm32n657xx/sb
     product: BOOT-SERIAL
     pre_script: <path_to_zephyr>/boards/st/common/scripts/board_power_reset.sh
     runner: stm32cubeprogrammer

.. _STM32N6570_DK website:
   https://www.st.com/en/evaluation-tools/stm32n6570-dk.html

.. _STM32N6570_DK User Manual:
   https://www.st.com/resource/en/user_manual/um3300-discovery-kit-with-stm32n657x0-mcu-stmicroelectronics.pdf

.. _STM32N657X0 on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32n657x0.html

.. _STM32N657 reference manual:
   https://www.st.com/resource/en/reference_manual/rm0486-stm32n647657xx-armbased-32bit-mcus-stmicroelectronics.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html

.. _STM32 ISP module:
   https://github.com/stm32-hotspot/zephyr-stm32-mw-isp
