.. zephyr:board:: cyw920829m2evk_02

Overview
********

The AIROC™ CYW20829 Bluetooth® LE MCU Evaluation Kit (CYW920829M2EVK-02) with its included on-board
peripherals enables evaluation, prototyping, and development of a wide array of
Bluetooth® Low Energy applications, all on Infineon's low power, high performance AIROC™ CYW20829.
The AIROC™ CYW20829's robust RF performance and 10 dBm TX output power without an external power
amplifier (PA). This provides enough link budget for the entire spectrum of Bluetooth® LE use cases
including industrial IoT applications, smart home, asset tracking, beacons and sensors, and
medical devices.

The system features Dual Arm® Cortex®-M33s for powering the MCU and Bluetooth subsystem with
programmable and reconfigurable analog and digital blocks. In addition, on the kit, there is a
suite of on-board peripherals including six-axis inertial measurement unit (IMU), thermistor,
analog mic, user programmable buttons (2), LEDs (2), and RGB LED. There is also extensive GPIO
support with extended headers and Arduino Uno R3 compatibility for third-party shields.

Hardware
********

For more information about the CYW20829 SoC and CYW920829M2EVK-02 board:

- `CYW20829 SoC Website`_
- `CYW920829M2EVK-02 Board Website`_

Kit Features:
=============

- AIROC™ CYW20829 Bluetooth® LE MCU in 56 pin QFN package
- Arduino compatible headers for hardware expansion
- On-board sensors - 6-axis IMU, Thermistor, Infineon analog microphone,
  and Infineon digital microphone
- User switches, RGB LED and user LEDs
- USB connector for power, programming and USB-UART bridge

Kit Contents:
=============

- CYW20829 evaluation board (CYW9BTM2BASE3+CYW920829M2IPA2)
- USB Type-A to Micro-B cable
- Six jumper wires (five inches each)
- Quick start guide


Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
============

The AIROC™ CYW20829 Bluetooth®  MCU SoC is configured to use the internal IMO+FLL as a source for
the system clock. Other sources for the system clock are provided in the SOC, depending on your
system requirements.

Fetch Binary Blobs
******************

cyw920829m2evk_02 board requires fetch binary files (e.g Bluetooth controller firmware).

To fetch Binary Blobs:

.. code-block:: console

   west blobs fetch hal_infineon

Build blinking led sample
*************************

Here is an example for building the :zephyr:code-sample:`blinky` sample application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: cyw920829m2evk_02
   :goals: build

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The CYW920829M2EVK-02 includes an onboard programmer/debugger (`KitProg3`_) to provide debugging,
flash programming, and serial communication over USB. Flash and debug commands use OpenOCD and
require a custom Infineon OpenOCD version, that supports KitProg3, to be installed.

The CYW920829M2EVK-02 supports RTT via a SEGGER JLink device, under the target name cyw20829_tm.
This can be enabled for an application by building with the rtt-console snippet or setting the
following config values: CONFIG_UART_CONSOLE=n, CONFIG_RTT_CONSOLE=y, and CONFIG_USE_SEGGER_RTT=y.
e.g. west build -p always -b cyw920829m2evk_02 samples/basic/blinky -S rtt-console

As an additional note there is currently a discrepancy in RAM address between SEGGER and the
CYW920829M2EVK-02 device. So, for RTT control block, do not use "Auto Detection". Instead, set
the search range to something reflecting: RAM RangeStart at 0x20000000 and RAM RangeSize of 0x3d000.

Infineon OpenOCD Installation
=============================

Both the full `ModusToolbox`_ and the `ModusToolbox Programming Tools`_ packages include Infineon
OpenOCD. Installing either of these packages will also install Infineon OpenOCD. If neither package
is installed, a minimal installation can be done by downloading the `Infineon OpenOCD`_ release for
your system and manually extract the files to a location of your choice.

.. note:: Linux requires device access rights to be set up for KitProg3. This is handled
    automatically by the ModusToolbox and ModusToolbox Programming Tools installations.
    When doing a minimal installation, this can be done manually by executing the
    script ``openocd/udev_rules/install_rules.sh``.

West Commands
=============

The path to the installed Infineon OpenOCD executable must be available to the ``west`` tool
commands. There are multiple ways of doing this. The example below uses a permanent CMake argument
to set the CMake variable ``OPENOCD``.

   .. tabs::
      .. group-tab:: Windows

         .. code-block:: shell

            # Run west config once to set permanent CMake argument
            west config build.cmake-args -- -DOPENOCD=path/to/infineon/openocd/bin/openocd.exe

            # Do a pristine build once after setting CMake argument
            west build -b cyw920829m2evk_02 -p always samples/basic/blinky

            west flash
            west debug

      .. group-tab:: Linux

         .. code-block:: shell

            # Run west config once to set permanent CMake argument
            west config build.cmake-args -- -DOPENOCD=path/to/infineon/openocd/bin/openocd

            # Do a pristine build once after setting CMake argument
            west build -b cyw920829m2evk_02 -p always samples/basic/blinky

            west flash
            west debug

Once the gdb console starts after executing the west debug command, you may now set breakpoints and
perform other standard GDB debugging on the CYW20829 CM33 core.

Operate in SECURE Lifecycle Stage
*********************************

The device lifecycle stage (LCS) is a key aspect of the security of the AIROC™
CYW20829 Bluetooth® MCU. The lifecycle stages follow a strict, irreversible progression dictated by
the programming of the eFuse bits (changing the value from "0" to "1"). This system is used to
protect the device's data and code at the level required by the user.
SECURE is the lifecycle stage of a secured device.
Follow the instructions in `AN239590 Provision CYW20829 to SECURE LCS`_ to transition the device
to SECURE LCS. In the SECURE LCS stage, the protection state is set to secure. A secured device
will only boot if the authentication of its flash content is successful.

The following configuration options can be used to build for a device which has been provisioned
to SECURE LCS and configured to use an encrypted flash interface:

- :kconfig:option:`CONFIG_INFINEON_SECURE_LCS`: Enable if the target device is in SECURE LCS
- :kconfig:option:`CONFIG_INFINEON_SECURE_POLICY`: Path to the policy JSON file,
  which was created for provisioning the device to SECURE LCS (refer to section 3.2 "Key creation"
  of `AN239590 Provision CYW20829 to SECURE LCS`_)
- :kconfig:option:`CONFIG_INFINEON_SMIF_ENCRYPTION`: Enable to use encrypted flash interface when provisioned to
  SECURE LCS.

Here is an example for building the :zephyr:code-sample:`blinky` sample application for SECURE LCS.

.. zephyr-app-commands::
   :goals: build
   :board: cyw920829m2evk_02
   :zephyr-app: samples/basic/blinky
   :west-args: -p always
   :gen-args: -DCONFIG_INFINEON_SECURE_LCS=y -DCONFIG_INFINEON_SECURE_POLICY=\"policy/policy_secure.json\"

Using MCUboot
*************

CYW20829 devices are supported by the Cypress MCU bootloader (MCUBootApp) from the
`Cypress branch of MCUboot`_.

Building Cypress MCU Bootloader MCUBootApp
==========================================

Please refer to the `CYW20829 platform description`_ and follow the instructions to understand the
MCUBootApp building process for normal/secure silicon and its overall usage as a bootloader.
Place keys and policy-related folders in the cypress directory ``mcuboot/boot/cypress/``.

Ensure the default memory map matches the memory map of the Zephyr application (refer to partitions
of flash0 in :zephyr_file:`boards/infineon/cyw920829m2evk_02/cyw920829m2evk_02.dts`).

You can use ``west flash`` to flash MCUBootApp:

.. code-block:: shell

   # Flash MCUBootApp.hex
   west flash --skip-rebuild --hex-file /path/to/cypress/mcuboot/boot/cypress/MCUBootApp/out/CYW20829/Debug/MCUBootApp.hex

.. note:: ``west flash`` requires an existing Zephyr build directory which can be created by first
    building any Zephyr application for the target board.

Build Zephyr application
========================
Here is an example for building and flashing the :zephyr:code-sample:`blinky` sample application
for MCUboot.

.. zephyr-app-commands::
   :goals: build flash
   :board: cyw920829m2evk_02
   :zephyr-app: samples/basic/blinky
   :west-args: -p always
   :gen-args: -DCONFIG_BOOTLOADER_MCUBOOT=y -DCONFIG_MCUBOOT_SIGNATURE_KEY_FILE=\"/path/to/cypress/mcuboot/boot/cypress/keys/cypress-test-ec-p256.pem\"

If you use :kconfig:option:`CONFIG_MCUBOOT_ENCRYPTION_KEY_FILE` to generate an encrypted image then the final
hex will be ``zephyr.signed.encrypted.hex`` and the corresponding bin file will
be ``zephyr.signed.encrypted.bin``. Use these files for flashing and ota uploading respectively.
For example, to build and flash an encrypted :zephyr:code-sample:`blinky` sample application
image for MCUboot:

.. zephyr-app-commands::
   :goals: build flash
   :board: cyw920829m2evk_02
   :zephyr-app: samples/basic/blinky
   :west-args: -p always
   :gen-args: -DCONFIG_BOOTLOADER_MCUBOOT=y -DCONFIG_MCUBOOT_SIGNATURE_KEY_FILE=\"/path/to/cypress/mcuboot/boot/cypress/keys/cypress-test-ec-p256.pem\" -DCONFIG_MCUBOOT_ENCRYPTION_KEY_FILE=\"/path/to/cypress/mcuboot/enc-ec256-pub.pem\"
   :flash-args: --hex-file build/zephyr/zephyr.signed.encrypted.hex


.. _CYW20829 platform description:
    https://github.com/mcu-tools/mcuboot/blob/v1.9.4-cypress/boot/cypress/platforms/CYW20829.md

.. _Cypress branch of MCUboot:
    https://github.com/mcu-tools/mcuboot/tree/cypress

.. _AN239590 Provision CYW20829 to SECURE LCS:
    https://www.infineon.com/dgdl/Infineon-AN239590_Provision_CYW20829_CYW89829_to_Secure_LCS-ApplicationNotes-v02_00-EN.pdf?fileId=8ac78c8c8d2fe47b018e3677dd517258

.. _CYW20829 SoC Website:
    https://www.infineon.com/cms/en/product/wireless-connectivity/airoc-bluetooth-le-bluetooth-multiprotocol/airoc-bluetooth-le/cyw20829/

.. _CYW920829M2EVK-02 Board Website:
    https://www.infineon.com/cms/en/product/evaluation-boards/cyw920829m2evk-02/

.. _CYW920829M2EVK-02 BT User Guide:
    https://www.infineon.com/cms/en/product/wireless-connectivity/airoc-bluetooth-le-bluetooth-multiprotocol/airoc-bluetooth-le/cyw20829/#!?fileId=8ac78c8c8929aa4d018a16f726c46b26

.. _ModusToolbox:
    https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolbox

.. _ModusToolbox Programming Tools:
    https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolboxprogtools

.. _Infineon OpenOCD:
    https://github.com/Infineon/openocd/releases/latest

.. _KitProg3:
    https://github.com/Infineon/KitProg3
