.. zephyr:board:: ev48h83a

Overview
********

The **EV48H83A** evaluation board supports the **MEC1753B**, a highly configurable embedded
keyboard controller that accommodates a broad range of peripheral functions. The MEC1753B is a
mixed-signal advanced I/O controller provided in a 144-pin WFBGA package, integrating a 32-bit
ARM® Cortex-M4F processor with closely-coupled memory for optimal code execution and data access.

The board is primarily intended for use with the **Intel Meteor Lake Reference Validation Platform
(RVP)** and serves as a platform for the development of BIOS, drivers, firmware, and related
software. It can also be used as a standalone embedded controller development board for embedded
and industrial computing applications.

The EV48H83A is equipped with an Intel-specified **240-Pin Expansion Connector** and MS Windows
keyboard connectors, enabling direct connection to the Intel Meteor Lake RVP. A JTAG header, a
QSPI socket (allowing users to select their preferred QSPI memory), and configurable resistor
options are provided to enable various MEC1753B features.

Hardware
********

- ARM® Cortex-M4F processor, programmable up to 48 MHz (96 MHz internal PLL)
- 480 KB SRAM — 416 KB optimised for code, 64 KB optimised for data
- 128 bytes battery-backed SRAM (VBAT domain)
- 224 KB Boot ROM with runtime cryptographic APIs
- 8 KB internal EEPROM (selected variants)
- 512 KB internal SPI flash
- 4 Kbit in-circuit-programmable OTP with lockable 32-byte boundaries
- Enhanced Serial Peripheral Interface (eSPI) host interface — up to 66 MHz
- 5 I²C/SMBus controllers with 16 configurable ports (full crossbar)
- 3 UART channels (UART0–UART3), NS16C550A compatible, up to 1.5 Mbps
- 18 × 8 multiplexed keyboard scan matrix
- 10-bit / 12-bit ADC, up to 16 channels, 0.625 µs conversion time
- 16-channel internal DMA controller with hardware CRC-32
- 139 programmable PWM outputs
- 4 fan tachometer inputs, 2 RPM-based fan speed controllers
- 4 breathing/blinking LED outputs with piecewise-linear waveform control
- 2 PS/2 controllers (3 ports, 5 V tolerant)
- PECI Interface 3.1 (Intel low-voltage)
- PROCHOT interface with two PowerGuard instances
- Real-Time Clock (VBAT-powered), Hibernation Timer, Week Timer
- VBAT-Powered Control Interface (VCI) — 4 active-low inputs, 1 active-high output
- Two 32-bit and two 16-bit auto-reloading timers, RTOS timer, WDT
- True Random Number Generator (TRNG)
- AES-256, ECDSA/EC_KCDSA, SHA-512, RSA (1024–4096 bit), ECC (up to 571 bit) crypto engines
- Post-quantum cryptography: ECDSA-384, LMS (SHA256), ML-DSA-87 (Dilithium-5)
- Optional Physically Unclonable Function (PUF), 2 KB reserved
- 2-pin SWD, 4-pin JTAG, 1-pin ITM, Trace FIFO Debug Port (TFDP)
- Port 80 BIOS Debug Port (dual, 24-bit timestamp)
- Operating voltage: 3.3 V and 1.8 V; Temperature: −40 °C to 85 °C

For more information about the SoC please see `MEC1753B Product Page`_.

Board Components
================

- **MEC1753B** — ARM® Cortex-M4F embedded keyboard controller, 144-pin WFBGA
- **GD25LR512MFFIR** — Gigadevice 512 Mbit QSPI memory (socketed, 1.8 V capable)
- **MCP2200** — Microchip USB-to-UART interface for host communication with the MEC1753B
- **MCP1826S / 3.3 V LDO (VR2)** — Generates 3.3 V from USB 5.0 V input
- **MCP1826S / 1.8 V LDO (VR3)** — Generates 1.8 V from the 3.3 V LDO output
- P1 — Micro USB connector: board power and USB-to-UART communications
- P2 — 240-Pin Expansion Connector (backside mounted): Intel-defined primary interface to Meteor Lake RVP
- J1 — 20-pin JTAG header for MEC1753B programming and debugging
- J2 — 8-pin Dediprog SF100 header for QSPI flash programming
- J3 — 18-pin KSI keyboard FFC/FPC connector (Intel-specified, RVP interface)
- J4 — 6-pin Software Developer Interface header
- J6 — eSPI probe header for monitoring the eSPI bus
- J7 — 8-pin KSO keyboard FFC/FPC connector (Intel-specified, RVP interface)
- J10 — Power Sequence header for observing power ramp-up signal timing
- S1 — Reset switch; S2 — Power button switch
- LED1 — EC-CORE power indicator; LED2 — GPIO153; LED3 — GPIO157


Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

This evaluation board kit is comprised of the following HW blocks:

- EV48H83A MEC1753B Evaluation Board `EV48H83A User Guide`_

**Meteor Lake RVP Connectors**

- **P2 — 240-Pin Expansion Connector** (backside mounted): Intel-defined primary interface
  between the MEC1753B and the Meteor Lake RVP.
- **J3 — KSI Keyboard Connector**: 18-pin FFC/FPC (Intel-specified). Direct connection from
  MEC1753B to the Windows keyboard interface on the RVP.
- **J7 — KSO Keyboard Connector**: 8-pin FFC/FPC (Intel-specified). Direct connection from
  MEC1753B to the Windows keyboard interface on the RVP.

**Microchip / Development Connectors**

- **P1 — Micro USB Connector**: Supplies board power and interfaces with the MCP2200
  USB-to-UART converter for serial communication with the MEC1753B.
- **J1 — JTAG Connector**: 20-pin header. Configuration and programming interface for the
  MEC1753B, compatible with MPLAB® X IDE / IPE, ICD5, PICkit™ 5, and Keil ULINK2.
- **J2 — SF100 Dediprog Header**: 8-pin header. Used to flash firmware images into the
  onboard QSPI memory via a Dediprog SF100 programmer.
- **J4 — Software Developer Interface**: 6-pin header for board feature setup and validation.
- **J6 — eSPI Probe Header**: Enables monitoring of the eSPI bus between the MEC1753B and
  the host platform.
- **J10 — Power Sequence Header**: Allows observation of relative signal timing during the
  board power ramp-up sequence.

**Power Domains**

- **VR2 (3.3 V LDO)**: Generated from USB 5.0 V input by the MCP1826S.
- **VR3 (1.8 V LDO)**: Generated from the VR2 3.3 V rail by the MCP1826S.
- **VTR2**: Selectable 1.8 V or 3.3 V via jumper J26; powers the QSPI memory socket.

System Clock
============

The MEC1753B contains a 96 MHz internal PLL and supports three 32 kHz clock sources:

- Internal 32 kHz silicon oscillator (default)
- External parallel 32 kHz crystal (XTAL)
- External single-ended 32 kHz clock input

The EC core runs at up to 48 MHz (programmable). The RTOS timer and battery-backed peripherals
run off the 32 kHz domain continuously, regardless of processor sleep state. Firmware selects
the 32 kHz source via the clock enable register in the Power, Clocks and Resets (PCR) block.

Serial Port
===========

``UART0`` is configured as the default console for serial logs. UART1–UART3 are also available
and can be routed to various pin configurations via jumper settings. Standard baud rates up to
115.2 Kbps and custom baud rates up to 1.5 Mbps are supported.

Jumper Settings
***************

Please follow the jumper settings below to properly configure this board.
Advanced users may deviate from this recommendation.

Jumper settings for EV48H83A Rev A
====================================

Power-Related Jumpers
---------------------

If you wish to power from the Micro USB connector ``P1``, set the jumper ``J22 1-2``.
This is required for standalone mode.
If you wish to power through the 240-pin expansion connector ``P2`` and mate to an external
platform, set the jumper to ``J22 2-3``.

NOTE: A single jumper is required in J22.

If you wish to set VTR2 to 1.8 V (required for the default GD25LR512MFFIR QSPI device),
set the jumper ``J26 1-2``.
If you wish to set VTR2 to 3.3 V, set the jumper ``J26 2-3``.

.. warning::
   If the socketed QSPI memory device is replaced, validate the operating voltage of the
   selected device and set J26 accordingly **before** powering the board. Incorrect voltage
   selection may damage the memory device.

Boot-ROM Straps
---------------

This jumper configures the MEC1753B Crisis Recovery strap.

+----------------------------+
| J23 (Crisis Recovery)      |
+============================+
| Open (default) — disabled  |
+----------------------------+
| Closed — enabled           |
+----------------------------+

``J23 Closed`` enables crisis recovery mode. When closed, the MEC1753B Boot-ROM enters
Crisis Recovery via the UART interface.

Boot Source Select
------------------

The jumpers below configure the MEC1753B boot source and GPIO routing.

+------------------------------------------+-------------+
| Configuration                            | J25         |
+==========================================+=============+
| GPIO054 → PM_RSMRST (default)            | 1-2 Closed  |
+------------------------------------------+-------------+
| GPIO054 → EC_SPI_FLASH_CS_L (default)    | 3-4 Open    |
+------------------------------------------+-------------+
| GPIO055 → EC_SPI_FLASH_CS_L (default)    | 5-6 Closed  |
+------------------------------------------+-------------+

Bootstrap mode (BSS Strap) is controlled by ``J24``:

- **Open (default)**: Bootstrap mode disabled
- **Closed**: Bootstrap mode enabled


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Setup
=====

#. If you use Dediprog SF100 programmer, set it up first.

   Windows version can be found at the `SF100 Product page`_.

   Linux version source code can be found at `SF100 Linux GitHub`_.
   Follow the `SF100 Linux manual`_ to complete setup of the SF100 programmer.
   For Linux please make sure that you copied ``60-dediprog.rules``
   from the ``SF100Linux`` folder to the :code:`/etc/udev/rules.d`
   then restart the service using:

   .. code-block:: console

      $ udevadm control --reload

   Add the directory containing ``dpcmd`` (on Linux)
   or ``dpcmd.exe`` (on Windows) to your ``PATH``.

#. Use Microchip's PC-based **image generation tool** (compatible with Windows x64 and
   Linux x64) to produce a customer SPI image.

#. Clone the `MEC175x SPI Image Gen`_ repository or download the files within that directory.

#. Make the image generation tool available for Zephyr by making it searchable by path,
   for example:

   .. code-block:: console

      -DMEC175X_SPI_GEN=<path to spi_gen tool>/mec175x_spi_gen_lin_x86_64

   Note that the tools for Linux and Windows have different file names.

#. The default MEC175X_SPI_CFG file is ``spi_cfg.txt`` located in ``${BOARD_DIR}/support``.
   If needed, a custom SPI image configuration file can be specified:

   .. code-block:: console

      -DMEC175X_SPI_CFG=<path to spi_cfg file>/spi_cfg.txt

#. Example command to build a hello_world image:

   .. code-block:: console

      west build -p auto -b ev48h83a samples/hello_world -- \
        -DMEC175X_SPI_GEN=$HOME/CPGZephyrDocs/MEC175x/SPI_image_gen/mec175x_spi_gen_lin_x86_64 \
        -DMEC175X_SPI_CFG=$HOME/zephyrproject/zephyr/boards/microchip/ev48h83a/support/spi_cfg.txt

Wiring
======

#. Connect the Dediprog SF100 programmer to header **J2** on the EV48H83A board to flash the
   QSPI NOR memory. Make sure that your programmer's offset is ``0x0``.

   +------------+---------------+
   |  Dediprog  |               |
   |  Connector |      J2       |
   +============+===============+
   |    VCC     |       1       |
   +------------+---------------+
   |    GND     |       2       |
   +------------+---------------+
   |    CS      |       3       |
   +------------+---------------+
   |    CLK     |       4       |
   +------------+---------------+
   |    MISO    |       6       |
   +------------+---------------+
   |    MOSI    |       5       |
   +------------+---------------+

#. Connect ``P1`` Micro USB to your host computer for serial (UART0) communication
   via the onboard MCP2200 USB-to-UART converter.

#. Apply power to the board via the Micro USB cable.
   Configure this option by setting jumper ``J22 1-2``.

#. After flashing, press the Reset switch **S1** to restart the MEC1753B from the new image.

Building
========

#. Build :zephyr:code-sample:`hello_world` application as you would normally do.

#. The file :file:`spi_image.bin` will be created if the build system can find the image
   generation tool. This binary image can be used to flash the QSPI device.

Flashing
========

#. Run your favorite terminal program to listen for output.
   Under Linux the terminal should be :code:`/dev/ttyUSB0`. Do not close it.

   For example:

   .. code-block:: console

      $ minicom -D /dev/ttyUSB0 -o

   The ``-o`` option tells minicom not to send the modem initialization string.
   Connection should be configured as follows:

   - Speed: 115200
   - Data: 8 bits
   - Parity: None
   - Stop bits: 1

#. Flash your board using ``west`` from the second terminal window:

   .. code-block:: console

      $ west flash

   .. note:: When the west process starts, press the Reset button ``S1`` and hold it until
    the west process completes successfully.

#. You should see ``"Hello World! ev48h83a"`` in the first terminal window.
   If you don't see this message, press the Reset button and the message should appear.

OTP Configuration
=================

.. warning::
   OTP registers **cannot** be programmed unless a bootable image is already installed in QSPI.
   OTP bits can only be set once — once programmed, the configuration cannot be altered.

#. Use Microchip's **OTP Generator Tool** to define the configuration image based on your
   application requirements.

#. Connect a supported debugger to the **J1** 20-pin JTAG header:

   - Microchip DV164055 ICD5 In-Circuit Debugger
   - Microchip PG164150 MPLAB® PICkit™ 5
   - Microchip AC102015 Target Adapter Board
   - Keil ULINK2 Debug Adapter

#. Program the OTP registers using **MPLAB® X IDE** (version 6.20 or higher) or MPLAB IPE.

Debugging
=========

``J1`` header on the board allows for full JTAG access for boundary scan, OTP programming,
and step-debug via ICD5, PICkit™ 5, or ULINK2.

Troubleshooting
===============

#. In case you don't see your application running, please make sure ``LED1`` is lit.
   If ``LED1`` is off, verify jumper ``J22`` (power source selection) and check that the
   board fuse (F1) is intact.

#. If you can't program the board using Dediprog, disconnect and reconnect the cable
   connected to ``P1`` and try again.

#. If Dediprog can't detect the onboard flash, press the board's ``S1`` Reset button
   and try again.

#. A factory pre-loaded image is installed on the board that blinks ``LED2`` and ``LED3``.
   This confirms the MEC1753B boots correctly before any custom firmware is programmed.


References
**********

.. target-notes::

.. _MEC1753B Product Page:
    https://www.microchip.com/en-us/product/MEC1753B
.. _EV48H83A User Guide:
    https://www.microchip.com/en-us/development-tool/EV48H83A
.. _MEC175x SPI Image Gen:
    https://github.com/MicrochipTech/CPGZephyrDocs/tree/master/MEC175x/SPI_image_gen
.. _SF100 Linux GitHub:
    https://github.com/DediProgSW/SF100Linux
.. _SF100 Product page:
    https://www.dediprog.com/product/SF100
.. _SF100 Linux manual:
    https://www.dediprog.com/download/save/727.pdf