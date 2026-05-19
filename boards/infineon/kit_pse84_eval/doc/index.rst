.. zephyr:board:: kit_pse84_eval

Overview
********
The PSOC™ Edge E84 Evaluation Kit (KIT_PSE84_EVAL) enables applications to use the PSOC™ Edge E84 Series
Microcontroller (MCU) together with multiple on-board multimedia, Machine Learning (ML),
and connectivity features including custom MIPI-DSI displays, audio interfaces,
and AIROC™ Wi-Fi and Bluetooth® combo-based connectivity modules.

The PSOC™ Edge E84 MCUs are based on high-performance Arm® Cortex®-M55 including Helium DSP support,
an Ethos™-U55 NPU, and a low-power Arm® Cortex®-M33 paired with Infineon's ultra-low power NNLite
hardware accelerator. They integrate 2.5D graphics accelerators and display interfaces, while
featuring always-on acoustic activity and wake-word detection, efficient HMI operations, and
extended battery life.

The evaluation kit carries a PSOC™ Edge E84 MCU on a SODIMM-based detachable SOM board connected to
the baseboard. The MCU SOM also has 128 MB of QSP| Flash, 1GB of Octal Flash, 128MB of Octal RAM,
PSOC™ 4000T as CAPSENSE™ co-processor, and onboard AIROC™ Wi-Fi and Bluetooth® combo.

Hardware
********
For more information about the PSOC™ Edge E84 MCUs and the PSOC™ Edge E84 Evaluation Kit:

- `PSOC™ Edge E84 Arm® Cortex® Multicore SoC Website`_
- `PSOC™ Edge E84 Evaluation Kit Website`_

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

- PSOC™ Edge E84 base board
- PSOC™ Edge E84 SOM module
- 4.3in capacitive touch display and USB camera module
- USB Type C to Type-C cable
- Two proximity sensor wires
- Four stand-offs for Raspberry Pi compatible display
- Quick start guide

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

Please refer to `kit_pse84_eval User Manual Website`_ for more details.

Programming and Debugging
*************************

.. NOTE::
   `BOOT SW` on the board **MUST** be set to `ON` for any sample applications to work. On some
   boards this switch may be under the attached LCD screen.

.. zephyr:board-supported-runners::

The KIT-PSE84-EVAL includes an onboard programmer/debugger (`KitProg3`_) to provide debugging,
flash programming, and serial communication over USB. Flash and debug commands use OpenOCD and
require a custom Infineon OpenOCD version, that supports KitProg3, to be installed.

Please refer to the `ModusToolbox™ software installation guide`_ to install Infineon OpenOCD.

Flashing
========

Applications for the ``kit_pse84_eval/pse846gps2dbzc4a/m33`` board target can be
built, flashed, and debugged in the usual way. See
:ref:`build_an_application` and :ref:`application_run` for more details on
building and running.

Applications for the ``kit_pse84_eval/pse846gps2dbzc4a/m55``
board target need to be built using sysbuild to include the required application for the other core.

Enter the following command to compile ``hello_world`` for the CM55 core:

.. zephyr-app-commands::
   :app: samples/hello_world
   :board: kit_pse84_eval/pse846gps2dbzc4a/m55
   :goals: build flash
   :west-args: --sysbuild
   :gen-args: -DOPENOCD=path/to/infineon/openocd/bin/openocd


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
   :board: kit_pse84_eval/pse846gps2dbzc4a/m33
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

.. _PSOC™ Edge E84 Evaluation Kit Website:
    https://www.infineon.com/evaluation-board/KIT-PSE84-EVAL

.. _kit_pse84_eval User Manual Website:
    https://www.infineon.com/assets/row/public/documents/30/44/infineon-kit-pse84-eval-qsg-usermanual-en.pdf

.. _PSOC™ Edge Security Getting Started Application Note:
    https://www.infineon.com/assets/row/public/documents/30/42/infineon-an237849-getting-started-psoc-edge-security-applicationnotes-en.pdf

.. _ModusToolbox™:
    https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolboxsetup

.. _ModusToolbox™ software installation guide:
    https://www.Infineon.com/ModusToolboxInstallguide

.. _KitProg3:
    https://github.com/Infineon/KitProg3
