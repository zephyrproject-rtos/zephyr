.. _debug-host-tools:

Debug Host Tools
################

This guide describes the software tools you can run on your host workstation to
debug Zephyr applications.

Zephyr's west tool has built-in support for all of these in its ``debug``,
``debugserver``, and ``attach`` commands, provided your board hardware supports
them and your Zephyr board directory's :file:`board.cmake` file declares that
support properly. See :ref:`west-build-flash-debug` for more information on
these commands.

.. _jlink-debug-host-tools:

J-Link Debug Host Tools
***********************

Segger provides a suite of debug host tools for Linux, macOS, and Windows
operating systems:

- J-Link GDB Server: GDB remote debugging
- J-Link Commander: Command-line control and flash programming
- RTT Viewer: RTT terminal input and output
- SystemView: Real-time event visualization and recording

These debug host tools are compatible with the following debug probes:

- :ref:`lpclink2-jlink-onboard-debug-probe`
- :ref:`opensda-jlink-onboard-debug-probe`
- :ref:`jlink-external-debug-probe`
- :ref:`stlink-v21-onboard-debug-probe`

Check if your SoC is listed in `J-Link Supported Devices`_.

Download and install the `J-Link Software and Documentation Pack`_ to get the
J-Link GDB Server and Commander, and to install the associated USB device
drivers. RTT Viewer and SystemView can be downloaded separately, but are not
required.

Note that the J-Link GDB server does not yet support Zephyr RTOS-awareness.

.. _openocd-debug-host-tools:

OpenOCD Debug Host Tools
************************

OpenOCD is a community open source project that provides GDB remote debugging
and flash programming support for a wide range of SoCs. A fork that adds Zephyr
RTOS-awareness is included in the Zephyr SDK; otherwise see `Getting OpenOCD`_
for options to download OpenOCD from official repositories.

These debug host tools are compatible with the following debug probes:

- :ref:`opensda-daplink-onboard-debug-probe`
- :ref:`jlink-external-debug-probe`
- :ref:`stlink-v21-onboard-debug-probe`

Check if your SoC is listed in `OpenOCD Supported Devices`_.

.. note:: On Linux, openocd is available though the `Zephyr SDK
   <https://www.zephyrproject.org/developers/#downloads>`_.
   Windows users should use the following steps to install
   openocd:

   - Download openocd for Windows from here: `OpenOCD Windows`_
   - Copy bin and share dirs to ``C:\Program Files\OpenOCD\``
   - Add ``C:\Program Files\OpenOCD\bin`` to 'Path'

.. _pyocd-debug-host-tools:

pyOCD Debug Host Tools
**********************

pyOCD is an open source project from Arm that provides GDB remote debugging and
flash programming support for Arm Cortex-M SoCs. It is distributed on PyPi and
installed when you complete the :ref:`gs_python_deps` step in the Getting
Started Guide. pyOCD includes support for Zephyr RTOS-awareness.

These debug host tools are compatible with the following debug probes:

- :ref:`opensda-daplink-onboard-debug-probe`
- :ref:`stlink-v21-onboard-debug-probe`

Check if your SoC is listed in `pyOCD Supported Devices`_.

.. _J-Link Software and Documentation Pack:
   https://www.segger.com/downloads/jlink/#J-LinkSoftwareAndDocumentationPack

.. _J-Link Supported Devices:
   https://www.segger.com/downloads/supported-devices.php

.. _Getting OpenOCD:
   http://openocd.org/getting-openocd/

.. _OpenOCD Supported Devices:
   https://github.com/zephyrproject-rtos/openocd/tree/master/tcl/target

.. _pyOCD Supported Devices:
   https://github.com/mbedmicro/pyOCD/tree/master/pyocd/target/builtin

.. _OpenOCD Windows:
    http://gnutoolchains.com/arm-eabi/openocd/
