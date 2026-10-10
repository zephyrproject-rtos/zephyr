.. zephyr:board:: kit_pse84_ai

Overview
********

The `KIT_PSE84_AI`_ is an evaluation kit based on the PSOC™ Edge E84
family, featuring a PSE846GPS2DBZC4A microcontroller with an Arm®
Cortex®-M33 core at 200 MHz, an Arm® Cortex®-M55 core at 400 MHz, and
an Arm® Ethos™-U55 NPU. It is designed for machine learning, wearables,
and IoT applications.

Key features include 512-Mb QSPI NOR flash, 128-Mb Octal HYPERRAM™,
AIROC™ CYW55513-based Wi-Fi + Bluetooth® connectivity (LBEE5HY2FY
module), a rich sensor suite (6-axis IMU, magnetometer, barometric
pressure, humidity, radar), MIPI-DSI display interface, analog and
digital microphones, and an OV7675 DVP camera module.

The board includes an onboard `KitProg3`_ programmer/debugger with USB
Type-C connectivity, expansion IO header, and Raspberry Pi compatible
MIPI-DSI display support.

Boot Flow
=========

The Cortex®-M33 is the boot core. The Cortex®-M55 stays in reset until an
application running on the M33 releases it.

.. code-block:: none

   Reset
     -> ROM extended boot (boot pin HIGH by default: first image in external flash)
     -> CM33 Secure image (Zephyr, or TF-M when using the /ns target)
          -> CM33 application
          -> optional: releases the CM55
               -> CM55 application

The first image run by the ROM must be in MCUboot image format. The build
produces this format automatically; see `Secure Boot`_ for details.

Board Targets
*************

.. list-table::
   :header-rows: 1
   :widths: 35 40 10 15

   * - Build Target
     - What runs
     - Sysbuild
     - Flashed file
   * - ``kit_pse84_ai/pse846gps2dbzc4a/m33``
     - Zephyr on CM33 Secure. The CM55 is not started.
     - No
     - ``zephyr.signed.hex``
   * - ``kit_pse84_ai/pse846gps2dbzc4a/m33/ns``
     - TF-M on CM33 Secure, Zephyr on CM33 Non-Secure.
     - No
     - ``tfm_merged.hex``
   * - ``kit_pse84_ai/pse846gps2dbzc4a/m55``
     - Zephyr on CM55. A minimal CM33 image (``enable_cm55``) is built and flashed with it to
       start the CM55.
     - Yes
     - Both images

There are two ways to run an application on the CM55:

- **Standalone CM55** (``m55`` target with ``--sysbuild``): the CM55 runs the
  application and the CM33 only starts it. This is the usual choice.
- **CM55 with TF-M** (``m33/ns`` and ``m55`` built separately, **without**
  sysbuild): the CM55 uses the TF-M secure services running on the CM33. See
  `TF-M Multicore Support`_.

Hardware
********

- **SoC:** PSOC™ Edge E84 (PSE846GPS2DBZC4A)
- **CPUs:** Arm® Cortex®-M33 at 200 MHz (boot core), Arm® Cortex®-M55 at
  400 MHz
- **NPU:** Arm® Ethos™-U55
- **Flash:** 512-Mb QSPI NOR flash
- **RAM:** 128-Mb Octal HYPERRAM
- **Wireless:** LBEE5HY2FY module — Wi-Fi + Bluetooth® (AIROC CYW55513)
- **Sensors:** 6-axis IMU, 3-axis magnetometer, barometric pressure, humidity, radar
- **Display:** MIPI-DSI (Raspberry Pi compatible)
- **Audio:** Analog and digital microphones
- **USB:** Type-C (device)
- **Expansion:** Expansion IO header
- **User I/O:** User LEDs and user button
- **Security:** Arm® TrustZone®-M, secure enclave with crypto accelerators
- **Debug:** Onboard KitProg3 (SWD + UART bridge)

For more information about the PSOC™ Edge E84 and KIT_PSE84_AI:

- `PSOC Edge E84 SoC Website`_
- `KIT_PSE84_AI Board Website`_
- `KIT_PSE84_AI User Manual`_

Kit Contents
============

- PSOC™ Edge E84 AI Evaluation Kit board
- OV7675 DVP camera module

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

+-------+---------------+------------------------------------------+
| Pin   | Function      | Usage                                    |
+=======+===============+==========================================+
| P6.7  | SCB2 UART TX  | Console TX (``uart2``)                   |
+-------+---------------+------------------------------------------+
| P6.5  | SCB2 UART RX  | Console RX (``uart2``)                   |
+-------+---------------+------------------------------------------+
| P10.1 | SCB4 UART TX  | BT HCI TX (``uart4``)                    |
+-------+---------------+------------------------------------------+
| P10.0 | SCB4 UART RX  | BT HCI RX (``uart4``)                    |
+-------+---------------+------------------------------------------+
| P10.3 | SCB4 UART RTS | BT HCI RTS (``uart4``)                   |
+-------+---------------+------------------------------------------+
| P10.2 | SCB4 UART CTS | BT HCI CTS (``uart4``)                   |
+-------+---------------+------------------------------------------+
| P10.7 | GPIO          | LED0 (active high)                       |
+-------+---------------+------------------------------------------+
| P10.5 | GPIO          | LED1 (active high)                       |
+-------+---------------+------------------------------------------+
| P20.6 | GPIO          | RGB LED red (active high)                |
+-------+---------------+------------------------------------------+
| P20.4 | GPIO          | RGB LED green (active high)              |
+-------+---------------+------------------------------------------+
| P20.5 | GPIO          | RGB LED blue (active high)               |
+-------+---------------+------------------------------------------+
| P7.0  | GPIO          | User button SW1, ``sw0`` alias           |
|       |               | (active low, pull-up)                    |
+-------+---------------+------------------------------------------+

Serial Port
===========

All targets use **SCB2** (``uart2``) for the console and shell. It is routed
through the KitProg3 USB-UART bridge. Default settings are **115200 8N1**.
When both cores run Zephyr, their output shares this UART.

The Bluetooth HCI UART is **SCB4** (``uart4``), with hardware flow control
(RTS/CTS), connected to the LBEE5HY2FY module.

System Clock
============

The PSOC™ Edge E84 has 14 high-frequency clocks (``CLK_HF0`` to
``CLK_HF13``), configured by the CM33 image. The ones most relevant to
applications are:

- **CLK_HF0:** 200 MHz, CM33 core and system clock
- **CLK_HF1:** 400 MHz, CM55 core

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The `KIT_PSE84_AI`_ includes an onboard programmer/debugger (`KitProg3`_)
which can be used to program and debug both PSOC™ Edge E84 cores.

Board Setup
===========

Connect a USB cable from your PC to the KitProg3 USB Type-C connector (J1).
Open the KitProg3 serial port with a terminal of your choice (minicom, PuTTY,
etc.) at **115200 8N1**.

The examples below assume the default boot pin setting, which boots from
external flash (see `Secure Boot`_).

Infineon OpenOCD Installation
=============================

The `ModusToolbox™ Programming Tools`_ package includes Infineon OpenOCD.
Alternatively, download the `Infineon OpenOCD`_ release for your system and
extract it to a location of your choice.

.. note::

   On Linux, KitProg3 needs device access rights. The ModusToolbox™
   Programming Tools installer sets these up. For a standalone OpenOCD
   installation, run ``openocd/udev_rules/install_rules.sh``.

Tell west where Infineon OpenOCD is installed. On Windows the executable is
``openocd.exe``.

.. code-block:: shell

   west config build.cmake-args -- "-DOPENOCD=path/to/infineon/openocd/bin/openocd"

.. note::

   This replaces any existing ``build.cmake-args`` value. To set the path for a
   single command instead, use ``west flash --openocd <path>``.

Building and Flashing
=====================

CM33:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: kit_pse84_ai/pse846gps2dbzc4a/m33
   :goals: build flash

CM33 Non-Secure with TF-M:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: kit_pse84_ai/pse846gps2dbzc4a/m33/ns
   :goals: build flash

CM55 (sysbuild builds and flashes the ``enable_cm55`` CM33 image as well):

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: kit_pse84_ai/pse846gps2dbzc4a/m55
   :west-args: --sysbuild
   :goals: build flash

The console shows the board target that was built, for example:

.. code-block:: console

   *** Booting Zephyr OS build vX.Y.Z ***
   Hello World! kit_pse84_ai/pse846gps2dbzc4a/m33

Use ``west flash --erase`` to erase the device before programming. This erases
the internal RRAM and **all** external flash, including any stored settings.

Debugging
=========

CM33:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: kit_pse84_ai/pse846gps2dbzc4a/m33
   :goals: debug

For the CM55, ``west debug`` attaches to the CM55 on GDB port 3334.

Secure Boot
***********

The PSOC™ Edge E84 MCU includes an extended boot stage in ROM that, on reset, jumps to the first
application image. On the KIT_PSE84_AI the destination is selected by the level of the boot pin,
which by default is pulled HIGH and causes the ROM extended boot to jump to the first application
located in **external flash**.

To make the ROM extended boot jump to a first application located in internal **RRAM**, one of the
following must be done:

- **Hardware rework**: remove resistor ``R188`` and populate resistor ``R187`` to pull the boot
  pin LOW.
- **Reprovisioning (no hardware rework)**: reprovision the device using the same flow described
  in `Enabling Secure Boot`_ below, but customize the generated OEM policy JSON to ignore the
  boot pin state. While following the provisioning steps, after the OEM key pair has been
  generated, set ``oem_alt_boot`` to ``false`` in
  :file:`policy/policy_oem_provisioning.json` in the project, before provisioning the kit.

In either case, the boot behavior is then locked to booting from RRAM and must be reverted
(reattaching ``R188`` / removing ``R187``, or reprovisioning again with ``oem_alt_boot`` set back
to ``true``) to re-enable booting from external flash.

In all cases the first application image must be in MCUboot image format, i.e. it must be
preceded by an MCUboot image header (magic number, header size, vector table address, image size)
and followed by the trailer with the hash/signature TLVs. Out of the box, the device is **not**
provisioned for secure boot, so the ROM extended boot only checks the image format and hash; no
cryptographic signature verification is performed against a provisioned key.

The MCUboot image format is produced automatically by the
:file:`soc/infineon/edge/pse84/pse84_metadata.cmake` helper
``pse84_add_extboot_metadata()``, which invokes ``imgtool sign`` with the header address,
header size and slot size derived from the devicetree memory map. By default this helper does not
pass a signing key, which is sufficient for a non-provisioned device.

Enabling Secure Boot
====================

To enable real signature verification by the ROM extended boot, the device must be reprovisioned.
Follow sections **2.2.1**, **2.2.2** and **2.2.3** of the
`PSOC™ Edge Security Getting Started Application Note`_ to:

#. Generate (or import) the OEM signing key pair.
#. Provision the device with the corresponding public key and lifecycle transition.
#. Program the desired security counter / anti-rollback value.

After the device has been reprovisioned, the
``pse84_add_extboot_metadata()`` function in
:file:`soc/infineon/edge/pse84/pse84_metadata.cmake` must be updated so that ``imgtool sign``
also receives the signing key and a security counter. The relevant additions are:

.. code-block:: none

   ${PYTHON_EXECUTABLE} ${IMGTOOL} sign --version "0.0.0+0"
     --header-size ${header_size} --erased-val 0xff
     --slot-size ${slot_size} --hex-addr ${hex_addr}
     --key <oem-private-key-file>
     --security-counter <value>
     ${INPUT_FILE} ${OUTPUT_FILE}

Where ``<oem-private-key-file>`` is the path to the OEM private key file (e.g. a ``.pem``
file) matching the public key provisioned into the device, and ``<value>`` is the security
counter assigned during provisioning. Without these additional parameters, images built for a
provisioned device will be rejected by the ROM extended boot.

TF-M Multicore Support
**********************

In this configuration TF-M runs on the CM33 Secure side and serves PSA requests from both the
CM33 Non-Secure application and the CM55 application. The CM55 reaches TF-M through a
mailbox-based relay. It is enabled with ``CONFIG_PSOC_EDGE_M55_SRF_SUPPORT``, which must be set
on **both** images.

.. note::

   Unlike the standalone CM55 target, this configuration does **not** use sysbuild. The two
   images are built as separate Zephyr applications and flashed separately.

Building
========

The CM55 build uses PSA manifest headers generated by the CM33 Non-Secure (TF-M) build, so the
CM33 image **must be built first**. The CM55 build is pointed at it with the
``PSE84_CM33_BUILD_DIR`` CMake variable, which must be the CM33 build directory (the one passed
to ``west build -d``). Use an absolute path.

#. Build the CM33 Non-Secure image:

   .. code-block:: shell

      west build -b kit_pse84_ai/pse846gps2dbzc4a/m33/ns \
                 -d build_multicore_33 samples/hello_world \
                 -- -DCONFIG_PSOC_EDGE_M55_SRF_SUPPORT=y

#. Build the CM55 image:

   .. code-block:: shell

      west build -b kit_pse84_ai/pse846gps2dbzc4a/m55 \
                 -d build_multicore_55 samples/basic/blinky \
                 -- -DCONFIG_PSOC_EDGE_M55_SRF_SUPPORT=y \
                    -DPSE84_CM33_BUILD_DIR=<absolute-path-to>/build_multicore_33

If ``PSE84_CM33_BUILD_DIR`` is not given, it defaults to ``${ZEPHYR_BASE}/build``. If the
directory does not contain ``tfm/generated/interface/include``, the CM55 build stops with a
``FATAL_ERROR``.

Flashing
========

Flash the CM55 image first. On reset the CM33 Non-Secure image starts the CM55, and it faults if
no valid CM55 image is present.

.. code-block:: shell

   west flash -d build_multicore_55
   west flash -d build_multicore_33

References
**********

.. _KIT_PSE84_AI:
    https://www.infineon.com/evaluation-board/KIT-PSE84-AI

.. _PSOC Edge E84 SoC Website:
    https://www.infineon.com/products/microcontroller/32-bit-psoc-arm-cortex/32-bit-psoc-edge-arm/psoc-edge-e84

.. _KIT_PSE84_AI Board Website:
    https://www.infineon.com/evaluation-board/KIT-PSE84-AI

.. _KIT_PSE84_AI User Manual:
    https://www.infineon.com/assets/row/public/documents/30/44/infineon-kit-pse84-ai-user-guide-usermanual-en.pdf

.. _PSOC™ Edge Security Getting Started Application Note:
    https://www.infineon.com/assets/row/public/documents/30/42/infineon-an237849-getting-started-psoc-edge-security-applicationnotes-en.pdf

.. _ModusToolbox™ Programming Tools:
    https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolboxprogtools

.. _Infineon OpenOCD:
    https://github.com/Infineon/openocd/releases/latest

.. _KitProg3:
    https://github.com/Infineon/KitProg3
