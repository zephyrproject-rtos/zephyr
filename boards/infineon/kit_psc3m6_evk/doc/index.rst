.. zephyr:board:: kit_psc3m6_evk

Overview
********

The PSOC™ Control C3M6 Evaluation Kit (`KIT_PSC3M6_EVK`_) is an evaluation board for the
PSC3M6 microcontroller. The PSC3M6 is an Arm® Cortex®-M33 based SoC from Infineon's
CAT1B family. The PSC3M6GES3AHQ1 variant on this board features 512 KB flash, 128 KB SRAM,
and TrustZone-M security.

Hardware
********

- **SoC:** PSC3M6GES3AHQ1 (PG-E-LQFP-100)
- **CPU:** Arm® Cortex®-M33 at up to 180 MHz
- **Flash:** 512 KB internal flash
- **SRAM:** 128 KB
- **Connectivity:** CAN FD, SCB (UART/SPI/I2C)
- **Security:** TrustZone-M
- **Debug:** SEGGER J-Link (requires SEGGER J-Link version v9.68 or later)
- **User I/O:** Two user LEDs, two user buttons

Kit Contents
============

The `KIT_PSC3M6_EVAL User Guide`_ lists the following kit contents:

- PSOC™ Control C3M6 Evaluation Kit board
- USB Type-A to Type-C cable
- Jumper wires (10 wires)
- Quick start guide (QR code for web information)

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

LEDs
----

+---------+-------------------+
| Name    | GPIO Pin          |
+=========+===================+
| LED0    | P8.4 (active low) |
+---------+-------------------+
| LED1    | P8.5 (active low) |
+---------+-------------------+

Push Buttons
------------

+---------+--------------------+
| Name    | GPIO Pin           |
+=========+====================+
| SW4     | P10.2 (active low) |
+---------+--------------------+
| SW3     | P2.0 (active low)  |
+---------+--------------------+

Default Zephyr Peripheral Mapping
----------------------------------

+-----------+-----------------+----------------------------+
| Pin       | Function        | Usage                      |
+===========+=================+============================+
| P6.3      | SCB3 UART TX    | Console TX                 |
+-----------+-----------------+----------------------------+
| P6.2      | SCB3 UART RX    | Console RX                 |
+-----------+-----------------+----------------------------+
| P3.3      | SCB4 UART TX    | User UART TX               |
+-----------+-----------------+----------------------------+
| P3.2      | SCB4 UART RX    | User UART RX               |
+-----------+-----------------+----------------------------+
| P5.3      | CANFD0 CH1 TX   | CAN FD TX                  |
+-----------+-----------------+----------------------------+
| P5.2      | CANFD0 CH1 RX   | CAN FD RX                  |
+-----------+-----------------+----------------------------+
| P9.0      | SCB0 I2C SCL    | I2C clock                  |
+-----------+-----------------+----------------------------+
| P9.2      | SCB0 I2C SDA    | I2C data                   |
+-----------+-----------------+----------------------------+
| P7.0      | SCB2 SPI CLK    | SPI clock                  |
+-----------+-----------------+----------------------------+
| P7.1      | SCB2 SPI MOSI   | SPI MOSI                   |
+-----------+-----------------+----------------------------+
| P7.2      | SCB2 SPI MISO   | SPI MISO                   |
+-----------+-----------------+----------------------------+
| P8.4      | GPIO            | LED0                       |
+-----------+-----------------+----------------------------+
| P8.5      | GPIO            | LED1                       |
+-----------+-----------------+----------------------------+
| P10.2     | GPIO            | Button SW4                 |
+-----------+-----------------+----------------------------+
| P2.0      | GPIO            | Button SW3                 |
+-----------+-----------------+----------------------------+

System Clock
============

The PSOC™ Control C3M6 and evaluation board provide the following clock sources:

- **IMO** (Internal Main Oscillator): 8 MHz
- **IHO** (Internal High-speed Oscillator): 48 MHz
- **ECO** (External Crystal Oscillator): 16 MHz, connected to P1.0 and P1.1
- **WCO** (Watch Crystal Oscillator): 32.768 kHz, connected to P0.0 and P0.1

Serial Port
============

The Zephyr console output is assigned to the debug UART on **SCB3**
(``uart3``).

Default communication settings are **115200 8N1**.

Building
********

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: kit_psc3m6_evk
   :goals: build

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Use ``west flash`` and ``west debug`` with OpenOCD and SEGGER J-Link probe
(requires SEGGER J-Link version v9.68 or later).

Infineon OpenOCD Installation
=============================

The `ModusToolbox™ Programming Tools`_ package includes Infineon OpenOCD.
Alternatively, a standalone installation can be done by downloading the
`Infineon OpenOCD`_ release for your system and extracting the files to a
location of your choice.

Flashing
========

Build and flash the application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: kit_psc3m6_evk
   :goals: build flash
   :west-args: -p always
   :flash-args: --openocd <path/to/openocd>

MCUboot
*******

The ``kit_psc3m6_evk`` board supports `MCUboot`_ in overwrite-only mode. Use
``--sysbuild`` with ``-DSB_CONFIG_BOOTLOADER_MCUBOOT=y`` to build the bootloader
and application together:

.. zephyr-app-commands::
   :tool: west
   :zephyr-app: samples/hello_world
   :board: kit_psc3m6_evk
   :goals: build
   :west-args: --sysbuild
   :gen-args: -DSB_CONFIG_BOOTLOADER_MCUBOOT=y

Secure Boot
***********

The PSOC™ Control C3M6 MCU includes a ROM boot stage (FLASH_BOOT) that
authenticates and launches the first OEM application from internal flash.

The device operates in ``BOOT_SIMPLE_APP`` mode by default: FLASH_BOOT loads
the application at the start of internal flash with no cryptographic signature
verification. Standard Zephyr builds work without any signing in this mode.

When the MCUboot bootloader is used via sysbuild, MCUboot itself is the first
application launched by FLASH_BOOT. MCUboot then independently authenticates
the Zephyr application image in ``slot0_partition`` using the standard Zephyr
``imgtool`` signing flow. No OEM key provisioning is required for this
configuration; ``BOOT_SIMPLE_APP`` mode applies and the MCUboot binary runs
without BootROM signature verification.

Enabling Secure Boot
====================

To enable BootROM signature verification the device must be provisioned.
Follow the `AN240106 PSoC Control C3 Security`_ application note and use
`Infineon EdgeProtect Tools`_ (``edgeprotecttools -t psoc_c3x6``) to
initialise a project, generate an OEM key pair, obtain an Infineon-signed
OEM certificate from the `Infineon OSTS portal`_, and run
``provision-device`` to transfer ownership and enable ``SECURE_APP`` mode.

.. warning::

   ``provision-device`` is **irreversible**. The OEM key hash is written to
   SFLASH and the lifecycle state is advanced. Use a dedicated development
   board.

After provisioning, the first image that FLASH_BOOT verifies must be signed
with the OEM key. When using MCUboot via sysbuild this is the MCUboot binary;
the same ``sign-image`` steps apply to a standalone Zephyr application when
MCUboot is not used, adjusting ``--image`` and ``--slot-size`` to match the
application binary and its partition size. Use the PSC3-specific parameters
derived from the Zephyr partition layout and flash the signed image:

.. code-block:: console

   edgeprotecttools sign-image \
     --image  <build>/mcuboot/zephyr/zephyr.hex \
     --output <build>/mcuboot/zephyr/zephyr.signed.hex \
     --key    <oem-private-key>.pem \
     --hex-addr 0x32000000 \
     --header-size 0x400 --slot-size 0x3B000 \
     --align 1 --min-erase-size 0x200 --erased-val 0 --overwrite-only

   west flash --hex-file <build>/mcuboot/zephyr/zephyr.signed.hex

``--hex-addr 0x32000000`` is required because the SAHB window is the
writable data-access path to the flash; the CBUS window
(``0x12000000``) is read-only and cannot be written.
When using MCUboot, the Zephyr application in ``slot0_partition`` is
signed by the standard ``imgtool`` sysbuild flow and does not require
EPT signing.

References
**********

.. _KIT_PSC3M6_EVK:
    https://www.infineon.com/evaluation-board/KIT-PSC3M6-EVAL

.. _KIT_PSC3M6_EVAL User Guide:
    https://www.infineon.com/document-promo/infineon-kit-psc3m6-eval-user-guide-usermanual-en_3885ebcf-6410-484d-9f5c-0e3c645aec40

.. _ModusToolbox™ Programming Tools:
    https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolboxprogtools

.. _Infineon OpenOCD:
    https://github.com/Infineon/openocd/releases/latest

.. _AN240106 PSoC Control C3 Security:
    https://documentation.infineon.com/psoccontrolc3/docs/jew1730786286560

.. _Infineon EdgeProtect Tools:
    https://pypi.org/project/edgeprotecttools/

.. _Infineon OSTS portal:
    https://osts.infineon.com/epss/home
