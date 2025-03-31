.. zephyr:board:: verdin_imx8mm

Overview
********

The Verdin iMX8M Mini is a System on Module based on the NXP® i.MX 8M Mini family of
embedded System on Chips (SoCs). The i.MX 8M Mini family consists of the i.MX 8M Mini Quad,
i.MX 8M Mini QuadLite, i.MX 8M Mini Dual, i.MX 8M Mini DualLite, i.MX 8M Mini Solo, and i.MX
8M Mini SoloLite. The top-tier i.MX 8M Mini Quad features four Cortex-A53 cores as the main
processor cluster. The cores provide complete 64-bit Armv8-A support while maintaining seamless
backwards compatibility with 32-bit Armv7-A software. The main cores run at up to 1.8 GHz for
commercial graded products and 1.6 GHz for industrial temperature range products.

In addition to the main CPU complex, the i.MX 8M Mini features a Cortex-M4F processor which
peaks up to 400 MHz. This processor is independent of the main complex and features its own
dedicated interfaces while still being able to access the regular interfaces. This heterogeneous
multicore system allows for the running of additional real-time operating systems on the M4 cores
for time- and security-critical tasks.

- Board features:

  - RAM: 1GB - 2GB (LPDDR4)
  - Storage:

    - 4GB - 17GB eMMC
    - 2KB I²C EEPROM
  - Wireless:

    - WiFi Dual-band 802.11 ac/a/b/g/n 2.4/5 GHz
    - Bluetooth 5/BLE
  - USB:

    - 1x USB2.0 Host
    - 1x USB2.0 OTG
  - Ethernet Gigabit
  - Interfaces:

    - MIPI DSI 1x4 Data Lanes
    - PCIe Gen 2
    - MIPI CSI-2
    - SPI
    - QSPI
    - UART
    - I²C
    - I²S
    - SD/SDIO/MMC
    - GPIOs
    - CAN
    - ADC
    - S/PDIF
  - Debug

    - JTAG 10-pin connector

More information about the board can be found at the
`Toradex website`_.

Supported Features
==================

The Zephyr ``verdin_imx8mm/mimx8mm6/m4`` board target supports the following hardware
features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| SYSTICK   | on-chip    | systick                             |
+-----------+------------+-------------------------------------+
| CLOCK     | on-chip    | clock_control                       |
+-----------+------------+-------------------------------------+
| PINMUX    | on-chip    | pinmux                              |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port-polling;                |
|           |            | serial port-interrupt               |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | GPIO output                         |
|           |            | GPIO input                          |
+-----------+------------+-------------------------------------+

The default configuration can be found in the defconfig file:
:zephyr_file:`boards/toradex/verdin_imx8mm/verdin_imx8mm_mimx8mm6_m4_defconfig`.

It is recommended to disable peripherals used by the M4 core on the Linux host.

Other hardware features are not currently supported by the port.

Connections and IOs
===================

UART:

Zephyr is configured to use the UART4 by default, which is connected to the FTDI
USB converter on most Toradex carrier boards.

This is also the UART connected to WiFi/BT chip in modules that have the WiFi/BT
chip. Therefore, if UART4 is used, WiFI/BT will not work properly.

If the WiFi/BT is needed, then another UART should be used for Zephyr (UART2 for
example). You can change the UART by changing the ``zephyr,console`` and
``zephyr,shell-uart`` in the :zephyr_file:`boards/toradex/verdin_imx8mm/verdin_imx8mm_mimx8mm6_m4.dts` file.

+---------------+-----------------+---------------------------+
| Board Name    | SoC Name        | Usage                     |
+===============+=================+===========================+
| UART_1        | UART2           | General purpose UART      |
+---------------+-----------------+---------------------------+
| UART_2        | UART3           | General purpose UART      |
+---------------+-----------------+---------------------------+
| UART_3        | UART1           | Cortex-A53 debug UART     |
+---------------+-----------------+---------------------------+
| UART_4        | UART4           | Cortex-M4 debug UART      |
+---------------+-----------------+---------------------------+

GPIO:

All the GPIO banks are enabled in the :zephyr_file:`dts/arm/nxp/nxp_imx8m_m4.dtsi`.

LED:

There is no LED in the module itself, this is dependent on the carrier board that
is being used with the module. The device tree is configured to use the GPIO3_IO1,
which can be connected to the LED of the Verdin Development Board or changed in the
:zephyr_file:`boards/toradex/verdin_imx8mm/verdin_imx8mm_mimx8mm6_m4.dts` if needed.

System Clock
============

The M4 Core is configured to run at a 400 MHz clock speed.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The i.MX8MM doesn't have QSPI flash for the M4 and it needs
to be started by the A53 core. The A53 core is responsible to load the M4 binary
application into the RAM, putting the M4 in reset, setting the M4 Program Counter and
Stack Pointer, and get the M4 out of reset. The A53 can perform these steps at the
bootloader level or after the Linux system has booted via RemoteProc.

The M4 can use up to 3 different RAMs. These are the memory mapping for A53 and M4:

+------------+-------------------------+------------------------+-----------------------+----------------------+
| Region     | Cortex-A53              | Cortex-M4 (System Bus) | Cortex-M4 (Code Bus)  | Size                 |
+============+=========================+========================+=======================+======================+
| OCRAM      | 0x00900000-0x0093FFFF   | 0x20200000-0x2023FFFF  | 0x00900000-0x0093FFFF | 256KB                |
+------------+-------------------------+------------------------+-----------------------+----------------------+
| TCMU       | 0x00800000-0x0081FFFF   | 0x20000000-0x2001FFFF  |                       | 128KB                |
+------------+-------------------------+------------------------+-----------------------+----------------------+
| TCML       | 0x007E0000-0x007FFFFF   |                        | 0x1FFE0000-0x1FFFFFFF | 128KB                |
+------------+-------------------------+------------------------+-----------------------+----------------------+
| OCRAM_S    | 0x00180000-0x00187FFF   | 0x20180000-0x20187FFF  | 0x00180000-0x00187FFF | 32KB                 |
+------------+-------------------------+------------------------+-----------------------+----------------------+

For more information about memory mapping see the
`i.MX 8M Applications Processor Reference Manual`_  (section 2.1.2 and 2.1.3)

At compilation time you have to choose which RAM will be used. This
configuration is done in the file :zephyr_file:`boards/toradex/verdin_imx8mm/verdin_imx8mm_mimx8mm6_m4.dts`
with "zephyr,flash" (when CONFIG_XIP=y) and "zephyr,sram" properties.
The available configurations are:

.. code-block:: none

   "zephyr,flash"
   - &tcml_code
   - &ocram_code
   - &ocram_s_code

   "zephyr,sram"
   - &tcmu_sys
   - &ocram_sys
   - &ocram_s_sys

Starting the Cortex-M4 via U-Boot
=================================

Load and run Zephyr on M4 from A53 using u-boot by copying the compiled
``zephyr.bin`` to the eMMC (can be the FAT or EXT4 partition). You can do it
by using a USB stick or through the ethernet with the scp command, for example.
Power it up and stop at the u-boot prompt.

Load the M4 binary onto the desired memory and start its execution using:

.. code-block:: console

   fatload mmc 0:1 ${loadaddr} zephyr.bin
   cp.b ${loadaddr} 0x7e0000 <size_of_binary_in_bytes>
   bootaux 0x7e0000

Or if the binary is on the ext4 partition:

.. code-block:: console

   ext4load mmc 0:2 ${loadaddr} /path/to/zephyr.bin
   cp.b ${loadaddr} 0x7e0000 <size_of_binary_in_bytes>
   bootaux 0x7e0000

If you are using `TorizonCore`_ OS, then you should use partition 1:

.. code-block:: console

   ext4load mmc 0:1 ${loadaddr} /path/to/zephyr.bin
   cp.b ${loadaddr} 0x7e0000 <size_of_binary_in_bytes>
   bootaux 0x7e0000

Starting the Cortex-M4 via RemoteProc
=====================================

Copy the ``zepyhr.elf`` to ``/lib/firmware`` on the target.

.. note::
   In order to use remoteproc you have to add ``imx8mm-verdin_hmp_overlay.dtbo`` at
   the end of the line in the ``/boot/overlays.txt``, then reboot the target. If
   you are using `TorizonCore`_, then this file is located at
   ``/boot/ostree/torizon-<hash>/dtb/overlays.txt``.

To load and start a firmware use these commands:

.. code-block:: console

   verdin-imx8mm:~# echo zepyhr.elf > /sys/class/remoteproc/remoteproc0/firmware
   verdin-imx8mm:~# echo start > /sys/class/remoteproc/remoteproc0/state
   [   94.714498] remoteproc remoteproc0: powering up imx-rproc
   [   94.720481] remoteproc remoteproc0: Booting fw image zephyr.elf, size 473172
   [   94.727713] remoteproc remoteproc0: No resource table in elf
   [   94.733615] remoteproc remoteproc0: remote processor imx-rproc is now up

The M4-Core is now started up and running. You can see the output from Zephyr
on UART4.

Debugging
=========

MIMX8MM EVK board can be debugged by connecting an external JLink
JTAG debugger to the J902 debug connector and to the PC. Then
the application can be debugged using the usual way.

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: verdin_imx8mm/mimx8mm6/m4
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v3.4.0-1251-g43c549305bdb ***
   Hello World! verdin_imx8mm_m4

.. _Toradex website:
   https://developer.toradex.com/hardware/verdin-som-family/modules/verdin-imx8m-mini/

.. _i.MX 8M Applications Processor Reference Manual:
   https://www.nxp.com/webapp/Download?colCode=IMX8MMRM

.. _TorizonCore:
   https://developer.toradex.com/torizon/
