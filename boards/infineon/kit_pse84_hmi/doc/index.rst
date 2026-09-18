.. zephyr:board:: kit_pse84_hmi

Overview
********
The PSOC™ Edge E84 HMI Kit (KIT_PSE84_HMI) enables applications to use the PSOC™ Edge E84 Series
Microcontroller (MCU) together with multiple on-board multimedia, Machine Learning (ML),
and connectivity features including custom MIPI-DSI displays, audio interfaces,
and AIROC™ Wi-Fi and Bluetooth® combo-based connectivity modules.

The PSOC™ Edge E84 MCUs are based on high-performance Arm® Cortex®-M55 including
Helium DSP support, an Ethos™-U55 NPU, and a low-power Arm® Cortex®-M33 paired
with Infineon's ultra-low power NNLite hardware accelerator. They integrate 2.5D
graphics accelerators and display interfaces, while featuring always-on acoustic
activity and wake-word detection, efficient HMI operations, and extended battery
life.

The HMI kit carries a PSOC™ Edge E84 MCU and includes the following:

- 1 GB Octal Flash and 512 MB SPI HYPERRAM™
- 3.95-inch Raspberry Pi compatible MIPI-DSI capacitive touch TFT LCD with integrated camera module
- Onboard AIROC™ Wi-Fi and Bluetooth® combo module
- 6-axis accelerometer and gyroscope (BMI270), digital humidity and temperature sensor (SHT40),
  and XENSIV™ 60 GHz RADAR sensor (BGT60TR13C) shield for data collection
- KitProg3 onboard SWD programmer/debugger with USB-UART and USB-I2C bridge functionality
- Two XENSIV™ MEMS analog microphones, four XENSIV™ MEMS PDM microphones, and an onboard speaker
  for audio applications
- MicroSD card interface
- Humidity and temperature sensor – SHT40-AD1B-R2 (Sensirion)

Board Targets
*************

The KIT_PSE84_HMI provides the following build targets:

+---------------------------------------------+------------------------------------------------+
| Build Target                                | Description                                    |
+=============================================+================================================+
| ``kit_pse84_hmi/pse846gps2dbzc4a/m33``      | CM33 Secure - primary target for flashing and  |
|                                             | debugging                                      |
+---------------------------------------------+------------------------------------------------+
| ``kit_pse84_hmi/pse846gps2dbzc4a/m55``      | CM55 - requires ``--sysbuild`` flag            |
+---------------------------------------------+------------------------------------------------+

.. note::

   CM55 builds **must** use the ``--sysbuild`` flag. Sysbuild automatically
   creates the CM33 companion application (``samples/basic/minimal`` with
   ``CONFIG_SOC_PSE84_M55_ENABLE``) required to boot the CM55 core.

.. note::

   The CM33 secure image is automatically signed during the build. The build
   outputs a ``.signed.hex`` file suitable for flashing.

Hardware
********

- **SoC:** PSOC™ Edge E84 (PSE846GPS2DBZC4A)
- **Primary CPU:** Arm® Cortex®-M55 at 400 MHz with Helium™ DSP
- **Secondary CPU:** Arm® Cortex®-M33 at 200 MHz with NNLite accelerator
- **NPU:** Arm® Ethos™-U55
- **External flash:** 1-Gbit (128 MiB) octal-DDR NOR (Infineon S28HS01GT) on SMIF0
- **External RAM:** SPI HYPERRAM™
- **Display:** 3.95 in. Raspberry Pi compatible MIPI-DSI capacitive-touch TFT LCD
  with integrated camera module (FocalTech touch controller)
- **Wireless:** AIROC™ CYW55513 Wi-Fi + Bluetooth® combo module (BT HCI over UART)
- **Audio:** Two analog MEMS microphones, four PDM MEMS microphones, onboard speaker
- **Sensors:** BMI270 6-axis IMU, SHT40 humidity/temperature sensor,
  BGT60TR13C 60 GHz RADAR sensor shield
- **Storage:** microSD card interface
- **User I/O:** Three user LEDs and one user button
- **Security:** Arm® TrustZone®-M, secure enclave with crypto accelerators
- **Debug:** Onboard KitProg3 (SWD + USB-UART + USB-I2C bridge)

For more information about the PSOC™ Edge E84 and KIT_PSE84_HMI:

- `PSOC™ Edge E84 Arm® Cortex® Multicore SoC Website`_
- `PSOC™ Edge E84 HMI Kit Website`_

Kit Contents
============

- PSOC™ Edge E84 HMI Kit
- USB Type-C to Type-C cable
- Quick start guide

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

Please refer to the `kit_pse84_hmi User Manual Website`_ for the full pinout.

LEDs
----

+------------+---------------------+
| Name       | GPIO Pin            |
+============+=====================+
| LED0/Red   | P16.7 (active HIGH) |
+------------+---------------------+
| LED1/Green | P16.6 (active HIGH) |
+------------+---------------------+
| LED2/Blue  | P16.5 (active HIGH) |
+------------+---------------------+

Push Buttons
------------

+------+----------------------------+
| Name | GPIO Pin                   |
+======+============================+
| SW0  | P8.3 (active low, pull-up) |
+------+----------------------------+

Default Zephyr Peripheral Mapping
----------------------------------

+-------+-----------------+----------------------------+
| Pin   | Function        | Usage                      |
+=======+=================+============================+
| P6.7  | SCB2 UART TX    | Console TX                 |
+-------+-----------------+----------------------------+
| P6.5  | SCB2 UART RX    | Console RX                 |
+-------+-----------------+----------------------------+
| P10.1 | SCB4 UART TX    | BT HCI TX                  |
+-------+-----------------+----------------------------+
| P10.0 | SCB4 UART RX    | BT HCI RX                  |
+-------+-----------------+----------------------------+
| P10.3 | SCB4 UART RTS   | BT HCI RTS                 |
+-------+-----------------+----------------------------+
| P10.2 | SCB4 UART CTS   | BT HCI CTS                 |
+-------+-----------------+----------------------------+
| P8.0  | SCB0 I2C SCL    | I2C0 (sensors, touch)      |
+-------+-----------------+----------------------------+
| P8.1  | SCB0 I2C SDA    | I2C0 (sensors, touch)      |
+-------+-----------------+----------------------------+
| P16.7 | GPIO            | LED0/Red                   |
+-------+-----------------+----------------------------+
| P16.6 | GPIO            | LED1/Green                 |
+-------+-----------------+----------------------------+
| P16.5 | GPIO            | LED2/Blue                  |
+-------+-----------------+----------------------------+
| P8.3  | GPIO            | Button SW0                 |
+-------+-----------------+----------------------------+

System Clock
============

The PSOC™ Edge E84 cores are sourced from the on-chip DPLLs configured in the
board devicetree:

- **CM33 (CLK_HF0):** 200 MHz
- **CM55 (CLK_HF1):** 400 MHz

Serial Port
===========

The console output is assigned to **SCB2** (``uart2``), routed through the
KitProg3 USB-UART bridge. Default communication settings are **115200 8N1**.

The Bluetooth® HCI UART is assigned to **SCB4** (``uart4``) with hardware flow
control (RTS/CTS).

Building
********

Here is an example for the :zephyr:code-sample:`hello_world` application on the
CM33 core.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: kit_pse84_hmi/pse846gps2dbzc4a/m33
   :goals: build

To build for the CM55 core, use the ``--sysbuild`` flag:

.. code-block:: console

   west build -p -b kit_pse84_hmi/pse846gps2dbzc4a/m55 samples/hello_world --sysbuild

.. note::

   The ``--sysbuild`` flag is required for CM55 builds. Sysbuild automatically
   creates the CM33 companion application that boots the CM55 core.

Programming and Debugging
*************************

.. NOTE::
   `BOOT SW` on the board **MUST** be set to `ON` for any sample applications to work. On some
   boards this switch may be under the attached LCD screen.

.. zephyr:board-supported-runners::

The KIT-PSE84-HMI includes an onboard programmer/debugger (`KitProg3`_) to provide debugging,
flash programming, and serial communication over USB. Flash and debug commands use OpenOCD and
require a custom Infineon OpenOCD version, that supports KitProg3, to be installed.

Please refer to the `ModusToolbox™ software installation guide`_ to install Infineon OpenOCD.

Configuring a Console
=====================

Connect a USB cable from your PC to the KitProg3 USB Type-C connector on the
board. Use the serial terminal of your choice (minicom, PuTTY, etc.) with the
following settings:

- **Speed:** 115200
- **Data:** 8 bits
- **Parity:** None
- **Stop bits:** 1

Flashing
========

.. tabs::

   .. group-tab:: Windows

      One time, set the Infineon OpenOCD path:

      .. code-block:: shell

         west config build.cmake-args -- "-DOPENOCD=path/to/infineon/openocd/bin/openocd.exe"

      Build and flash the application (CM33):

      .. code-block:: shell

         west build -b kit_pse84_hmi/pse846gps2dbzc4a/m33 -p always samples/hello_world
         west flash

      Build and flash the application (CM55 with sysbuild):

      .. code-block:: shell

         west build -b kit_pse84_hmi/pse846gps2dbzc4a/m55 -p always samples/hello_world --sysbuild
         west flash

   .. group-tab:: Linux

      One time, set the Infineon OpenOCD path:

      .. code-block:: shell

         west config build.cmake-args -- -DOPENOCD=path/to/infineon/openocd/bin/openocd

      Build and flash the application (CM33):

      .. code-block:: shell

         west build -b kit_pse84_hmi/pse846gps2dbzc4a/m33 -p always samples/hello_world
         west flash

      Build and flash the application (CM55 with sysbuild):

      .. code-block:: shell

         west build -b kit_pse84_hmi/pse846gps2dbzc4a/m55 -p always samples/hello_world --sysbuild
         west flash

You should see the following message on the console:

.. code-block:: console

   *** Booting Zephyr OS build vX.Y.Z ***
   Hello World! kit_pse84_hmi

Debugging
=========
The path to the installed Infineon OpenOCD executable must be available to the ``west`` tool
commands. There are multiple ways of doing this. The example below uses a permanent CMake argument
to set the CMake variable ``OPENOCD``.

   .. tabs::
      .. group-tab:: Windows

         .. code-block:: shell

            # Run west config once to set permanent CMake argument
            west config build.cmake-args -- -DOPENOCD=path/to/infineon/openocd/bin/openocd.exe

      .. group-tab:: Linux

         .. code-block:: shell

            # Run west config once to set permanent CMake argument
            west config build.cmake-args -- -DOPENOCD=path/to/infineon/openocd/bin/openocd

.. zephyr-app-commands::
   :app: samples/basic/blinky
   :board: kit_pse84_hmi/pse846gps2dbzc4a/m33
   :goals: debug

Once the gdb console starts after executing the west debug command, you may now set breakpoints and
perform other standard GDB debugging on the PSOC E84 CM33 core.

Secure Boot
***********

The PSOC™ Edge E84 MCU includes an extended boot stage in ROM that, on reset, jumps to the first
application image. The destination is selected by the on-board ``BOOT SW``:

- ``BOOT SW`` **OFF**: the ROM extended boot jumps to the first application located in internal
  RRAM.
- ``BOOT SW`` **ON**: the ROM extended boot jumps to the first application located in external
  flash.

In both cases the first application image must be in MCUboot image format, i.e. it must be
preceded by an MCUboot image header (magic number, header size, vector table address, image size)
and followed by the trailer with the hash/signature TLVs. Out of the box, the device is **not**
provisioned for secure boot, so the ROM extended boot only checks the image format and hash; no
cryptographic signature verification is performed against a provisioned key.

The MCUboot image format is produced automatically by the
:file:`soc/infineon/edge/pse84/pse84_metadata.cmake` helper
``pse84_add_metadata_secure_hex()``, which invokes ``imgtool sign`` with the header address,
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
``pse84_add_metadata_secure_hex()`` function in
:file:`soc/infineon/edge/pse84/pse84_metadata.cmake` must be updated so that ``imgtool sign``
also receives the signing key and a security counter. The relevant additions are:

.. code-block:: none

   ${PYTHON_EXECUTABLE} ${IMGTOOL} sign --version "0.0.0+0"
     --header-size ${header_size} --erased-val 0xff --pad-header
     --slot-size ${slot_size} --hex-addr ${header_addr}
     --key <oem-private-key-file>
     --security-counter <value>
     ${INPUT_FILE} ${OUTPUT_FILE}

Where ``<oem-private-key-file>`` is the path to the OEM private key file (e.g. a ``.pem``
file) matching the public key provisioned into the device, and ``<value>`` is the security
counter assigned during provisioning. Without these additional parameters, images built for a
provisioned device will be rejected by the ROM extended boot.

References
**********

- `PSOC™ Edge E84 Arm® Cortex® Multicore SoC Website`_

.. _PSOC™ Edge E84 Arm® Cortex® Multicore SoC Website:
    https://www.infineon.com/products/microcontroller/32-bit-psoc-arm-cortex/32-bit-psoc-edge-arm/psoc-edge-e84#Overview

.. _PSOC™ Edge E84 HMI Kit Website:
    https://www.infineon.com/evaluation-board/KIT-PSE84-HMI

.. _kit_pse84_hmi User Manual Website:
    https://www.infineon.com/assets/row/public/documents/30/44/infineon-kit-pse84-hmi-ug-usermanual-en.pdf

.. _PSOC™ Edge Security Getting Started Application Note:
    https://www.infineon.com/assets/row/public/documents/30/42/infineon-an237849-getting-started-psoc-edge-security-applicationnotes-en.pdf

.. _ModusToolbox™:
    https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolboxsetup

.. _ModusToolbox™ software installation guide:
    https://www.Infineon.com/ModusToolboxInstallguide

.. _KitProg3:
    https://github.com/Infineon/KitProg3
