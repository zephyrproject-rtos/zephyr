.. _usb_dfu:

USB DFU Sample Application
##########################

Overview
********

This sample app demonstrates use of a USB DFU Class driver provided
by the Zephyr project.

Requirements
************

This project requires an USB device driver. Currently, the USB DFU
class provided by the Zephyr project depends on DFU image manager and
partition layout. Refer to :ref:`flash_map_api` for details about
partition layout. You SoC must run MCUboot as the stage 1 bootloader.
This sample is built as an application for the MCUboot bootloader.

Building and Testing
********************

Building and signing the application
====================================

This sample can be built in the usual way (see :ref:`build_an_application`
for more details) and flashed with regular flash tools, but will need
to be loaded at the offset of SLOT-0.

Application images (such as this sample) must be signed.
Use the ``scripts/imagetool.py`` script from the `MCUboot GitHub repo`_
to sign the image.  (See the `Using MCUboot with Zephyr`_ documentation for
details.)

.. code-block:: console

   ~/src/mcuboot/scripts/imgtool.py sign \
           --key ~/src/mcuboot/root-rsa-2048.pem \
           --header-size 0x200 \
           --align 8 \
           --version 1.2 \
           --slot-size 0x60000 \
           ./zephyr/zephyr.bin \
           signed-zephyr.bin

Build and flash MCUboot bootloader for Zephyr project as it is described in
the `Using MCUboot with Zephyr`_ documentation. Then build, sign and flash
the USB DFU sample at the offset of SLOT-0.

Build and sign a second application image e.g. :ref:`hello_world`,
which will be used as an image for the update.
Do not forget to enable the required MCUboot Kconfig option (as described
in :ref:`mcuboot`) by adding the following line to
:zephyr_file:`samples/hello_world/prj.conf`:

.. code-block:: console

   CONFIG_BOOTLOADER_MCUBOOT=y

Testing
=======

The Linux ``dfu-util`` tool can be used to backup or update the application
image.

Use the following command to backup the SLOT-0 image:

.. code-block:: console

   dfu-util --alt 0 --upload slot0_backup.bin

Use the following command to update the application:

.. code-block:: console

   dfu-util --alt 1 --download signed-hello.bin

Reset the SoC. MCUboot boot will swap the images and boot the new application,
showing this output to the console:

.. code-block:: console

  ***** Booting Zephyr OS v1.1.0-65-g4ec7f76 *****
  [MCUBOOT] [INF] main: Starting bootloader
  [MCUBOOT] [INF] boot_status_source: Image 0: magic=unset, copy_done=0xff, image_ok=0xff
  [MCUBOOT] [INF] boot_status_source: Scratch: magic=unset, copy_done=0xe, image_ok=0xff
  [MCUBOOT] [INF] boot_status_source: Boot source: slot 0
  [MCUBOOT] [INF] boot_swap_type: Swap type: test
  [MCUBOOT] [INF] main: Bootloader chainload address offset: 0x20000
  [MCUBOOT] [INF] main: Jumping to the first image slot0
  ***** Booting Zephyr OS v1.11.0-830-g9df01813c4 *****
  Hello World! arm

Reset the SoC again and MCUboot should revert the images and boot
USB DFU sample, showing this output to the console:

.. code-block:: console

  ***** Booting Zephyr OS v1.1.0-65-g4ec7f76 *****
  [MCUBOOT] [INF] main: Starting bootloader
  [MCUBOOT] [INF] boot_status_source: Image 0: magic=good, copy_done=0x1, image_ok=0xff
  [MCUBOOT] [INF] boot_status_source: Scratch: magic=unset, copy_done=0xe, image_ok=0xff
  [MCUBOOT] [INF] boot_status_source: Boot source: none
  [MCUBOOT] [INF] boot_swap_type: Swap type: revert
  [MCUBOOT] [INF] main: Bootloader chainload address offset: 0x20000
  ***** Booting Zephyr OS v1.11.0-830-g9df01813c4 *****

.. _MCUboot GitHub repo: https://github.com/runtimeco/mcuboot
.. _Using MCUboot with Zephyr: https://mcuboot.com/mcuboot/readme-zephyr.html
