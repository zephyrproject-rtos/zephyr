.. zephyr:board:: imx8mp_var_som

Overview
********

Variscite's VAR-SOM-MX8M-PLUS System on Module (SoM) is based on the i.MX 8M Plus family,
which is a set of NXP products built to achieve both high performance and low power
consumption and relies on a powerful, fully coherent core complex based on a quad Cortex®-A53
cluster and Cortex®-M7 low-power coprocessor, audio digital signal processor, machine learning
and graphics accelerators.

Zephyr OS is ported to run on either the Cortex®-A53 or the Cortex®-M7.

Specs Summary
*************

  - CPU

    - NXP i.MX8M Plus:
    - Up to 4x Cortex®-A53 @ 1.8GHz
    - 1x Cortex®-M7 @ 800 MHz
    - 1x NPU 2.3 TOPS
  - Memory

    - Up to 8GB LPDDR4 RAM @ 2000MHz
    - 8-bit up to 128GB eMMC boot and storage
  - GPU

    - 3D: Vivante™ GC7000UltraLite (2 shaders) OpenGL ES 3.0, OpenCL1.2, Vulkan
    - 2D: Vivante™ GC520L
  - NPU (Neural Processing Unit)

    - 2.3 TOPS Neural Network performance
  - Display

    - 2x LVDS interface 4-lane each up to 1080p60
    - HDMI 2.0a up to 4Kp30
    - 1x MIPI DSI with up to 4 data lanes 1080p60
  - Network

    - 2x 10/100/1000 Mbit/s Ethernet Interface
    - Certified Wi-Fi 6 dual-band 802.11ax/ac/a/b/g/n with optional 802.15.4
    - Bluetooth/BLE 5.4
  - Camera

    - Up to 2x MIPI CSI – CMOS Serial camera Interface 4 lanes
    - 375 Mpixel/s HDR ISP (Image Sensor Processor)
  - Audio

    - Headphones
    - Microphone: Digital, Analog (stereo)
    - 6x I2S(SAI), S/PDIF RX TX, PDM 8CH, Line In/Out
  - USB

    - 2x USB 3.0/2.0 Host/Device
  - Serial interfaces

    - SPI: x3
    - I2C: x5
    - UART: x4, up to 5 Mbps
    - CAN: x2
  - Temperature range

    - -40°C to 85°C

More information about the SoM can be found at the
`Variscite Wiki`_ and
`Variscite website`_.

Supported Features
******************

.. zephyr:board-supported-hw::

.. note::

   It is recommended to disable peripherals used by the M7 core on the Linux host.

Devices
========
System Clock
------------

This board configuration uses a system clock frequency of 8 MHz.

The M7 core is configured to run at an 800 MHz clock speed.

Serial Port
-----------

This board configuration uses a single serial communication channel with the
CPU's UART4.

Programming and Debugging (A53)
*******************************

Copy the compiled ``zephyr.bin`` to the boot directory of the SD card and
plug the SD card into the board. Power it up and stop the U-Boot execution at
prompt.

Use U-Boot to load and run zephyr.bin on the Cortex-A53:

.. code-block:: console

   load mmc $mmcdev:$mmcpart $loadaddr /boot/zephyr.bin
   dcache flush; icache flush; go $loadaddr

Use this configuration to run basic Zephyr applications and kernel tests,
for example, with the :zephyr:code-sample:`hello_world` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: imx8mp_var_som/mimx8ml8/a53
   :goals: build

This will build an image with the hello_world sample app. When loaded and executed
it will display the following ram console output:

.. code-block:: console

   *** Booting Zephyr OS build v4.0.0-3113-g5aeda6fe7dfa ***
   Hello World! imx8mp_var_som/mimx8ml8/a53


Programming and Debugging (M7)
******************************

.. zephyr:board-supported-runners::

The VAR-SOM-MX8M-PLUS don't have QSPI flash for the M7, and it needs to be
started by the A53 core. The A53 core is responsible to load the M7 binary
application into the RAM, put the M7 in reset, set the M7 Program Counter and
Stack Pointer, and get the M7 out of reset. The A53 can perform these steps at
bootloader level or after the Linux system has booted.

The M7 can use up to 3 different RAMs (currently, only two configurations are
supported: ITCM and DDR). These are the memory mapping for A53 and M7:

+------------+-------------------------+------------------------+-----------------------+----------------------+
| Region     | Cortex-A53              | Cortex-M7 (System Bus) | Cortex-M7 (Code Bus)  | Size                 |
+============+=========================+========================+=======================+======================+
| OCRAM      | 0x00900000-0x0098FFFF   | 0x20200000-0x2028FFFF  | 0x00900000-0x0098FFFF | 576KB                |
+------------+-------------------------+------------------------+-----------------------+----------------------+
| DTCM       | 0x00800000-0x0081FFFF   | 0x20000000-0x2001FFFF  |                       | 128KB                |
+------------+-------------------------+------------------------+-----------------------+----------------------+
| ITCM       | 0x007E0000-0x007FFFFF   |                        | 0x00000000-0x0001FFFF | 128KB                |
+------------+-------------------------+------------------------+-----------------------+----------------------+
| OCRAM_S    | 0x00180000-0x00188FFF   | 0x20180000-0x20188FFF  | 0x00180000-0x00188FFF | 36KB                 |
+------------+-------------------------+------------------------+-----------------------+----------------------+
| DDR        | 0x80000000-0x803FFFFF   | 0x7B200000-0x7B3FFFFF  | 0x7B000000-0x7B1FFFFF | 2MB                  |
+------------+-------------------------+------------------------+-----------------------+----------------------+

For more information about memory mapping see the
`i.MX 8M Applications Processor Reference Manual`_  (section 2.1 to 2.3)

At compilation time you have to choose which RAM will be used. This
configuration is done based on board name (e.g. imx8mp_var_som/mimx8ml8/m7
for ITCM and imx8mp_var_som/mimx8ml8/m7/ddr for DDR).

There are two methods to load M7 Core images: U-Boot command and Linux remoteproc.

Load and Run M7 Zephyr Image from U-Boot
========================================

Load and run Zephyr on M7 from A53 using U-Boot by copying the compiled
``zephyr.bin`` to the boot directory of the SD card and plug the SD
card into the board. Power it up and stop the U-Boot execution at prompt.

Load the M7 binary onto the desired memory and start its execution using:

ITCM
====

.. code-block:: console

   load mmc 1:1 0x48000000 /boot/zephyr.bin
   cp.b 0x48000000 0x7e0000 20000
   bootaux 0x7e0000

DDR
===

.. code-block:: console

   load mmc 1:1 0x7b000000 /boot/zephyr.bin
   dcache flush
   bootaux 0x7b000000

Load and Run M7 Zephyr Image by using Linux remoteproc
======================================================

Transfer built binaries ``zephyr.bin`` and ``zephyr.elf`` to the SoM's ``/boot`` and
``/lib/firmware`` respectively using ``scp`` or through an USB drive.

It is possible to execute Zephyr binaries using Variscite remoteproc scripts made
for MCUXpresso binaries:

.. code-block:: console

   root@imx8mp-var-dart:~# /etc/remoteproc/variscite-rproc-linux -f /lib/firmware/zephyr.elf
   [  212.888118] remoteproc remoteproc0: powering up imx-rproc
   [  212.899215] remoteproc remoteproc0: Booting fw image zephyr.elf, size 515836
   [  212.912070] remoteproc remoteproc0: No resource table in elf
   [  213.444675] remoteproc remoteproc0: remote processor imx-rproc is now up

Which should yield the following result on the UART4 serial console:

.. code-block:: console

   *** Booting Zephyr OS build v4.0.0-3113-g5aeda6fe7dfa ***
   Hello World! imx8mp_var_som/mimx8ml8/m7

If the device tree dedicated to be used with Cortex-M7 applications is not being
currently used, the script will give instructions on how to do so:

.. code-block:: console

   Error: /sys/class/remoteproc/remoteproc0 not found.
   Please enable remoteproc driver.
   Most likely you need to use the correct device tree, for example:
   fw_setenv fdt_file imx8mp-var-som-symphony-m7.dtb && reboot

You can also configure U-Boot to load firmware on boot:

.. code-block:: console

   root@imx8mp-var-dart:~# /etc/remoteproc/variscite-rproc-u-boot -f /boot/zephyr.bin
   Configuring for TCM memory
   + fw_setenv m7_addr 0x7E0000
   + fw_setenv fdt_file imx8mp-var-som-symphony-m7.dtb
   + fw_setenv use_m7 yes
   + fw_setenv m7_bin zephyr.bin

   Finished: Please reboot, the m7 firmware will run during U-Boot

For more information about Variscite remoteproc scripts and general Cortex-M7
support, visit `Variscite Wiki`_.

Debugging
=========

VAR-SOM-MX8M-PLUS board can be debugged by connecting an external
JLink JTAG debugger to the 14-pin header on the top left side of
the SoM and to the PC. Then the application can be debugged using
the usual way.

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: imx8mp_var_som/mimx8ml8/m7
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v4.0.0-3113-g5aeda6fe7dfa ***
   Hello World! imx8mp_var_som/mimx8ml8/m7

References
**********

- `Variscite Wiki`_
- `Variscite website`_
- `i.MX 8M Applications Processor Reference Manual`_

.. _Variscite Wiki:
   https://variwiki.com/index.php?title=VAR-SOM-MX8M-PLUS

.. _Variscite website:
   https://www.variscite.com/product/system-on-module-som/cortex-a53-krait/var-som-mx8m-plus-nxp-i-mx-8m-plus

.. _i.MX 8M Applications Processor Reference Manual:
   https://www.nxp.com/webapp/Download?colCode=IMX8MPRM
