.. _debug-probes:

Debug Probes
############

A *debug probe* is special hardware which allows you to control execution of a
Zephyr application running on a separate board. Debug probes usually allow
reading and writing registers and memory, and support breakpoint debugging of
the Zephyr application on your host workstation using tools like GDB. They may
also support other debug software and more advanced features such as
:ref:`tracing program execution <tracing>`. For details on the related host
software supported by Zephyr, see :ref:`flash-debug-host-tools`.

Debug probes are usually connected to your host workstation via USB; they
are sometimes also accessible via an IP network or other means. They usually
connect to the device running Zephyr using the JTAG or SWD protocols. Debug
probes are either separate hardware devices or circuitry integrated into the same
board which runs Zephyr.

Many supported boards in Zephyr include a second microcontroller that serves as
an onboard debug probe, usb-to-serial adapter, and sometimes a drag-and-drop
flash programmer. This eliminates the need to purchase an external debug probe
and provides a variety of debug host tool options.

Several hardware vendors have their own branded onboard debug probe
implementations: NXP boards may use
`OpenSDA <#opensda-onboard-debug-probe>`_,
`LPC-Link2 <#lpc-link2-onboard-debug-probe>`_, or
`MCU-Link <#mcu-link-onboard-debug-probe>`_, probes depending on
the microcontroller the debug probe firmware runs on.
ST boards have the `ST-LINK probe <#stlink-v21-onboard-debug-probe>`_. Each
onboard debug probe microcontroller can support one or more types of firmware
that communicate with their respective debug host tools. For example, an
OpenSDA microcontroller can be programmed with DAPLink firmware to communicate
with pyOCD or OpenOCD debug host tools, or with J-Link firmware to communicate
with J-Link debug host tools.


+------------------------------------------+---------------------------------------------------------------------------------------------------------+
|| *Debug Probes & Host Tools*             |                                               Host Tools                                                |
+| *Compatibility Chart*                   +--------------------+--------------------+---------------------+--------------------+--------------------+
|                                          |  **J-Link Debug**  |    **OpenOCD**     |      **pyOCD**      |   **NXP S32DS**    | **NXP LinkServer** |
+----------------+-------------------------+--------------------+--------------------+---------------------+--------------------+--------------------+
|                | **J-Link External**     |           ✓        |          ✓         |                     |                    |                    |
|                +-------------------------+--------------------+--------------------+---------------------+--------------------+--------------------+
|                | **LPC-Link2 CMSIS-DAP** |                    |                    |                     |                    |         ✓          |
|                +-------------------------+--------------------+--------------------+---------------------+--------------------+--------------------+
|                | **LPC-Link2 J-Link**    |           ✓        |                    |                     |                    |                    |
|                +-------------------------+--------------------+--------------------+---------------------+--------------------+--------------------+
|                | **MCU-Link CMSIS-DAP**  |                    |                    |                     |                    |         ✓          |
|  Debug Probes  +-------------------------+--------------------+--------------------+---------------------+--------------------+--------------------+
|                | **MCU-Link J-Link**     |           ✓        |                    |                     |                    |                    |
|                +-------------------------+--------------------+--------------------+---------------------+--------------------+--------------------+
|                | **NXP S32 Debug Probe** |                    |                    |                     |          ✓         |                    |
|                +-------------------------+--------------------+--------------------+---------------------+--------------------+--------------------+
|                | **OpenSDA DAPLink**     |                    |          ✓         |          ✓          |                    |         ✓          |
|                +-------------------------+--------------------+--------------------+---------------------+--------------------+--------------------+
|                | **OpenSDA J-Link**      |           ✓        |                    |                     |                    |                    |
|                +-------------------------+--------------------+--------------------+---------------------+--------------------+--------------------+
|                | **ST-LINK/V2-1**        |           ✓        |          ✓         | *some STM32 boards* |                    |                    |
+----------------+-------------------------+--------------------+--------------------+---------------------+--------------------+--------------------+


Some supported boards in Zephyr do not include an onboard debug probe and
therefore require an external debug probe. In addition, boards that do include
an onboard debug probe often also have an SWD or JTAG header to enable the use
of an external debug probe instead. One reason this may be useful is that the
onboard debug probe may have limitations, such as lack of support for advanced
debuggers or high-speed tracing. You may need to adjust jumpers to prevent the
onboard debug probe from interfering with the external debug probe.

.. _nxp-onboard-debug-probes:

NXP Onboard Debug Probes
************************

NXP boards may have one of several onboard debug probes. These probes include
the :ref:`mcu-link-onboard-debug-probe`, :ref:`lpc-link2-onboard-debug-probe`
and :ref:`opensda-onboard-debug-probe`. Each of these probes is implemented
as a secondary microcontroller present on the evaluation board. The specific
debug probe type present on a given board can be determined based on the
debug microcontroller SOC:

- LPC55S69: :ref:`mcu-link-onboard-debug-probe`
- LPC4322: :ref:`lpc-link2-onboard-debug-probe`
- MK20: :ref:`opensda-onboard-debug-probe`

For example, the :ref:`frdm_k64f` board has an MK20 debug microcontroller,
so this board uses the :ref:`opensda-onboard-debug-probe`.

.. _mcu-link-onboard-debug-probe:

MCU-Link Onboard Debug Probe
****************************

The MCU-Link onboard debug probe uses an LPC55S69 SOC. This probe supports
the following firmwares:

- :ref:`mcu-link-cmsis-onboard-debug-probe` (default firmware)
- :ref:`mcu-link-jlink-onboard-debug-probe`

This probe is programmed using the MCU-Link host tools, which are installed
with the :ref:`linkserver-debug-host-tools`. NXP recommends using NXP's
`MCUXpresso Installer`_ to install the Linkserver tools.

.. _mcu-link-cmsis-onboard-debug-probe:

MCU-Link CMSIS-DAP Onboard Debug Probe
======================================

This is the default firmware installed on MCU-Link debug probes.  The CMSIS-DAP
debug probes allow debugging from any compatible toolchain, including IAR
EWARM, Keil MDK, NXP’s MCUXpresso IDE and MCUXpresso extension for VS Code. In
addition to debug probe functionality, the MCU-Link probes may also provide:

1. SWO trace end point: this virtual device is used by MCUXpresso to retrieve
   SWO trace data. See the MCUXpresso IDE documentation for more information.
#. Virtual COM (VCOM) port / UART bridge connected to the target processor
#. USB to UART, SPI and/or I2C interfaces (depending on MCU-Link
   type/implementation)
#. Energy measurements of the target MCU

This debug probe is compatible with the following debug host tools:

- :ref:`linkserver-debug-host-tools`

Once the MCU-Link host tools are installed, the following steps are
required to program the CMSIS-DAP firmware:

1. Make sure the MCU-Link utility is present on your host machine. This can
   be done by installing :ref:`linkserver-debug-host-tools`.

#. Put the MCU-Link microcontroller into DFU boot mode by attaching the DFU
   jumper then connecting to the USB debug port on the board.  This jumper may
   also be referred to as the ISP jumper, and will be connected to ``PIO0_5``
   on the LPC55S69.

#. Run the ``program_CMSIS`` script, found in the installed MCU-Link ``scripts``
   folder.

#. Remove the DFU jumper and power cycle the board.

.. _mcu-link-jlink-onboard-debug-probe:

MCU-Link JLink Onboard Debug Probe
==================================

This debug probe firmware provides a JLink compatible debug interface,
as well as a USB-Serial adapter. It is compatible with the following debug host
tools:

- :ref:`jlink-debug-host-tools`

These probes do not have JLink firmware installed by default, and must be
updated. Once the MCU-Link host tools are installed, the following steps are
required to program the JLink firmware:

1. Make sure the MCU-Link utility is present on your host machine. This can
   be done by installing :ref:`linkserver-debug-host-tools`.

#. Put the MCU-Link microcontroller into DFU boot mode by attaching the DFU
   jumper then connecting to the USB debug port on the board.  This jumper may
   also be referred to as the ISP jumper, and will be connected to ``PIO0_5``
   on the LPC55S69.

#. Run the ``program_JLINK`` script, found in the installed MCU-Link ``scripts``
   folder.

#. Remove the DFU jumper and power cycle the board.

.. _lpc-link2-onboard-debug-probe:

LPC-LINK2 Onboard Debug Probe
*****************************

The LPC-LINK2 onboard debug probe uses an LPC4322 SOC. This probe supports
the following firmwares:

- :ref:`lpclink2-cmsis-onboard-debug-probe`
- :ref:`lpclink2-jlink-onboard-debug-probe`
- :ref:`lpclink2-daplink-onboard-debug-probe` (default firmware)

This probe is programmed using the LPCScrypt host tools, which are installed
with the :ref:`linkserver-debug-host-tools`. NXP recommends using NXP's
`MCUXpresso Installer`_ to install the Linkserver tools.

.. _lpclink2-cmsis-onboard-debug-probe:

LPC-LINK2 CMSIS DAP Onboard Debug Probe
=======================================

The CMSIS-DAP debug probes allow debugging from any compatible toolchain,
including IAR EWARM, Keil MDK, as well as NXP’s MCUXpresso IDE and
MCUXpresso extension for VS Code.
As well as providing debug probe functionality, the LPC-Link2 probes also
provide:

1. SWO trace end point: this virtual device is used by MCUXpresso to retrieve
   SWO trace data. See the MCUXpresso IDE documentation for more information.
2. Virtual COM (VCOM) port / UART bridge connected to the target processor
3. LPCSIO bridge that provides communication to I2C and SPI slave devices

This debug probe firmware is compatible with the following debug host tools:

- :ref:`linkserver-debug-host-tools`

The probe may be updated to use CMSIS-DAP firmware with the following steps:

1. Make sure the LPCScrypt utility is present on your host machine. This can
   be done by installing :ref:`linkserver-debug-host-tools`.

#. Put the LPC-Link2 microcontroller into DFU boot mode by attaching the DFU
   jumper, then connecting to the USB debug port on the board. This
   jumper is connected to ``P2_6`` on the LPC4322 SOC.

#. Run the ``program_CMSIS`` script, found in the installed LPCScrypt ``scripts``
   folder.

#. Remove the DFU jumper and power cycle the board.

.. _lpclink2-jlink-onboard-debug-probe:

LPC-Link2 J-Link Onboard Debug Probe
====================================

.. note:: On some boards, the J-Link probe firmware will no longer power the
   board via the USB debug port. On these boards, an alternative method
   of powering the board must be used when this firmware is programmed.

This debug probe firmware provides a JLink compatible debug interface,
as well as a USB-Serial adapter. It is compatible with the following debug host
tools:

- :ref:`jlink-debug-host-tools`

The probe may be updated to use the J-Link firmware with the following steps:

.. note:: Verify the firmware supports your board by visiting `Firmware for LPCXpresso`_

1. Make sure the LPCScrypt utility is present on your host machine. This can
   be done by installing :ref:`linkserver-debug-host-tools`.

#. Put the LPC-Link2 microcontroller into DFU boot mode by attaching the DFU
   jumper, then connecting to the USB debug port on the board. This
   jumper is connected to ``P2_6`` on the LPC4322 SOC.

#. Run the ``program_JLINK`` script, found in the installed LPCScrypt ``scripts``
   folder.

#. Remove the DFU jumper and power cycle the board.

.. _lpclink2-daplink-onboard-debug-probe:

LPC-Link2 DAPLink Onboard Debug Probe
=====================================

The LPC-Link2 DAPLink firmware is the default firmware shipped on LPC-Link2
based boards, but is not the recommended firmware. Users should update to
the :ref:`lpclink2-cmsis-onboard-debug-probe` firmware following the
instructions provided above. For details on programming the DAPLink firmware,
see `NXP AN13206`_.

.. _opensda-onboard-debug-probe:

OpenSDA Onboard Debug Probe
***************************

The OpenSDA onboard debug probe is based on the NXP MK20 SOC. It features
drag and drop programming supports, and supports the following debug firmwares:

- :ref:`opensda-daplink-onboard-debug-probe` (default firmware)
- :ref:`opensda-jlink-onboard-debug-probe`

.. _opensda-daplink-onboard-debug-probe:

OpenSDA DAPLink Onboard Debug Probe
===================================

This debug probe firmware is compatible with the following debug host tools:

- :ref:`pyocd-debug-host-tools`
- :ref:`openocd-debug-host-tools`
- :ref:`linkserver-debug-host-tools`

This probe is realized by programming the OpenSDA microcontroller with DAPLink
OpenSDA firmware. NXP provides `OpenSDA DAPLink Board-Specific Firmwares`_.

Install the debug host tools before you program the firmware.

As with all OpenSDA debug probes, the steps for programming the firmware are:

1. Put the OpenSDA microcontroller into bootloader mode by holding the reset
   button while you power on the board. Note that "bootloader mode" in this
   context applies to the OpenSDA microcontroller itself, not the target
   microcontroller of your Zephyr application.

#. After you power on the board, release the reset button. A USB mass storage
   device called **BOOTLOADER** or **MAINTENANCE** will enumerate. If the
   enumerated device is named **BOOTLOADER**, please first update the bootloader
   to the latest revision by following the instructions for a
   `DAPLink Bootloader Update`_.

#. Copy the OpenSDA firmware binary to the USB mass storage device.

#. Power cycle the board, this time without holding the reset button. You
   should see three USB devices enumerate: a CDC device (serial port), a HID
   device (debug port), and a mass storage device (drag-and-drop flash
   programming).

.. _opensda-jlink-onboard-debug-probe:

OpenSDA J-Link Onboard Debug Probe
==================================

This debug probe is compatible with the following debug host tools:

- :ref:`jlink-debug-host-tools`

This probe is realized by programming the OpenSDA microcontroller with J-Link
OpenSDA firmware. Segger provides `OpenSDA J-Link Generic Firmwares`_ and
`OpenSDA J-Link Board-Specific Firmwares`_, where the latter is generally
recommended when available. Board-specific firmwares are required for i.MX RT
boards to support their external flash memories, whereas generic firmwares are
compatible with all Kinetis boards.

Install the debug host tools before you program the firmware.

As with all OpenSDA debug probes, the steps for programming the firmware are:

1. Put the OpenSDA microcontroller into bootloader mode by holding the reset
   button while you plug a USB into the board's USB debug port. Note that
   "bootloader mode" in this context applies to the OpenSDA microcontroller
   itself, not the target microcontroller of your Zephyr application.

#. After you power on the board, release the reset button. A USB mass storage
   device called **BOOTLOADER** or **MAINTENANCE** will enumerate. If the
   enumerated device is named **BOOTLOADER**, please first update the bootloader
   to the latest revision by following the instructions for a
   `DAPLink Bootloader Update`_.

#. Copy the OpenSDA firmware binary to the USB mass storage device.

#. Power cycle the board, this time without holding the reset button. You
   should see two USB devices enumerate: a CDC device (serial port) and a
   vendor-specific device (debug port).

.. _jlink-external-debug-probe:

J-Link External Debug Probe
***************************

`Segger J-Link`_ is a family of external debug probes, including J-Link EDU,
J-Link PLUS, J-Link ULTRA+, and J-Link PRO, that support a large number of
devices from different hardware architectures and vendors.

This debug probe is compatible with the following debug host tools:

- :ref:`jlink-debug-host-tools`
- :ref:`openocd-debug-host-tools`

Install the debug host tools before you program the firmware.

.. _stlink-v21-onboard-debug-probe:

ST-LINK/V2-1 Onboard Debug Probe
********************************

ST-LINK/V2-1 is a serial and debug adapter built into all Nucleo and Discovery
boards. It provides a bridge between your computer (or other USB host) and the
embedded target processor, which can be used for debugging, flash programming,
and serial communication, all over a simple USB cable.

It is compatible with the following host debug tools:

- :ref:`openocd-debug-host-tools`
- :ref:`jlink-debug-host-tools`

For some STM32 based boards, it is also compatible with:

- :ref:`pyocd-debug-host-tools`

While it works out of the box with OpenOCD, it requires some flashing
to work with J-Link. To do this, SEGGER offers a firmware upgrading the
ST-LINK/V2-1 on board on the Nucleo and Discovery boards. This firmware makes
the ST-LINK/V2-1 compatible with J-LinkOB, allowing users to take advantage of
most J-Link features like the ultra fast flash download and debugging speed or
the free-to-use GDBServer.

More information about upgrading ST-LINK/V2-1 to JLink or restore ST-Link/V2-1
firmware please visit: `Segger over ST-Link`_

Flash and debug with ST-Link
============================

.. tabs::

    .. tab:: Using OpenOCD

        OpenOCD is available by default on ST-Link and configured as the default flash
        and debug tool. Flash and debug can be done as follows:

          .. zephyr-app-commands::
             :zephyr-app: samples/hello_world
             :goals: flash

          .. zephyr-app-commands::
             :zephyr-app: samples/hello_world
             :goals: debug

    .. tab:: _`Using Segger J-Link`

        Once STLink is flashed with SEGGER FW and J-Link GDB server is installed on your
        host computer, you can flash and debug as follows:

        Use CMake with ``-DBOARD_FLASH_RUNNER=jlink`` to change the default OpenOCD
        runner to J-Link. Alternatively, you might add the following line to your
        application ``CMakeList.txt`` file.

          .. code-block:: cmake

             set(BOARD_FLASH_RUNNER jlink)

        If you use West (Zephyr's meta-tool) you can modify the default runner using
        the ``--runner`` (or ``-r``) option.

          .. code-block:: console

             west flash --runner jlink

        To attach a debugger to your board and open up a debug console with ``jlink``.

          .. code-block:: console

             west debug --runner jlink

        For more information about West and available options, see :ref:`west`.

        If you configured your Zephyr application to use `Segger RTT`_ console instead,
        open telnet:

          .. code-block:: console

             $ telnet localhost 19021
             Trying ::1...
             Trying 127.0.0.1...
             Connected to localhost.
             Escape character is '^]'.
             SEGGER J-Link V6.30f - Real time terminal output
             J-Link STLink V21 compiled Jun 26 2017 10:35:16 V1.0, SN=773895351
             Process: JLinkGDBServerCLExe
             Zephyr Shell, Zephyr version: 1.12.99
             Type 'help' for a list of available commands
             shell>

        If you get no RTT output you might need to disable other consoles which conflict
        with the RTT one if they are enabled by default in the particular sample or
        application you are running, such as disable UART_CONSOLE in menuconfig

Updating or restoring ST-Link firmware
======================================

ST-Link firmware can be updated using `STM32CubeProgrammer Tool`_.
It is usually useful when facing flashing issues, for instance when using
twister's device-testing option.

Once installed, you can update attached board ST-Link firmware with the
following command

  .. code-block:: console

     s java -jar ~/STMicroelectronics/STM32Cube/STM32CubeProgrammer/Drivers/FirmwareUpgrade/STLinkUpgrade.jar -sn <board_uid>

Where board_uid can be obtained using twister's generate-hardware-map
option. For more information about twister and available options, see
:ref:`twister_script`.

.. _nxp-s32-debug-probe:

NXP S32 Debug Probe
*******************

`NXP S32 Debug Probe`_ enables NXP S32 target system debugging via a standard
debug port while connected to a developer's workstation via USB or remotely via
Ethernet.

NXP S32 Debug Probe is designed to work in conjunction with NXP S32 Design Studio
(S32DS) and NXP Automotive microcontrollers and processors. Install the debug
host tools as in indicated in :ref:`nxp-s32-debug-host-tools` before you program
the firmware.

.. _LPCScrypt:
   https://www.nxp.com/lpcscrypt

.. _Firmware for LPCXpresso:
   https://www.segger.com/products/debug-probes/j-link/models/other-j-links/lpcxpresso-on-board/

.. _OpenSDA DAPLink Board-Specific Firmwares:
   https://www.nxp.com/opensda

.. _OpenSDA J-Link Generic Firmwares:
   https://www.segger.com/downloads/jlink/#JLinkOpenSDAGenericFirmwares

.. _OpenSDA J-Link Board-Specific Firmwares:
   https://www.segger.com/downloads/jlink/#JLinkOpenSDABoardSpecificFirmwares

.. _Segger J-Link:
   https://www.segger.com/products/debug-probes/j-link/

.. _Segger over ST-Link:
   https://www.segger.com/products/debug-probes/j-link/models/other-j-links/st-link-on-board/

.. _Segger RTT:
    https://www.segger.com/jlink-rtt.html

.. _STM32CubeProgrammer Tool:
    https://www.st.com/en/development-tools/stm32cubeprog.html

.. _MCUXpresso Installer:
	https://www.nxp.com/lgfiles/updates/mcuxpresso/MCUXpressoInstaller.exe

.. _NXP S32 Debug Probe:
   https://www.nxp.com/design/software/automotive-software-and-tools/s32-debug-probe:S32-DP

.. _NXP AN13206:
   https://www.nxp.com/docs/en/application-note/AN13206.pdf

.. _DAPLink Bootloader Update:
   https://os.mbed.com/blog/entry/DAPLink-bootloader-update/
