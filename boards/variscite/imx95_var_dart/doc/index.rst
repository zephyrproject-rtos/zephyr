.. zephyr:board:: imx95_var_dart

Overview
********

The DART-MX95 is a high-performance, low-power System-on-Module based on NXP’s i.MX 95 applications
processor family. It targets demanding edge and industrial applications that require AI/ML
acceleration, advanced graphics, rich connectivity and functional safety.

The i.MX 95 integrates up to six Arm® Cortex®-A55 processors running at up to 2.0 GHz together with
an NPU for machine learning acceleration. Two real-time Arm® cores complete the heterogeneous
architecture: a Cortex®-M7 running up to 800 MHz for high-performance real-time tasks and a
Cortex®-M33 for system management, safety and low-power processing.

The SoM also features dual GbE plus 10GbE, PCIe Gen 3, high-speed USB, multiple display and camera
interfaces, and industrial temperature range, making it suitable for a wide variety of embedded
designs.

Zephyr OS is ported to run on either the Cortex®-A55 or the Cortex®-M7.

More information about the SoM can be found at the `Variscite Wiki`_ and `Variscite website`_.

Specs Summary
*************

  - CPU

    - NXP i.MX 95:
    - Up to 6x Cortex®-A55 @ 2.0 GHz
    - 1x Cortex®-M7 @ 800 MHz
    - 1x Cortex®-M33 (System Manager)
    - Neural Processing Unit (NPU) up to 2 TOPS

  - Memory

    - Up to 16GB LPDDR5 RAM

  - GPU

    - 2D/3D GPU with support for OpenGL® ES 3.2, Vulkan® 1.2 and OpenCL® 3.0
    - Hardware 4K video encode/decode

  - NPU (Neural Processing Unit)

    - AI/ML acceleration engine up to 2 TOPS
    - Optimized for 8-bit and 16-bit integer workloads

  - Display

    - 1x MIPI DSI up to 4K resolution
    - Dual-channel LVDS up to 1920x1080p60
    - Additional parallel / secondary display options depending on carrier

  - Network

    - 2x 10/100/1000 Mbit/s Ethernet interfaces
    - 1x 10GbE interface
    - Certified Wi-Fi 802.11ax/ac/a/b/g/n
    - Bluetooth/BLE 5.x and 802.15.4 (on Wi-Fi/Bluetooth module variants)

  - Camera

    - Up to 2x MIPI CSI-2 camera inputs (lane configuration depends on carrier)

  - Audio

    - Headphones / line out
    - Digital and analog microphone interfaces
    - Multiple I2S(SAI), S/PDIF, PDM and MQS outputs

  - USB

    - Up to 2x USB 3.0/2.0 OTG / Dual-Role ports (SoM + carrier dependent)

  - Serial interfaces

    - Up to 7x I2C
    - Up to 8x LPUART
    - Up to 8x SPI
    - 1x I3C
    - Up to 5x CAN-FD
    - SD/MMC, QSPI and other high-speed interfaces

  - Temperature range

    - -40°C to 85°C (industrial grade)

More information about the SoM can be found at the `Variscite Wiki`_ and `Variscite website`_.

Supported Features
******************

.. zephyr:board-supported-hw::

.. note::

   It is recommended to disable peripherals used by the Cortex-M7 core on the Linux host when
   running Zephyr on M7.

Devices
=======
System Clock
------------

This board configuration uses a system clock frequency of 24 MHz. Cortex-A55 cores can run up to 2.0
GHz. Cortex-M7 core can run up to 800 MHz, and the SYSTICK timer runs at the same frequency as the
M7 core clock.

Serial Port
-----------

On the standard DART-MX95 + DT8MCustomBoard setup, this board configuration uses:

  - LPUART1 as the primary debug serial console for the Cortex-A55 core
  - LPUART3 as the debug serial console for the Cortex-M7 core

Programming and Debugging (A55)
*******************************

Copy the compiled ``zephyr.bin`` to the boot directory of the SD card or eMMC device and plug it
into the DT8MCustomBoard. Power it up and stop the U-Boot execution at the prompt.

Use U-Boot to load and run ``zephyr.bin`` on the Cortex-A55:

.. code-block:: console

   load mmc $mmcdev:$mmcpart $loadaddr /boot/zephyr.bin
   dcache off; icache flush; go $loadaddr

Use this configuration to run basic Zephyr applications and kernel tests, for example, with the
:zephyr:code-sample:`hello_world` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: imx95_var_dart/mimx9596/a55
   :goals: build

This will build an image with the ``hello_world`` sample app. When loaded and executed it will
display the following console output on the A55 serial console:

.. code-block:: console

   *** Booting Zephyr OS build v4.0.0-xx-gxxxxxxxxxxxx ***
   Hello World! imx95_var_dart/mimx9596/a55


Programming and Debugging (M7)
*******************************

.. zephyr:board-supported-runners::

There are two main methods to load Cortex-M7 images on DART-MX95:

- Using U-Boot commands to load and start the Cortex-M7 firmware.
- Using the Linux ``remoteproc`` framework (manual sysfs control or Variscite helper scripts).

.. note::

   Do not enable both U-Boot auto-loading and Linux ``remoteproc`` control at the same time. If
   Linux is expected to load/control the M7, ensure U-Boot auto-loading is disabled.

Load and Run M7 Zephyr Image from U-Boot
========================================

Load and run Zephyr on M7 from the Cortex-A55 using U-Boot by copying the compiled ``zephyr.bin`` to
the boot directory of the SD card or eMMC device. Power it up and stop U-Boot at the prompt.

The Variscite U-Boot environment provides helper commands to load and start the Cortex-M7 firmware.
First, configure the M7 support and device tree once from Linux (optional but recommended):

.. code-block:: console

   root@imx95-var-dart:~# fw_setenv use_m7 yes
   root@imx95-var-dart:~# fw_setenv fdt_file imx95-var-dart-dt8mcustomboard-m7.dtb

Then, after copying ``zephyr.bin`` to ``/boot``, from U-Boot you can run:

.. code-block:: console

   setenv m7_bin zephyr.bin
   saveenv
   run loadm7bin
   run runm7bin

By default, the Variscite environment loads the M7 image to the alias of the M7 TCM code area (for
example ``0x203c0000``) and then starts it using ``bootaux``. The Zephyr console for M7 will appear
on the M7 UART (LPUART3 on DT8MCustomBoard).

Use this configuration to run basic Zephyr applications and kernel tests, for example, with the
:zephyr:code-sample:`hello_world` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: imx95_var_dart/mimx9596/m7
   :goals: build

When loaded and executed it will display the following console output on the M7 serial port:

.. code-block:: console

   *** Booting Zephyr OS build v4.0.0-xx-gxxxxxxxxxxxx ***
   Hello World! imx95_var_dart/mimx9596/m7

Load and Run M7 Zephyr Image from Linux
=======================================

The Linux ``remoteproc`` framework can be used to load the Cortex-M7 firmware from Linux userspace.
This can be done either manually via sysfs, or by using Variscite helper scripts.

.. note::

   The device index (for example ``remoteproc1``) may vary depending on the kernel and device tree.
   List available devices with:

   .. code-block:: console

      ls /sys/class/remoteproc/

Option A: Manual remoteproc control (sysfs)
-------------------------------------------

(Optional) increase kernel loglevel while debugging:

.. code-block:: console

   sysctl -w kernel.printk=7

If the state is ``running``, stop the Cortex-M7:

.. code-block:: console

   echo stop > /sys/class/remoteproc/remoteproc1/state

Load new firmware:

.. code-block:: console

   echo zephyr.elf > /sys/class/remoteproc/remoteproc1/firmware

The ``.elf`` file is expected to exist in the ``/lib/firmware`` directory.

Run the new firmware:

.. code-block:: console

   echo start > /sys/class/remoteproc/remoteproc1/state

Option B: Variscite helper scripts (recommended)
------------------------------------------------

Transfer the built binaries ``zephyr.bin`` and ``zephyr.elf`` to the SoM ``/boot`` and
``/lib/firmware`` respectively using ``scp`` or a USB drive.

Before running Cortex-M7 binaries from Linux, it is necessary to enable the device tree variant
dedicated to Cortex-M7 applications:

.. code-block:: console

   root@imx95-var-dart:~# fw_setenv fdt_file imx95-var-dart-dt8mcustomboard-m7.dtb
   root@imx95-var-dart:~# reboot

After rebooting into Linux with the *-m7* device tree, it is possible to execute Zephyr binaries
using Variscite remoteproc scripts originally designed for MCUXpresso firmware:

.. code-block:: console

   root@imx95-var-dart:~# /etc/remoteproc/variscite-rproc-linux -f /lib/firmware/zephyr.elf -c 1
   [  233.xxx] remoteproc remoteproc1: powering up imx-rproc
   [  233.xxx] remoteproc remoteproc1: Booting fw image zephyr.elf
   [  233.xxx] remoteproc remoteproc1: header-less resource table
   [  233.xxx] remoteproc remoteproc1: remote processor imx95-cm7 is now up

The Zephyr log should then appear on the Cortex-M7 UART (LPUART3), showing output similar to:

.. code-block:: console

   *** Booting Zephyr OS build v4.0.0-xx-gxxxxxxxxxxxx ***
   Hello World! imx95_var_dart/mimx9596/m7

Configure U-Boot to auto-load the M7 firmware (optional)
--------------------------------------------------------

You can also configure U-Boot to automatically load the M7 firmware on boot using the Variscite
helper script:

.. code-block:: console

   root@imx95-var-dart:~# /etc/remoteproc/variscite-rproc-u-boot -f /boot/zephyr.bin -c 1
   Configuring for M7 TCM memory
   + fw_setenv m7_addr 0x203c0000
   + fw_setenv fdt_file imx95-var-dart-dt8mcustomboard-m7.dtb
   + fw_setenv use_m7 yes
   + fw_setenv m7_bin zephyr.bin
   + fw_setenv m7_addr_auxview 0x00000000

   Finished: Please reboot, the M7 firmware will run during U-Boot

After this configuration, on every boot U-Boot will load the Cortex-M7 firmware from
``/boot/zephyr.bin`` before starting Linux.

For more information about Variscite remoteproc scripts and general Cortex-M7 support, visit
`Variscite Wiki`_.

References
**********

- `Variscite Wiki`_
- `Variscite website`_
- `NXP website`_

.. _Variscite Wiki:
   https://variwiki.com/index.php?title=DART-MX95

.. _Variscite website:
   https://www.variscite.com/product/system-on-module-som/cortex-a55/dart-mx95-nxp-i-mx-95/

.. _NXP website:
   https://www.nxp.com/products/processors-and-microcontrollers/arm-processors/i-mx-applications-processors/i-mx-9-processors/i-mx-95-applications-processors-family:i.MX95
