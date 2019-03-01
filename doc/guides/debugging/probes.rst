.. _debug-probes:

Debug Probes
############

A *debug probe* is special hardware which allows you to control execution of a
Zephyr application running on a separate board. Debug probes usually allow
reading and writing registers and memory, and support breakpoint debugging of
the Zephyr application on your host workstation using tools like GDB. They may
also support other debug software and more advanced features such as
:ref:`tracing program execution <tracing>`. For details on the related host
software supported by Zephyr, see :ref:`debug-host-tools`.

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
implementations: NXP LPC boards have LPC-Link2, NXP Kinetis (former Freescale)
boards have OpenSDA, and ST boards have ST-LINK. Each onboard debug probe
microcontroller can support one or more types of firmware that communicate with
their respective debug host tools. For example, an OpenSDA microcontroller can
be programmed with DAPLink firmware to communicate with pyOCD or OpenOCD debug
host tools, or with J-Link firmware to communicate with J-Link debug host
tools.

Some supported boards in Zephyr do not include an onboard debug probe and
therefore require an external debug probe. In addition, boards that do include
an onboard debug probe often also have an SWD or JTAG header to enable the use
of an external debug probe instead. One reason this may be useful is that the
onboard debug probe may have limitations, such as lack of support for advanced
debuggers or high-speed tracing. You may need to adjust jumpers to prevent the
onboard debug probe from interfering with the external debug probe.

.. _lpclink2-jlink-onboard-debug-probe:

LPC-Link2 J-Link Onboard Debug Probe
************************************

The LPC-Link2 J-Link is an onboard debug probe and usb-to-serial adapter
supported on many NXP LPC and i.MX RT development boards.

This debug probe is compatible with the following debug host tools:

- :ref:`jlink-debug-host-tools`

This probe is realized by programming the LPC-Link2 microcontroller with J-Link
LPC-Link2 firmware. Download and install `LPCScrypt`_ to get the firmware and
programming scripts.

1. Put the LPC-Link2 microcontroller into DFU boot mode by attaching the DFU
   jumper, then powering up the board.

#. Run the ``program_JLINK`` script.

#. Remove the DFU jumper and power cycle the board.

.. _opensda-daplink-onboard-debug-probe:

OpenSDA DAPLink Onboard Debug Probe
***********************************

The OpenSDA DAPLink is an onboard debug probe and usb-to-serial adapter
supported on many NXP Kinetis and i.MX RT development boards. It also includes
drag-and-drop flash programming support.

This debug probe is compatible with the following debug host tools:

- :ref:`pyocd-debug-host-tools`
- :ref:`openocd-debug-host-tools`

This probe is realized by programming the OpenSDA microcontroller with DAPLink
OpenSDA firmware. NXP provides `OpenSDA DAPLink Board-Specific Firmwares`_.

Before you program the firmware, make sure to install the debug host tools
first.

As with all OpenSDA debug probes, the steps for programming the firmware are:

1. Put the OpenSDA microcontroller into bootloader mode by holding the reset
   button while you power on the board. Note that "bootloader mode" in this
   context applies to the OpenSDA microcontroller itself, not the target
   microcontroller of your Zephyr application.

#. After you power on the board, release the reset button. A USB mass storage
   device called **BOOTLOADER** or **MAINTENANCE** will enumerate.

#. Copy the OpenSDA firmware binary to the USB mass storage device.

#. Power cycle the board, this time without holding the reset button. You
   should see three USB devices enumerate: a CDC device (serial port), a HID
   device (debug port), and a mass storage device (drag-and-drop flash
   programming).

.. _opensda-jlink-onboard-debug-probe:

OpenSDA J-Link Onboard Debug Probe
**********************************

The OpenSDA J-Link is an onboard debug probe and usb-to-serial adapter
supported on many NXP Kinetis and i.MX RT development boards.

This debug probe is compatible with the following debug host tools:

- :ref:`jlink-debug-host-tools`

This probe is realized by programming the OpenSDA microcontroller with J-Link
OpenSDA firmware. Segger provides `OpenSDA J-Link Generic Firmwares`_ and
`OpenSDA J-Link Board-Specific Firmwares`_, where the latter is generally
recommended when available. Board-specific firmwares are required for i.MX RT
boards to support their external flash memories, whereas generic firmwares are
compatible with all Kinetis boards.

Before you program the firmware, make sure to install the debug host tools
first.

As with all OpenSDA debug probes, the steps for programming the firmware are:

1. Put the OpenSDA microcontroller into bootloader mode by holding the reset
   button while you power on the board. Note that "bootloader mode" in this
   context applies to the OpenSDA microcontroller itself, not the target
   microcontroller of your Zephyr application.

#. After you power on the board, release the reset button. A USB mass storage
   device called **BOOTLOADER** or **MAINTENANCE** will enumerate.

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

Before you use this debug probe, make sure to install the debug host tools
first.

.. _LPCScrypt:
   https://www.nxp.com/lpcscrypt

.. _OpenSDA DAPLink Board-Specific Firmwares:
   https://www.nxp.com/opensda

.. _OpenSDA J-Link Generic Firmwares:
   https://www.segger.com/downloads/jlink/#JLinkOpenSDAGenericFirmwares

.. _OpenSDA J-Link Board-Specific Firmwares:
   https://www.segger.com/downloads/jlink/#JLinkOpenSDABoardSpecificFirmwares

.. _Segger J-Link:
   https://www.segger.com/products/debug-probes/j-link/
