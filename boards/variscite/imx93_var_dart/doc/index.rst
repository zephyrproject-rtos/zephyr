.. zephyr:board:: imx93_var_dart

Overview
********

The DART-MX93 offers a high-performance processing for a low-power System-on-Module.
The product is based on the i.MX 93 family which represents NXP’s latest power-optimized
processors for smart home, building control, contactless HMI, IoT edge, and Industrial
applications.

The i.MX 93 includes powerful dual Arm® Cortex®-A55 processors with speeds up to 1.7 GHz
integrated with a NPU that accelerates machine learning inference. A general-purpose Arm®
Cortex®-M33 running up to 250 MHz is for real-time and low-power processing. Robust control
networks are possible via CAN-FD interface. Also, dual 1 Gbps Ethernet controllers, one
supporting Time Sensitive Networking (TSN), drive gateway applications with low latency.

Zephyr OS is ported to run on either the Cortex®-A55 or the Cortex®-M33.

Specs Summary
*************

  - CPU

    - NXP i.MX 93:
    - 2x Cortex®-A55 @ 1.7GHz
    - 1x Cortex®-M33 @ 250 MHz
    - 1x Ethos-U65 microNPU 0.5 TOPS
  - Memory

    - Up to 2GB LPDDR4 RAM
  - GPU

    - PXP 2D Pixel acceleration engine
  - NPU (Neural Processing Unit)

    - Neural Network performance (256 MACs operating up to 1.0 GHz and 2 OPS/MAC)
    - NPU targets 8-bit and 16-bit integer RNN
    - Handles 8-bit weights
  - Display

    - LVDS up to 1366x768p60 or 1280x800p60
    - Parallel RGB up to 1366x768p60 or 1280x800p60
    - 1x MIPI DSI up to 1920x1200p60 24-bit
  - Network

    - 2x 10/100/1000 Mbit/s Ethernet Interface
    - Certified Wi-Fi 802.11ax/ac/a/b/g/n
    - Bluetooth/BLE 5.4
  - Camera

    - One 2-lane MIPI CSI-2 camera input
  - Audio

    - Headphones
    - Microphone: Digital, Analog (stereo)
    - 3x I2S(SAI), S/PDIF, PDM 4CH
  - USB

    - 2x USB 2.0 OTG
  - Serial interfaces

    - SPI: x7
    - I2C: x7
    - UART: x7, up to 5 Mbps
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

   It is recommended to disable peripherals used by the M33 core on the Linux host.

Devices
========
System Clock
------------

This board configuration uses a system clock frequency of 24 MHz.
Cortex-A55 Core runs up to 1.7 GHz.
Cortex-M33 Core runs up to 200MHz in which SYSTICK runs on same frequency.

Serial Port
-----------

This board configuration uses a single serial communication channel with the
CPU's UART7 for A55 core and M33 core.

Programming and Debugging (A55)
*******************************

Copy the compiled ``zephyr.bin`` to the boot directory of the SD card and
plug the SD card into the board. Power it up and stop the U-Boot execution at
prompt.

Use U-Boot to load and run zephyr.bin on the Cortex-A55:

.. code-block:: console

   load mmc $mmcdev:$mmcpart $loadaddr /boot/zephyr.bin
   dcache off; icache flush; go $loadaddr

Use this configuration to run basic Zephyr applications and kernel tests,
for example, with the :zephyr:code-sample:`hello_world` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: imx93_var_dart/mimx9352/a55
   :goals: build

This will build an image with the hello_world sample app. When loaded and executed
it will display the following ram console output:

.. code-block:: console

   *** Booting Zephyr OS build v4.0.0-44-g93cbaccbbc41 ***
   Hello World! imx93_var_dart/mimx9352/a55


Programming and Debugging (M33)
*******************************

.. zephyr:board-supported-runners::

There are two methods to load M33 Core images: U-Boot command and Linux remoteproc.

Load and Run M33 Zephyr Image from U-Boot
=========================================

Load and run Zephyr on M33 from A55 using U-Boot by copying the compiled
``zephyr.bin`` to the boot directory of the SD card and plug the SD
card into the board. Power it up and stop the U-Boot execution at prompt.

Load the M33 binary onto the desired memory and start its execution using:

.. code-block:: console

   load mmc $mmcdev:$mmcpart 0x80000000 /boot/zephyr.bin
   cp.b 0x80000000 0x201e0000 0x30000
   bootaux 0x1ffe0000 0

Load and Run M33 Zephyr Image by using Linux remoteproc
=======================================================

Transfer built binaries ``zephyr.bin`` and ``zephyr.elf`` to the SoM's ``/boot`` and
``/lib/firmware`` respectively using ``scp`` or through an USB drive.

Before running Cortex-M33 binaries from Linux it is necessary to enable the device tree
dedicated to be used with Cortex-M33 applications:

.. code-block:: console

   root@imx93-var-som:~# fw_setenv fdt_file imx93-var-dart-dt8mcustomboard-m33.dtb
   root@imx93-var-som:~# reboot

It is possible to execute Zephyr binaries using Variscite remoteproc scripts made
for MCUXpresso binaries:

.. code-block:: console

   root@imx93-var-som:~# /etc/remoteproc/variscite-rproc-linux -f /lib/firmware/zephyr.elf
   [  125.449838] remoteproc remoteproc0: powering up imx-rproc
   [  125.459162] remoteproc remoteproc0: Booting fw image zephyr.elf, size 469356
   [  125.468958] remoteproc remoteproc0: No resource table in elf
   [  125.987142] remoteproc remoteproc0: remote processor imx-rproc is now up

Which should yield the following result on the UART7 serial console:

.. code-block:: console

   *** Booting Zephyr OS build v4.0.0-44-g93cbaccbbc41 ***
   Hello World! imx93_var_dart/mimx9352/m33

You can also configure U-Boot to load firmware on boot:

.. code-block:: console

   root@imx93-var-som:~# /etc/remoteproc/variscite-rproc-u-boot -f /boot/zephyr.bin
   Configuring for TCM memory
   + fw_setenv m33_addr 0x201E0000
   + fw_setenv fdt_file imx93-var-dart-dt8mcustomboard-m33.dtb
   + fw_setenv use_m33 yes
   + fw_setenv m33_bin zephyr.bin

   Finished: Please reboot, the m33 firmware will run during U-Boot

For more information about Variscite remoteproc scripts and general Cortex-M33
support, visit `Variscite Wiki`_.

References
**********

- `Variscite Wiki`_
- `Variscite website`_
- `NXP website`_

.. _Variscite Wiki:
   https://variwiki.com/index.php?title=DART-MX93

.. _Variscite website:
   https://www.variscite.com/product/system-on-module-som/cortex-a55/dart-mx93-nxp-i-mx93/

.. _NXP website:
   https://www.nxp.com/products/processors-and-microcontrollers/arm-processors/i-mx-applications-processors/i-mx-9-processors/i-mx-93-applications-processor-family-arm-cortex-a55-ml-acceleration-power-efficient-mpu:i.MX93
