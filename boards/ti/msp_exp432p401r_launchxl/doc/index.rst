.. _msp_exp432p401r_launchxl:

MSP-EXP432P401R LaunchXL
########################

Overview
********

The SimpleLink MSP‐EXP432P401R LaunchPad development kit is an easy-to-use evaluation
module for the SimpleLink MSP432P401R microcontroller. It contains everything needed to start
developing on the SimpleLink MSP432 low-power + performance ARM |reg| 32-bit Cortex |reg|-M4F
microcontroller (MCU).

.. figure:: img/msp_exp432p401r_launchxl.jpg
     :align: center
     :alt: MSP-EXP432P401R LaunchXL development board

Features:
=========

* Low-power ARM Cortex-M4F MSP432P401R
* 40-pin LaunchPad development kit standard that leverages the BoosterPack plug-in module ecosystem
* XDS110-ET, an open-source onboard debug probe featuring EnergyTrace+ technology and application
  UART
* Two buttons and two LEDs for user interaction
* Backchannel UART through USB to PC

Details on the MSP-EXP432P401R LaunchXL development board can be found in the
MSP-EXP432P401R LaunchXL User's Guide.

Supported Features
==================

* The on-board 32-kHz crystal allows for lower LPM3 sleep currents and a higher-precision clock source than the
  default internal 32-kHz REFOCLK. Therefore, the presence of the crystal allows the full range of low-
  power modes to be used.
* The on-board 48-MHz crystal allows the device to run at its maximum operating speed for MCLK and HSMCLK.

The MSP-EXP432P401R LaunchXL development board configuration supports the following hardware features:

+-----------+------------+-----------------------+
| Interface | Controller | Driver/Component      |
+===========+============+=======================+
| NVIC      | on-chip    | nested vectored       |
|           |            | interrupt controller  |
+-----------+------------+-----------------------+
| SYSTICK   | on-chip    | system clock          |
+-----------+------------+-----------------------+
| UART      | on-chip    | serial port           |
+-----------+------------+-----------------------+

More details about the supported peripherals are available in MSP432P4XX TRM
Other hardware features are not currently supported by the Zephyr kernel.

Building and Flashing
*********************

Prerequisites:
==============

#. Ensure the XDS-110 emulation firmware is updated.

   Download and install the latest `XDS-110 emulation package`_.

   Follow these `xds110 firmware update directions
   <http://software-dl.ti.com/ccs/esd/documents/xdsdebugprobes/emu_xds110.html#updating-the-xds110-firmware>`_

   Note that the emulation package install may place the xdsdfu utility
   in ``<install_dir>/ccs_base/common/uscif/xds110/``.

#. Install OpenOCD

   You can obtain OpenOCD by following these
   :ref:`installing the latest Zephyr SDK instructions <toolchain_zephyr_sdk>`.

   After the installation, add the directory containing the OpenOCD executable
   to your environment's PATH variable. For example, use this command in Linux:

   .. code-block:: console

      export PATH=$ZEPHYR_SDK_INSTALL_DIR/sysroots/x86_64-pokysdk-linux/usr/bin/openocd:$PATH

   If you had previously installed TI OpenOCD, you can simply switch to use
   the one in the Zephyr SDK. If for some reason you wish to continue to use
   your TI OpenOCD installation, you can set the OPENOCD and
   OPENOCD_DEFAULT_PATH variables in
   :zephyr_file:`boards/ti/msp_exp432p401r_launchxl/board.cmake` to point the build
   to the paths of the OpenOCD binary and its scripts, before
   including the common openocd.board.cmake file:

   .. code-block:: cmake

      set(OPENOCD "/usr/local/bin/openocd" CACHE FILEPATH "" FORCE)
      set(OPENOCD_DEFAULT_PATH /usr/local/share/openocd/scripts)
      include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)

Flashing
========

Follow the :ref:`getting_started` instructions for Zephyr application
development.

For example, to build and flash the :ref:`hello_world` application for the
MSP-EXP432P401R LaunchXL:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: msp_exp432p401r_launchxl
   :goals: flash

This will load the image into flash.

To see program output from UART0, connect a separate terminal window:

.. code-block:: console

  % screen /dev/ttyACM0 115200 8N1

Then press the reset button (S3) on the board to run the program.

Debugging
=========

To debug a previously flashed image, after resetting the board, use the 'debug'
build target:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: msp_exp432p401r_launchxl
   :maybe-skip-config:
   :goals: debug

References
**********

TI MSP432 Wiki:
   https://en.wikipedia.org/wiki/TI_MSP432

TI MSP432P401R Product Page:
   http://www.ti.com/product/msp432p401r

TI MSP432 SDK:
   http://www.ti.com/tool/SIMPLELINK-MSP432-SDK

.. _UniFlash:
   http://processors.wiki.ti.com/index.php/UniFlash_v4_Quick_Guide#Command_Line_Interface

.. _CCS IDE:
   http://www.ti.com/tool/ccstudio

..  _XDS-110 emulation package:
   http://processors.wiki.ti.com/index.php/XDS_Emulation_Software_Package#XDS_Emulation_Software_.28emupack.29_Download
