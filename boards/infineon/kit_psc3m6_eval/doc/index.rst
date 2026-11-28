.. zephyr:board:: kit_psc3m6_eval

Overview
********

The PSOC™ Control C3M6 Evaluation Kit (`KIT_PSC3M6_EVAL`_) is an evaluation board for the
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
- **User I/O:** Two user LEDs, two user buttons, potentiometer

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
| AN_B4     | SAR ADC ch 12   | Potentiometer              |
+-----------+-----------------+----------------------------+

Standalone DACs
===============

MCPASS v3 provides two independent 12-bit standalone DACs, represented by
``dac0`` and ``dac1`` with compatible ``infineon,hppass-dac-v3``. Each uses
channel 0 and supports buffered pin output.
They are separate from the 10-bit DACs in the CSG block.
Enable :kconfig:option:`CONFIG_MFD` and :kconfig:option:`CONFIG_DAC`; the
HPPASS MFD parent must be enabled for the standalone DAC driver.
The standalone driver requires ``buffered = true`` in the DAC channel
configuration (or ``zephyr,buffered`` in a channel devicetree node).
The ``power-mode`` property defaults to mode 3 for external pin loads.
Modes 0 and 1 are intended for internal loads; mode 2 is not supported.
Output is unavailable while the device is suspended; the driver restores
the configuration and last written value on resume.
After a power-domain or suspend-to-RAM power restoration, the DAC remains
disabled and rejects setup and writes until resume. If resume fails, it
remains suspended and preserves the saved output value for a later retry.

Concurrent operations are serialized per DAC. Calls from a context that cannot
block return ``-EBUSY`` if another operation holds the DAC lock.

The :zephyr:code-sample:`dac` sample uses standalone DAC0 with its internal
supply reference (VDDA). External reference selection is not supported.
The driver loads factory trims from SFLASH on setup and resume, using the
HPPASS parent's devicetree address for DAC register accesses.
To use DAC1, change the sample overlay's ``dac`` phandle
to ``&dac1`` and enable/configure ``&dac1`` instead of ``&dac0``; keep
``dac-channel-id = <0>``.

The dedicated ``drivers.dac.api.infineon_hppass`` API test scenario covers
standalone DAC0 and selects this board in Twister integration runs:

.. zephyr-app-commands::
   :zephyr-app: tests/drivers/dac/dac_api
   :board: kit_psc3m6_eval/psc3m6ges3ahq1
   :goals: build

The standalone DAC0 test profile disables SAR to exercise DAC startup
independently. The API test checks setup and writes, not analog output voltage.

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
   :board: kit_psc3m6_eval
   :goals: build

Board Variants
**************

NOR Flash Variant
=================

The ``kit_psc3m6_eval/psc3m6ges3ahq1/norflash`` variant enables an external
S25FL128 SPI NOR flash (16 MB) connected via SCB5 (P8.1 MOSI, P8.2 MISO,
P8.3 CLK, P8.0 CS). This flash is exposed as a storage partition suitable for
use with the flash API, file systems, or settings storage.

.. note::

   This variant requires a hardware rework. The default board target
   (``kit_psc3m6_eval``) should be used for unmodified boards.

.. note::

   The S25FL128 flash operates in standard 4-wire SPI mode only. The WP# and
   RESET# pins are not connected on this board, so QUAD SPI is not supported.
   Hardware write-protection is not available; software write-protection via
   the flash status register is still possible.

.. _kit_psc3m6_eval_norflash_rework:

Kit Rework for NOR Flash
------------------------

The following resistor rework is required per the `KIT_PSC3M6_EVAL User Guide`_
to enable the SCB5 SPI NOR flash interface:

+------------------------+----------+
| Reference Designator   | Assembly |
+========================+==========+
| R62                    | Stuff    |
+------------------------+----------+
| R63                    | Stuff    |
+------------------------+----------+
| R64                    | Stuff    |
+------------------------+----------+
| R67                    | Stuff    |
+------------------------+----------+
| R71                    | DNI      |
+------------------------+----------+
| R72                    | DNI      |
+------------------------+----------+
| R73                    | DNI      |
+------------------------+----------+
| R74                    | DNI      |
+------------------------+----------+
| R134                   | DNI      |
+------------------------+----------+

Building for the NOR Flash Variant
----------------------------------

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/flash_shell
   :board: kit_psc3m6_eval/psc3m6ges3ahq1/norflash
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
   :board: kit_psc3m6_eval
   :goals: build flash
   :west-args: -p always
   :flash-args: --openocd <path/to/openocd>

MCUboot
*******

The ``kit_psc3m6_eval`` board supports `MCUboot`_ in overwrite-only mode. Use
``--sysbuild`` with ``-DSB_CONFIG_BOOTLOADER_MCUBOOT=y`` to build the bootloader
and application together:

.. zephyr-app-commands::
   :tool: west
   :zephyr-app: samples/hello_world
   :board: kit_psc3m6_eval
   :goals: build
   :west-args: --sysbuild
   :gen-args: -DSB_CONFIG_BOOTLOADER_MCUBOOT=y

References
**********

.. _KIT_PSC3M6_EVAL:
    https://www.infineon.com/evaluation-board/KIT-PSC3M6-EVAL

.. _KIT_PSC3M6_EVAL User Guide:
    https://www.infineon.com/document-promo/infineon-kit-psc3m6-eval-user-guide-usermanual-en_3885ebcf-6410-484d-9f5c-0e3c645aec40

.. _ModusToolbox™ Programming Tools:
    https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolboxprogtools

.. _Infineon OpenOCD:
    https://github.com/Infineon/openocd/releases/latest
