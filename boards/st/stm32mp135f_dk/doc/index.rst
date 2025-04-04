.. zephyr:board:: stm32mp135f_dk

Overview
********
The STM32MP135 Discovery kit (STM32MP135F-DK) leverages the capabilities of the
1 GHz STM32MP135 microprocessors to allow users to develop applications easily with Zephyr RTOS.

It includes an ST-LINK embedded debug tool, LEDs, push-buttons, two 10/100 Mbit/s Ethernet (RMII) connectors, one USB Type-C |reg| connector, four USB Host Type-A connectors, and one microSD™ connector.

To expand the functionality of the STM32MP135 Discovery kit, one GPIO expansion connector is also available for third-party shields.

Additionally, the STM32MP135 Discovery kit features an LCD display with a touch panel, Wi‑Fi |reg| and Bluetooth |reg| Low Energy capability, and a 2-megapixel CMOS camera module.

It also provides secure boot and cryptography features.

Zephyr OS is ported to run on the Cortex |reg|-A7 core.

- STM32MP135FAF7: Arm |reg| Cortex |reg|-A7 32-bit processor at 1 GHz, in a TFBGA320 package
- ST PMIC STPMIC1
- 4-Gbit DDR3L, 16 bits, 533 MHz
- 4.3" 480x272 pixels LCD display module with capacitive touch panel and RGB interface
- UXGA 2-megapixel CMOS camera module (included) with MIPI CSI-2 |reg| / SMIA CCP2 deserializer
- Wi-Fi |reg| 802.11b/g/n
- Bluetooth |reg| Low Energy 4.1
- Dual 10/100 Mbit/s Ethernet (RMII) compliant with IEEE-802.3u, one with Wake on LAN (WoL) support
- USB Host 4-port hub
- USB Type-C |reg| DRP based on an STM32G0 device
- 4 user LEDs
- 4 push-buttons (2× user, tamper, and reset)
- 1 wake-up button
- Board connectors:

  - Dual-lane MIPI CSI-2 |reg| camera module expansion
  - 2x Ethernet RJ45
  - 4x USB Type-A
  - USB Micro-B
  - USB Type-C |reg|
  - microSD™ card holder
  - GPIO expansion
  - 5 V / 3 A USB Type-C |reg| power supply input (charger not provided)
  - VBAT for power backup

- On-board current measurement
- On-board STLINK-V3E debugger/programmer with USB re-enumeration capability:

  - mass storage
  - Virtual COM port
  - debug port

More information about the board can be found at the
`STM32P135 Discovery website`_.

Hardware
********

More information about the STM32MP135F_DK board hardware can be found here:

- `STM32MP135F_DK Hardware Description`_

More information about STM32P135F microprocessor can be found here:

- `STM32MP135F on www.st.com`_
- `STM32MP135F reference manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

STM32MP135F-DK Discovery Board schematic is available here:
`STM32MP135F Discovery board schematics`_.


Default Zephyr Peripheral Mapping:
----------------------------------

- USART_4 TX/RX : PD6/PD8 (UART console)

- USER_BUTTON : PA13
- LED_3 : PA14
- LED_4 : PA13

System Clock
------------

The Cortex |reg|-A7 core is configured to run at a clock speed of up to 1GHz.

Memory mapping
--------------

+------------+-----------------------+----------------+
| Region     |        Address        |     Size       |
+============+=======================+================+
| SYSRAM     | 0x2FFE0000-0x2FFFFFFF | 128KB          |
+------------+-----------------------+----------------+
| SRAM 1     | 0x30000000-0x30003FFF |  16KB          |
+------------+-----------------------+----------------+
| SRAM 2     | 0x30004000-0x30005FFF |   8KB          |
+------------+-----------------------+----------------+
| SRAM 3     | 0x30006000-0x30007FFF |   8KB          |
+------------+-----------------------+----------------+
| DDR        | 0xC0000000-0xDFFFFFFF |   512 MB       |
+------------+-----------------------+----------------+

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Prerequisite
============

The STM32MP135 has a DDR controller that need to be initialized before loading the Zephyr example.

One method to perform this is to flash the Zephyr executable, along with the DDR initialization script, on an SD card inserted in the board. To do so, you first need to :ref:`install STM32CubeProgrammer <stm32cubeprog-flash-host-tools>` and download the `STM32CubeMP13 package`_.

Signature and flashing
======================

After building the Zephyr project, you need to sign your binary file using the Stm32ImageAddHeader.py with the following command:

.. code-block:: console

   python3 ${Path_to_STM32CubeMP13}/Utilities/ImageHeader/Python3/Stm32ImageAddHeader.py ${Path_to_build_dir}/zephyr/zephyr.bin ${STM32CubeMP13}/Projects/STM32MP135C-DK/External_Loader/Prebuild_Binaries/SD_Ext_Loader/zephyr_Signed.bin -bt 10 -la C0000000 -ep C0000000

Here -bt specifies the boot type, -la specifies the load address and -ep the entry point for your executable (same as the load address in this case).

Then, copy :zephyr_file:`boards/st/stm32mp135f_dk/support/Zephyr.tsv` to ``${Path_to_STM32CubeMP13}/Projects/STM32MP135C-DK/External_Loader/Prebuild_Binaries/SD_Ext_Loader/``.

Finally using the Cube Programmer select the Zephyr.tsv and flash the SD card with the following command:

.. code-block:: console

   ${Path_to_STM32cube_Programmer}/bin/STM32_Programmer.sh -c port=${ConnectedPort} p=even br=115200 -d ${Path_to_STM32CubeMP13}/Projects/STM32MP135C-DK/External_Loader/Prebuild_Binaries/SD_Ext_Loader/Zephyr.tsv

.. note::
  You can refer to this example to flash an example to the SD card:
  `How to install STM32Cube software package on microSD card`_

Debugging
=========

You can debug an application using OpenOCD and GDB.

- Build the sample:

  .. zephyr-app-commands::
     :zephyr-app: samples/hello_world
     :board: stm32mp135f_dk
     :goals: build

- Flash the SD card using:
  `How to install STM32Cube software package on microSD card`_

- Run the application from the SD card

- Attach to the target:

  .. code-block:: console

      west attach

.. note::
  The ``run`` command of GDB isn't supported at the moment for this board.

References
**********

.. target-notes::

.. _STM32P135 Discovery website:
   https://www.st.com/en/evaluation-tools/stm32mp135f-dk.html

.. _STM32MP135F Discovery board User Manual:
   https://www.st.com/resource/en/user_manual/dm00862450.pdf

.. _STM32MP135F Discovery board schematics:
   https://www.st.com/resource/en/schematic_pack/mb1635-mp135f-e02-schematic.pdf

.. _STM32MP135F on www.st.com:
   https://www.st.com/content/st_com/en/products/microcontrollers-microprocessors/stm32-arm-cortex-mpus/stm32mp1-series/stm32mp135/stm32mp135f.html

.. _STM32MP135F reference manual:
   https://www.st.com/resource/en/reference_manual/DM00670465-.pdf

.. _STM32MP135 STM32Cube software package:
   https://www.st.com/en/embedded-software/stm32cubemp13.html#get-software

.. _How to install STM32Cube software package on microSD card:
   https://wiki.st.com/stm32mpu/wiki/How_to_load_and_start_STM32CubeMP13_applications_via_microSD_card

.. _STM32MP135F boot architecture:
   https://wiki.st.com/stm32mpu/wiki/STM32CubeMP13_package_-_boot_architecture

.. _STM32MP135F baremetal distribution:
   https://wiki.st.com/stm32mpu/wiki/Category:Bare_metal_-_RTOS_embedded_software

.. _STM32CubeMP13 package:
   https://github.com/STMicroelectronics/STM32CubeMP13

.. _STM32MP135F_DK Hardware Description:
   https://wiki.stmicroelectronics.cn/stm32mpu/wiki/STM32MP135x-DK_-_hardware_description
