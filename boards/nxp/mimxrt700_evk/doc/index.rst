.. _mimxrt595_evk:

NXP MIMXRT595-EVK
##################

Overview
********

The new i.MX RT700 CPU architecture is composed of a high-performance main compute subsystem,
a secondary “always-on” sense-compute subsystem and specialized coprocessors.

The main compute subsystem uses a 325 MHz capable Arm® Cortex®-M33 (CM33).
Similar to the i.MX RT600 crossover MCU, the i.MX RT700 includes a Cadence Tensilica® HiFi 4 DSP.
The HiFi 4 is a high performance DSP core based upon a Very Long Instruction Word (VLIW) architecture,
which is capable of processing up to eight 32x16 MACs per instruction cycle. It can be used for offloading
high-performance numerical tasks such as audio and image processing and supports both fixed-point and
floating-point operations.

Note: Due to board isn't launched, the picture is blank
.. image:: mimxrt700_evk.webp
   :align: center
   :alt: MIMXRT700-EVK

Hardware
********

- Main Compute Subsystem:
      Arm Cortex-M33 up to 325 MHz
      HiFi 4 DSP up to 325 MHz
      eIQ Neutron NPU up to 325 MH
- Sense Compute Subsystem:
      Arm Cortex-M33 up to 250 MHz
      HiFi 1 DSP up to 250 MHz
- 7.5 MB on-chip SRAM
- Three xSPI interfaces for off-chip memory expansion, supporting up to 16b wide external memories up to 250 MHz DDR
- eUSB support with integrated PHY
- Two SD/eMMC memory card interfaces—one supporting eMMC 5.0 with HS400/DDR operation
- USB high-speed host/device controller with on-chip PHY
- A digital microphone interface supporting up to 8 channels
- Serial peripherals (UART/I²C/I3C/SPI/HSPI/SAI)
- 2.5D GPU with vector graphics acceleration and frame buffer compression
- EZH-V using RISC-V core with additional SIMD/DSP instructions
- Full openVG 1.1 support
- Upto 720p@60FPS from on-chip SRAM
- LCD Interface + MIPI DSI
- Integrated JPEG and PNG support
- CSI 8/10/16-bit parallel (via FlexIO)

For more information about the MIMXRT798 SoC and MIMXRT700-EVK board, see
these references:

Note: There are currently no available links to provide.

Supported Features
==================

NXP considers the MIMXRT700-EVK as a superset board for the i.MX RT7xx
family of MCUs.  This board is a focus for NXP's Full Platform Support for
Zephyr, to better enable the entire RT7xx family.  NXP prioritizes enabling
this board with new support for Zephyr features.  The mimxrt700_evk board
configuration supports the hardware features below.

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| SYSTICK   | on-chip    | systick                             |
+-----------+------------+-------------------------------------+
| IOCON     | on-chip    | pinmux                              |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port-polling;                |
|           |            | serial port-interrupt               |
+-----------+------------+-------------------------------------+
| CLOCK     | on-chip    | clock_control                       |
+-----------+------------+-------------------------------------+

The default configuration can be found in the defconfig file:

   :zephyr_file:`boards/nxp/mimxrt700_evk/mimxrt700_evk_mimxrt798s_cpu0_defconfig`
   :zephyr_file:`boards/nxp/mimxrt700_evk/mimxrt700_evk_mimxrt798s_cpu1_defconfig`

Other hardware features are not currently supported by the port.

Connections and IOs
===================

The MIMXRT798 SoC has IOCON registers, which can be used to configure the
functionality of a pin.

+---------+-----------------+----------------------------+
| Name    | Function        | Usage                      |
+=========+=================+============================+
| PIO0_31 | UART0           | UART RX                    |
+---------+-----------------+----------------------------+
| PIO1_0  | UART0           | UART TX                    |
+---------+-----------------+----------------------------+
| PIO0_18 | GPIO            | GREEN LED                  |
+---------+-----------------+----------------------------+
| PIO0_9  | GPIO            | SW5                        |
+---------+-----------------+----------------------------+
| PIO8_14 | UART19          | UART TX                    |
+---------+-----------------+----------------------------+
| PIO8_15 | UART19          | UART RX                    |
+---------+-----------------+----------------------------+

System Clock
============

The MIMXRT700 EVK is configured to use the Systick
as a source for the system clock.

Programming and Debugging
*************************

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Configuring a Debug Probe
=========================

A debug probe is used for both flashing and debugging the board. This board is
configured by default to use the LPC-Link2.

.. tabs::

    .. group-tab:: JLink External


        1. Install the :ref:`jlink-debug-host-tools` and make sure they are in your search path.

        2. To disconnect the SWD signals from onboard debug circuit, **install** jumpers JP24

        3. Connect the J-Link probe to J10 10-pin header.

        See :ref:`jlink-external-debug-probe` for more information.

    .. group-tab:: Linkserver

        1. Install the :ref:`linkserver-debug-host-tools` and make sure they are in your search path.
        2. To update the debug firmware, please follow the instructions on `MIMXRT700-EVK Debug Firmware`

Configuring a Console
=====================

Connect a USB cable from your PC to J54, and use the serial terminal of your choice
(minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Flashing
========

Here is an example for the :zephyr:code-sample:`hello_world` application. This example uses the
:ref:`jlink-debug-host-tools` as default.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mimxrt700_evk/mimxrt798s/cpu0
   :goals: flash

Open a serial terminal, reset the board (press the RESET button), and you should
see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS v3.7.0 ***
   Hello World! mimxrt700_evk/mimxrt798s/cpu0

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application. This example uses the
:ref:`jlink-debug-host-tools` as default.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mimxrt700_evk/mimxrt798s/cpu0
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS v3.7.0 ***
   Hello World! mimxrt700_evk/mimxrt798s/cpu0
