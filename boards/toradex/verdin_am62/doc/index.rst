.. zephyr:board:: verdin_am62

Overview
********

The Verdin-AM62 board configuration is used by Zephyr applications that run on
the TI AM62x platform. The board configuration provides support for:

- ARM Cortex-M4F MCU core and the following features:

   - Nested Vector Interrupt Controller (NVIC)
   - System Tick System Clock (SYSTICK)

The board configuration also enables support for the semihosting debugging console.

See the `Toradex Verdin AM62 Product Page`_ for details.

Hardware
********

The Toradex Verdin AM62 is a System on Module (SoM) based on the Texas Instruments AM62x family of
processors. It features up to four Arm® Cortex®-A53 cores, a Cortex®-M4F real-time core, and
dedicated peripherals such as PRU cores.

Zephyr is ported to run on the M4F core. The following listed hardware specifications are used:

- Low-power ARM Cortex-M4F
- Memory

   - 256KB of SRAM
   - 16MB of DDR4 (can go from 512MB to 2GB maximum)

Supported Features
==================

.. zephyr:board-supported-hw::

Devices
========
System Clock
------------

This board configuration uses a system clock frequency of 400 MHz.

DDR RAM
-------

The board can have from 512MB up to 2GB of DDR RAM. This board configuration allocates approximately
16MB of RAM, which includes the resource table, shared memories for IPC and SRAM to be used by the
cortex-M.

Serial Port
-----------

This board configuration uses a single serial communication channel with the
MCU domain UART (MCU_UART0).

Programming the M4F Core
************************

Cortex-M4F core can be programmed by remoteproc on both Linux Kernel or U-Boot bootloader. For the
Linux kernel, the remoteproc uses the resource table to load the firmware into the Cortex-M4F.

To test the M4F core, build the :zephyr:code-sample:`hello_world` sample with the following command:

.. code-block:: console

   # From the root of the Zephyr repository
   west build -p -b verdin_am62/am6234/m4 samples/hello_world

This builds the binary, which is located at :file:`build/zephyr` directory as :file:`zephyr.elf`.

This binary needs to be copied to Verdin AN62 (can be copied to the eMMC, use an SD card, usb
stick...) and place it on :file:`/lib/firmware` directory and name it as :file:`am62-mcu-m4f0_0-fw`.
After that, it can be loaded by Linux remoteproc with the following commands:

.. code-block:: console

   echo start > /sys/class/remoteproc/remoteproc0/state

The binary will run and print Hello world to the MCU_UART0 port.

If instead it is desired to load it with U-Boot, the following commands can be executed into the
bootloader terminal:

.. code-block:: console

   rproc init
   rproc list
   load mmc 0:2 ${loadaddr} /lib/firmware/am62-mcu-m4f0_0-fw
   rproc load ${loadaddr} 0 0x${filesize}
   rproc start 0

.. hint::
   For both remoteproc examples, check the id of the remote processor to make sure the firmware is
   being loaded into the correct core.

When the core starts, in this case with the hello world sample, this will be shown into the UART
from the cortex-m (which will be /dev/ttyUSB2 for both Dahlia and Verdin Development boards):

.. code-block:: console

   *** Booting Zephyr OS build v4.2.0-1172-g242870ac3feb ***
   Hello World! verdin_am62/am6234/m4

References
**********

.. _Toradex Verdin AM62 Product Page:
   https://www.toradex.com/computer-on-modules/verdin-arm-family/ti-am62

.. _Toradex Verdin AM62 Developer Page:
   https://developer.toradex.com/hardware/verdin-som-family/modules/verdin-am62/
