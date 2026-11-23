.. zephyr:board:: pico_ultra

Overview
********

`Luckfox Pico Ultra W`_ is a low-cost micro Linux development boards based on the Rockchip RV1106 chip, and comes with 256MB of DDR3L DRAM.

Hardware
********

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

The Rockchip RV1106 must be initialized via the Luckfox SDK U-Boot before running Zephyr.

Modifying U-Boot
================

The default U-Boot must be modified to enable cache commands and prevent forcing
Thumb mode. Apply these patches to the Luckfox SDK:

.. code-block:: diff

   --- a/sysdrv/source/uboot/u-boot/cmd/boot.c
   +++ b/sysdrv/source/uboot/u-boot/cmd/boot.c
   @@ -21,10 +21,6 @@
    unsigned long do_go_exec(ulong (*entry)(int, char * const []), int argc,
                                     char * const argv[])
    {
   -#ifdef CONFIG_CPU_V7
   -    ulong addr = (ulong)entry | 1;
   -    entry = (void *)addr;
   -#endif
        return entry (argc, argv);
    }

   --- a/sysdrv/source/uboot/u-boot/configs/luckfox_rv1106_uboot_defconfig
   +++ b/sysdrv/source/uboot/u-boot/configs/luckfox_rv1106_uboot_defconfig
   @@ -141,3 +141,4 @@
    CONFIG_SPL_GZIP=y
    CONFIG_ERRNO_STR=y
    # CONFIG_EFI_LOADER is not set
   +CONFIG_CMD_CACHE=y

Follow the official `Luckfox Pico SDK`_ instructions to rebuild and flash U-Boot.

Building and Running
====================

Build the Zephyr application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: pico_ultra
   :goals: build

Load the binary via TFTP, disable and flush caches, then launch Zephyr:

.. code-block:: console

   => dcache off && icache off
   => dcache flush && icache flush
   => setenv ipaddr 192.168.1.2
   => setenv serverip 192.168.1.1
   => tftp 0x0 zephyr.bin
   => go 0x0

You should see the following output on the serial console:

.. code-block:: text

   *** Booting Zephyr OS build v4.3.0-4280-g2f3d729d951a ***
   Hello World! pico_ultra/rv1106


.. _Luckfox Pico Ultra W:
   https://wiki.luckfox.com/Luckfox-Pico-Ultra

.. _Luckfox Pico SDK:
   https://github.com/LuckfoxTECH/luckfox-pico
