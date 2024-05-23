.. zephyr:board:: sk_am64

Overview
********

The SK-AM64 board configuration is used by Zephyr applications that run on
the TI AM64x platform. The board configuration provides support for the ARM
Cortex-M4F MCU core and the following features:

- Nested Vector Interrupt Controller (NVIC)
- System Tick System Clock (SYSTICK)

The board configuration also enables support for the semihosting debugging console.

See the `TI AM64 Product Page`_ for details.

Hardware
********
The SK-AM64 EVM features the AM64 SoC, which is composed of a dual Cortex-A53
cluster and a single Cortex-M4 core in the MCU domain. Zephyr is ported to run on
the M4F core and the following listed hardware specifications are used:

- Low-power ARM Cortex-M4F
- Memory

   - 256KB of SRAM
   - 2GB of DDR4

- Debug

   - XDS110 based JTAG

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

The board has 2GB of DDR RAM available. This board configuration
allocates Zephyr 4kB of RAM (only for resource table: 0xa4100000 to 0xa4100400).

Serial Port
-----------

This board configuration uses a single serial communication channel with the
MCU domain UART (MCU_UART0).

GPIO
----

The SK-AM64 has a heartbeat LED connected to MCU_GPIO0_6. It's configured
to build and run the :zephyr:code-sample:`blinky` sample.

SD Card
*******

Download TI's official `WIC`_ and flash the WIC file with an etching software
onto an SD card. This will boot Linux on the A53 application cores of the EVM.
These cores will then load the zephyr binary on the M4 core using remoteproc.

The default configuration can be found in
:zephyr_file:`boards/ti/sk_am64/sk_am64_am6442_m4_defconfig`

Flashing
********

The board can using remoteproc, and uses the OpenAMP resource table to accomplish this.

The testing requires the binary to be copied to the SD card to allow the A53 cores to load it while booting using remoteproc.

To test the M4F core, we build the :zephyr:code-sample:`hello_world` sample with the following command.

.. code-block:: console

   # From the root of the Zephyr repository
   west build -p -b sk_am64/am6442/m4 samples/hello_world

This builds the program and the binary is present in the :file:`build/zephyr` directory as
:file:`zephyr.elf`.

We now copy this binary onto the SD card in the :file:`/lib/firmware` directory and name it as
:file:`am64-mcu-m4f0_0-fw`.

.. code-block:: console

   # Mount the SD card at sdcard for example
   sudo mount /dev/sdX sdcard
   # copy the elf to the /lib/firmware directory
   sudo cp --remove-destination zephyr.elf sdcard/lib/firmware/am64-mcu-m4f0_0-fw

The SD card can now be used for booting. The binary will now be loaded onto the M4F core on boot.

To allow the board to boot using the SD card, set the boot pins to the SD Card boot mode. Refer to `SK-AM64B EVM User's Guide`_.

After changing the boot mode, the board should go through the boot sequence on powering up.
The binary will run and print Hello world to the MCU_UART0 port.

References
**********

.. _TI AM64 Product Page:
   https://www.ti.com/product/AM6442

.. _WIC:
   https://dr-download.ti.com/software-development/software-development-kit-sdk/MD-yXgchBCk98/10.01.10.04/tisdk-default-image-am64xx-evm-10.01.10.04.rootfs.wic.xz

.. _SK-AM64B EVM User's Guide:
   https://www.ti.com/lit/ug/spruj64/spruj64.pdf

.. _build OpenOCD from source:
   https://docs.u-boot.org/en/latest/board/ti/k3.html#building-openocd-from-source
