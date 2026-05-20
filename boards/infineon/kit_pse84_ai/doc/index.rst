.. zephyr:board:: kit_pse84_ai

Overview
********
The PSOC™ Edge E84 AI kit enables evaluation and development of applications using the PSOC™ Edge
E84 Series Microcontroller (MCU) and a multitude of on-board multimedia, Machine Learning (ML), and
connectivity features like Raspberry Pi compatible MIPI-DSI displays, analog and digital microphones
for audio interfaces, and AIROC™ CYW55513IUBGT base Wi-Fi & Bluetooth combo Murata Type2FY
connectivity module.  The kit also has 512-Mbit Quad-SPI NOR Flash and 128-Mbit Octal-SPI HYPERRAM™.
The board features an on-board programmer/debugger (KitProg3), JTAG/SWD debug headers, expansion I/O
header, USB-C connectors, 6-axis IMU sensor, 3-axis magnetometer, barometric pressure sensor,
humidity sensor, RADAR sensor, user LEDs, and a user buttor.  The MCU power domain and perihporal
power domain supports operating voltages of 1.8V and 3.3V.

PSOC™ E84 MCU is an ultra-low-power PSOC™ device specifically designed for ML, wearables and IoT
products like smart thermostats, smart locks, smart home appliances and industrial HMI.

PSOC™ E84 MCU is a true programmable embedded system-on-chip with dual CPUs, integrating a 400 MHz
Arm® Cortex®-M55 as the primary application processor, a 200 MHz Arm® Cortex®-M33 that supports
low-power operations, and a 400 MHz Arm® Ethos-U55 as a neural net companion processor, graphics and
audio block, DSP capability, security enclave with crypto accelerators and protection units,
high-performance memory expansion capability (QSPI, and Octal HYPERRAM™), low-power analog subsystem
with high performance analog-to-digital conversion and low-power comparators, on-board IoT
connectivity module , communication channels, programmable analog and digital blocks that allow
higher flexibility, in-field tuning of the design, and faster time-to-market.

Hardware
********
For more information about the PSOC™ Edge E84 MCUs and the PSOC™ Edge E84 AI Kit:

- `PSOC™ Edge Arm® Cortex® Multicore SoC Website`_
- `PSOC™ Edge E84 AI Kit Website`_

Kit Features:
=============

- Cortex®-M55 CPU with Helium™ DSP
- Advanced ML with Arm Ethos™-U55 NPU
- Low-Power Cortex®-M33
- NNLite ultra-low power NPU
- Analog and Digital Microphones
- State-of-the-Art Secured Enclave
- Integrated Programmer/Debugger

Kit Contents:
=============

- PSOC™ Edge E84 AI board
- OV7675 DVP camera module

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

Please refer to `kit_pse84_ai User Manual Website`_ for more details.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The KIT-PSE84-AI includes an onboard programmer/debugger (`KitProg3`_) to provide debugging,
flash programming, and serial communication over USB. Flash and debug commands use OpenOCD and
require a custom Infineon OpenOCD version, that supports KitProg3, to be installed.

Please refer to the `ModusToolbox™ software installation guide`_ to install Infineon OpenOCD.

Flashing
========
Applications for the ``kit_pse84_ai/pse846gps2dbzc4a/m33`` board target can be
built, flashed, and debugged in the usual way. See
:ref:`build_an_application` and :ref:`application_run` for more details on
building and running.

Applications for the ``kit_pse84_ai/pse846gps2dbzc4a/m55``
board target need to be built using sysbuild to include the required application for the other core.

Enter the following command to compile ``hello_world`` for the CM55 core:

.. code-block:: console

   west build -p -b kit_pse84_ai/pse846gps2dbzc4a/m55 samples/hello_world --sysbuild

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

            # Do a pristine build once after setting CMake argument
            west build -b kit_pse84_ai/pse846gps2dbzc4a/m33 -p always samples/basic/blinky
            west flash
            west debug

      .. group-tab:: Linux

         .. code-block:: shell

            # Run west config once to set permanent CMake argument
            west config build.cmake-args -- -DOPENOCD=path/to/infineon/openocd/bin/openocd

            # Do a pristine build once after setting CMake argument
            west build -b kit_pse84_ai/pse846gps2dbzc4a/m33 -p always samples/basic/blinky

            west flash
            west debug

Once the gdb console starts after executing the west debug command, you may now set breakpoints and
perform other standard GDB debugging on the PSOC E84 CM33 core.

Secure Boot
***********

The PSOC™ Edge E84 MCU includes an extended boot stage in ROM that, on reset, jumps to the first
application image. On the KIT-PSE84-AI the destination is selected by the level of the boot pin,
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

- `PSOC™ Edge Arm® Cortex® Multicore SoC Website`_

.. _PSOC™ Edge Arm® Cortex® Multicore SoC Website:
    https://www.infineon.com/products/microcontroller/32-bit-psoc-arm-cortex/32-bit-psoc-edge-arm/psoc-edge-e84#Overview

.. _PSOC™ Edge E84 AI Kit Website:
    https://www.infineon.com/evaluation-board/KIT-PSE84-AI

.. _kit_pse84_ai User Manual Website:
    https://www.infineon.com/assets/row/public/documents/30/44/infineon-kit-pse84-ai-user-guide-usermanual-en.pdf

.. _PSOC™ Edge Security Getting Started Application Note:
    https://www.infineon.com/assets/row/public/documents/30/42/infineon-an237849-getting-started-psoc-edge-security-applicationnotes-en.pdf

.. _ModusToolbox™:
    https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolboxsetup

.. _ModusToolbox™ software installation guide:
    https://www.Infineon.com/ModusToolboxInstallguide

.. _KitProg3:
    https://github.com/Infineon/KitProg3
