.. zephyr:code-sample:: usb-dfu
   :name: USB DFU
   :relevant-api: usbd_api usbd_dfu

   Implement a basic USB DFU device

Overview
********

This sample application demonstrates the USB DFU implementation using the
new experimental USB device stack.

Requirements
************

This project requires an experimental USB device driver (UDC API) and uses the
:ref:`disk_access_api` and RAM-disk to download/upload the image.

Building and Running
********************

This sample can be built for multiple boards, in this example we will build it
for the reel board:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/dfu
   :board: reel_board
   :goals: build flash
   :compact:

`dfu-util`_ tool can be used to download or upload the images. There are two
modes of operation in the USB DFU, runtime and DFU. The example starts in
runtime mode. To switch to DFU mode without uploading or downloading, the
following command can be used:

.. code-block:: console

   dfu-util --detach

Use the following command to upload the ``ramdisk0`` image to the host:

.. code-block:: console

   dfu-util --alt 0 --upload ramdisk0_backup.bin

Use the following command to download the ``ramdisk0`` image to the device:

.. code-block:: console

   dfu-util --alt 0 --download ramdisk0_backup.bin

Building with flash backend enabled
***********************************

The USB DFU device support has a built-in flash backend. This backend uses
:ref:`flash_img_api` and :ref:`flash_map_api` to write or read flash image, the
implementation is similar to the one we had in the previous USB DFU device
example.

To use flash backend set the :kconfig:option:`CONFIG_APP_USB_DFU_USE_FLASH_BACKEND`.
An additional interface will be available in DFU mode to upload/download the
SLOT-1 image.

It is also possible to try the sample together with the MCUboot bootloader
library. The following example shows how to build MCUboot and this sample with
flash backend and MCUboot support enabled using the :ref:`sysbuild`:

.. zephyr-app-commands::
   :tool: west
   :zephyr-app: samples/subsys/usb/dfu
   :board: reel_board
   :goals: build flash
   :west-args: --sysbuild
   :gen-args: -DSB_CONFIG_BOOTLOADER_MCUBOOT=y -DCONFIG_APP_USB_DFU_USE_FLASH_BACKEND=y

Another application image is required to be used as a firmware update and
downloaded to SLOT-1. Build and sign a second application image e.g.
:zephyr:code-sample:`hello_world`, which will be used as an image for the
update. Do not forget to enable the required :kconfig:option:`CONFIG_BOOTLOADER_MCUBOOT`
option (as described in :ref:`mcuboot`). For example:

.. zephyr-app-commands::
   :app: zephyr/samples/hello_world
   :board: reel_board
   :gen-args: -DCONFIG_MCUBOOT_SIGNATURE_KEY_FILE=\"bootloader/mcuboot/root-rsa-2048.pem\" -DCONFIG_BOOTLOADER_MCUBOOT=y
   :goals: flash

Use the following command to download new image to the device:

.. code-block:: console

   dfu-util --alt 1 --download build/zephyr/zephyr.signed.bin

Reset the SoC. MCUboot boot will swap the images and boot the new application,
showing this output to the console:

.. code-block:: console

   *** Booting MCUboot v2.1.0-rc1-134-gb9d69dd2a2d6 ***
   *** Using Zephyr OS build v3.7.0-4345-ga5d0d8533a41 ***
   I: Starting bootloader
   I: Primary image: magic=good, swap_type=0x4, copy_done=0x1, image_ok=0x1
   I: Secondary image: magic=good, swap_type=0x2, copy_done=0x3, image_ok=0x3
   I: Boot source: none
   I: Image index: 0, Swap type: test
   I: Starting swap using move algorithm.
   I: Bootloader chainload address offset: 0xc000
   I: Image version: v0.0.0
   I: Jumping to the first image slot
   *** Booting Zephyr OS build v3.7.0-4345-ga5d0d8533a41 ***
   Hello World! reel_board@1/nrf52840


Reset the SoC again and MCUboot should revert the images and boot
USB DFU sample, showing this output to the console:

.. code-block:: console

   *** Booting MCUboot v2.1.0-rc1-134-gb9d69dd2a2d6 ***
   *** Using Zephyr OS build v3.7.0-4345-ga5d0d8533a41 ***
   I: Starting bootloader
   I: Primary image: magic=good, swap_type=0x2, copy_done=0x1, image_ok=0x3
   I: Secondary image: magic=unset, swap_type=0x1, copy_done=0x3, image_ok=0x3
   I: Boot source: none
   I: Image index: 0, Swap type: revert
   I: Starting swap using move algorithm.
   I: Secondary image: magic=unset, swap_type=0x1, copy_done=0x3, image_ok=0x3
   I: Bootloader chainload address offset: 0xc000
   I: Image version: v0.0.0
   I: Jumping to the first image slot
   *** Booting Zephyr OS build v3.7.0-4345-ga5d0d8533a41 ***
   [00:00:00.000,335] <inf> main: USBD message: VBUS ready
   [00:00:00.000,427] <inf> main: USB DFU sample is initialized


.. _dfu-util: https://dfu-util.sourceforge.net/
.. _Using MCUboot with Zephyr: https://docs.mcuboot.com/readme-zephyr
