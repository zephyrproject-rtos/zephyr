.. zephyr:board:: rzg3e_smarc

Overview
********

The Renesas RZ/G3E Evaluation Board Kit (RZ/G3E-EVKIT) consists of a SMARC v2.1 module board and a carrier board.

* Device: RZ/G3E R9A09G047E57GBG

  * Cortex-A55 x 4, Cortex-M33 Single
  * FCBGA 529-pin, 15mmSq body, 0.5mm pitch

* SMARC v2.1 Module Board Functions

  * LPDDR4X SDRAM: 4GB x 1pc
  * QSPI flash memory: 16MB x 1pc
  * eMMC memory: 64GB x 1pc
  * PMIC power supply RAA215300A2GNP#HA8 implemented
  * microSD slot x2
  * Parallel interface
  * I3C connector
  * JTAG connector
  * ADC x8 channels
  * Current monitor (USB Micro B)

* Carrier Board Functions

  * Gigabit Ethernet x2
  * USB2.0 x 2ch (OTG x1ch, Host x2ch)
  * USB3.2 x 1ch (Host x1ch)
  * CAN-FD interface x2
  * MIPI CSI-2 camera interface
  * Micro HDMI, over MIPI-DS
  * LVDS interface x2
  * microSD slot
  * Mono speaker interface, stereo headphone, mic. and aux. interfaces
  * PMOD x3 (Type 3A and Type 6A are exclusive)
  * PCIe Gen3 4-lane slot (G3E supports only 2-lane)M.2 Key E interface
  * M.2 Key B interface and SIM card slot
  * USB Type-C power input

Hardware
********

The Renesas RZ/G3E MPU documentation can be found at `RZ/G3E Group Website`_

.. figure:: rzg3e_block_diagram.webp
	:width: 600px
	:align: center
	:alt: RZ/G3E group feature

	RZ/G3E block diagram (Credit: Renesas Electronics Corporation)

Detailed hardware features for the board can be found at `RZG3E-EVKIT Website`_

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``rzg3e_smarc`` board can be built in the usual way as
documented in :ref:`build_an_application`.

Console
=======

The UART port for Cortex-M33 System Core can be accessed by connecting `Pmod USBUART <https://store.digilentinc.com/pmod-usbuart-usb-to-uart-interface/>`_
to the upper side of ``PMOD1_3A``.

Debugging
=========

It is possible to load and execute a Zephyr application binary on
this board on the Cortex-M33 System Core from
the internal SRAM, using ``JLink`` debugger (:ref:`jlink-debug-host-tools`).

Here is an example for building and debugging with the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rzg3e_smarc/r9a09g047a57gbg/cm33
   :goals: build debug

Flashing
========

RZ/G3E-EVKIT is designed to start different systems on different cores.
It uses Yocto as the build system to build Linux system and boot loaders
to run Zephyr on Cortex-M33 with u-boot. The minimal steps are described below.

1. Follow ''2.1 Building Images'' of `RZ/G3E-EVKIT Linux Start-up Guide`_ to prepare the build environment.

2. At step (5), follow step ''2. Download Multi-OS Package'' and ''3. Add the layer for Multi-OS Package''
   of ''3.2 Integration of OpenAMP related stuff'' of `RZ/G3E Quick Start Guide for RZ/G3E Multi-OS Package`_
   to add the layer for Multi-OS Package.

.. code-block:: console

   $ cd ~/rzg_vlp_<pkg ver>
   $ unzip <Multi-OS Dir>/r01an5869ej0300-rzg-multi-os-pkg.zip
   $ tar zxvf r01an5869ej0300-rzg-multi-os-pkg/meta-rz-features_multi-os_v3.0.0.tar.gz
   $ cd build
   $ bitbake-layers add-layer ../meta-rz-features/meta-rz-multi-os/meta-rzg3e

3. Start the build:

.. code-block:: console

   $ MACHINE=smarc-rzg3e bitbake core-image-minimal

The below necessary artifacts will be located in the build/tmp/deploy/images

+---------------+------------------------------------------------------+
| Artifacts     | File name                                            |
+===============+======================================================+
| Boot loader   | bl2_bp_spi-smarc-rzg3e.srec                          |
|               |                                                      |
|               | fip-smarc-rzg3e.srec                                 |
+---------------+------------------------------------------------------+
| Flash Writer  | Flash_Writer_SCIF_RZG3E_EVK_LPDDR4X.mot              |
+---------------+------------------------------------------------------+
| SD card image | core-image-minimal-smarc-rzg3e.rootfs.wic.gz         |
|               |                                                      |
|               | core-image-minimal-smarc-rzg3e.rootfs.wic.bmap       |
+---------------+------------------------------------------------------+

4. Follow ''4.4 Startup Procedure for RZ/G3E'' of `RZ/G3E-EVKIT Linux Start-up Guide`_ for power supply and board setting
   at SCIF download (SW_MODE[1:4] = OFF, ON, OFF, ON) and Cortex-A55 cold boot (BOOT[1:6] = OFF, OFF, ON, OFF, OFF, OFF)

5. Follow ''4.5 Download Flash Writer to RAM'' of `RZ/G3E-EVKIT Linux Start-up Guide`_ to download Flash Writer to RAM

6. Follow ''4.6 Write the Bootloader'' of `RZ/G3E-EVKIT Linux Start-up Guide`_ to write the boot loader
   to the target board by using Flash Writer.

7. Follow ''4.7 Change Boot Mode and set U-Boot variables'' with switch setting (SW_MODE[1:4] = OFF, OFF, OFF, ON)

8. Follow ''3. Preparing the SD Card'' of `RZ/G3E-EVKIT Linux Start-up Guide`_ to write files to the microSD Card

9. Run the following commands to write **zephyr.bin** to SD card.

.. code-block:: console

   $ sudo mkdir /mnt/sd -p
   $ sudo mount /dev/sdb2 /mnt/sd
   $ sudo cp /path/to/zephyr.bin /mnt/sd/boot
   $ sync
   $ sudo umount /mnt/sd

.. warning::

   Change ``/dev/sdb`` to your microSD card device name. Use ``dh -h`` to check.

10. Follow "4.3.2 CM33 Sample Program Invocation from u-boot" from step 2 to step 4 of `RZ/G3E Quick Start Guide for RZ/G3E Multi-OS Package`_

11. Execute the commands stated below on the console to start zephyr application with CM33 core.

.. code-block:: console

   => setenv cm33start 'dcache off
   => mw.l 0x10420D2C 0x02000000
   => mw.l 0x1043080c 0x08003000
   => mw.l 0x10430810 0x18003000
   => mw.l 0x10420604 0x00040004
   => mw.l 0x10420C1C 0x00003100
   => mw.l 0x10420C0C 0x00000001
   => mw.l 0x10420904 0x00380008
   => mw.l 0x10420904 0x00380038
   => ext4load mmc 1:2 0x08003000 boot/zephyr.bin
   => mw.l 0x10420C0C 0x00000000
   => dcache on'
   => saveenv
   => run cm33start

References
**********

.. target-notes::

.. _RZ/G3E Group Website:
   https://www.renesas.com/en/products/rz-g3e

.. _RZG3E-EVKIT Website:
   https://www.renesas.com/en/design-resources/boards-kits/rz-g3e-evkit

.. _RZ/G3E-EVKIT Linux Start-up Guide:
   https://www.renesas.com/en/document/gde/rzg3e-linux-start-guide-rev100

.. _RZ/G3E Quick Start Guide for RZ/G3E Multi-OS Package:
   https://www.renesas.com/en/document/qsg/quick-start-guide-rzg3e-multi-os-package
