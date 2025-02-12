.. zephyr:code-sample:: smp-svr
   :name: SMP server

   Implement a Simple Management Protocol (SMP) server.

Overview
********

This sample application implements a Simple Management Protocol (SMP) server.
SMP is a basic transfer encoding for use with the MCUmgr management protocol.
For more information about MCUmgr and SMP, please see :ref:`device_mgmt`.

This sample application supports the following mcumgr transports by default:

    * Shell
    * Bluetooth
    * UDP

``smp_svr`` enables support for the following command groups:

    * ``fs_mgmt``
    * ``img_mgmt``
    * ``os_mgmt``
    * ``stat_mgmt``
    * ``shell_mgmt``

Caveats
*******

* The MCUboot bootloader is required for ``img_mgmt`` to function
  properly. More information about the Device Firmware Upgrade subsystem and
  MCUboot can be found in :ref:`mcuboot`.

Prerequisites
*************

Use of a tool
=============

To interact remotely with the management subsystem on a device, a tool is required to interact with
it. There are various tools available which are listed on the :ref:`mcumgr_tools_libraries` page
of the Management subsystem documentation.

Building a BLE Controller
=========================

.. note::
   This section is only relevant for Linux users

If you want to try out Device Firmware Upgrade (DFU) over the air using
Bluetooth Low Energy (BLE) and do not have a built-in or pluggable BLE radio,
you can build one and use it following the instructions in
:ref:`bluetooth-hci-uart-bluez`.

Building and flashing MCUboot
*****************************

The below steps describe how to build and run the MCUboot bootloader.
Detailed instructions can be found in the :ref:`mcuboot` documentation page.

The Zephyr port of MCUboot is essentially a normal Zephyr application, which means that
it can be built and flashed like normal using ``west``, like so:

.. code-block:: console

   west build -b <board> -d build_mcuboot bootloader/mcuboot/boot/zephyr
   west flash -d build_mcuboot

Substitute <board> for one of the boards supported by the sample, see
:file:`sample.yaml`.

.. _smp_svr_sample_build:

Building the sample application
*******************************

The below steps describe how to build and run the ``smp_svr`` sample in
Zephyr. The ``smp_svr`` sample comes in different flavours.

.. tabs::

   .. group-tab:: Bluetooth

      To build the bluetooth sample:

      .. code-block:: console

         west build \
            -b nrf52dk/nrf52832 \
            samples/subsys/mgmt/mcumgr/smp_svr \
            -- \
            -DEXTRA_CONF_FILE=overlay-bt.conf

   .. group-tab:: Serial

      To build the serial sample with file-system and shell management support:

      .. code-block:: console

         west build \
            -b frdm_k64f \
            samples/subsys/mgmt/mcumgr/smp_svr \
            -- \
            -DEXTRA_CONF_FILE='overlay-serial.conf;overlay-fs.conf;overlay-shell-mgmt.conf'

   .. group-tab:: USB CDC_ACM

      To build the serial sample with USB CDC_ACM backend:

      .. code-block:: console

         west build \
            -b nrf52840dk/nrf52840 \
            samples/subsys/mgmt/mcumgr/smp_svr \
            -- \
            -DEXTRA_CONF_FILE=overlay-cdc.conf \
            -DDTC_OVERLAY_FILE=usb.overlay

   .. group-tab:: Shell

      To build the shell sample:

      .. code-block:: console

         west build \
            -b frdm_k64f \
            samples/subsys/mgmt/mcumgr/smp_svr \
            -- \
            -DEXTRA_CONF_FILE='overlay-shell.conf'

   .. group-tab:: UDP

      The UDP transport for SMP supports both IPv4 and IPv6.
      In the sample, both IPv4 and IPv6 are enabled, but they can be
      enabled and disabled separately.

      To build the UDP sample:

      .. code-block:: console

         west build \
            -b frdm_k64f \
            samples/subsys/mgmt/mcumgr/smp_svr \
            -- \
            -DEXTRA_CONF_FILE=overlay-udp.conf

.. _smp_svr_sample_sign:

Signing the sample image
************************

A key feature of MCUboot is that images must be signed before they can be successfully
uploaded and run on a target. To sign images, the MCUboot tool :file:`imgtool` can be used.

To sign the sample image built in the previous step:

.. code-block:: console

    west sign -t imgtool -- --key bootloader/mcuboot/root-rsa-2048.pem

The above command creates an image file called :file:`zephyr.signed.bin` in the
build directory.

For more information on image signing and ``west sign``, see the :ref:`west-sign`
documentation.

Flashing the sample image
*************************

Upload the :file:`zephyr.signed.bin` file from the previous to image slot-0 of your
board.  See :ref:`flash_map_api` for details on flash partitioning.

To upload the initial image file to an empty slot-0, use ``west flash`` like normal.
``west flash`` will automatically detect slot-0 address and confirm the image.

.. code-block:: console

    west flash --bin-file build/zephyr/zephyr.signed.bin

The *signed* image file needs to be used specifically, otherwise the non-signed version
will be used and the image won't be runnable.

Sample image: hello world!
==========================

The ``smp_svr`` app is ready to run.  Just reset your board and test the app
with your choice of tool's ``echo`` functionality, which will
send a string to the remote target device and have it echo it back.

J-Link Virtual MSD Interaction Note
***********************************

On boards where a J-Link OB is present which has both CDC and MSC (virtual Mass
Storage Device, also known as drag-and-drop) support, the MSD functionality can
prevent mcumgr commands over the CDC UART port from working due to how USB
endpoints are configured in the J-Link firmware (for example on the Nordic
``nrf52840dk``) because of limiting the maximum packet size (most likely to occur
when using image management commands for updating firmware). This issue can be
resolved by disabling MSD functionality on the J-Link device, follow the
instructions on :ref:`nordic_segger_msd` to disable MSD support.

Device Firmware Upgrade (DFU)
*****************************

Now that the SMP server is running on your board and you are able to communicate
with it using :file:`mcumgr`, you might want to test what is commonly called
"OTA DFU", or Over-The-Air Device Firmware Upgrade. This works for both BT and UDP.

The general sequence of a DFU process is as follows:

* Build an MCUboot enabled application, see :ref:`smp_svr_sample_build`
* Sign the application image, see :ref:`smp_svr_sample_sign`
* Upload the signed image using :file:`mcumgr`
* Listing the images on the device using :file:`mcumgr`
* Mark the uploaded image for testing using :file:`mcumgr`
* Reset the device remotely using :file:`mcumgr`
* Confirm the uploaded image using :file:`mcumgr` (optional)

Direct image upload and Image mapping to MCUboot slot
=====================================================

Currently the mcumgr supports, for direct upload, 4 target images, of which first two are mapped
into MCUboot primary (slot-0) and secondary (slot-1) respectively.

For clarity, here is DTS label to slot to ``<image>`` translation table:

    +-----------+--------+------------+
    | DTS label | Slot   | -n <image> |
    +===========+========+============+
    | "image-0" | slot-0 |     1      |
    +-----------+--------+------------+
    | "image-1" | slot-1 |     0, 1   |
    +-----------+--------+------------+
    | "image-2" |        |     2      |
    +-----------+--------+------------+
    | "image-3" |        |     3      |
    +-----------+--------+------------+

Upload the signed image
=======================

To upload the signed image, refer to the documentation for your chosen tool, select the new
firmware file to upload and begin the upload.

.. note::

   At the beginning of the upload process, the target might start erasing
   the image slot, taking several dozen seconds for some targets.

List the images
===============

A list of images (slot-0 and slot-1) that are present can now be obtained on the remote target device using
the tool of your choice, which should print the status and hash values of each of the images
present.

Test the image
==============

In order to instruct MCUboot to swap the images, the image needs to be tested first, making sure it
boots, see the instructions in the tool of your choice. Upon reboot, MCUBoot will swap to the new
image.

.. note::
   There is not yet any way of getting the image hash without actually uploading the
   image and getting the hash.

Reset remotely
==============

The device can be reset remotely to observe (use the console output) how MCUboot swaps the images,
check the documentation in the tool of your choice. Upon reset MCUboot will swap slot-0 and
slot-1.

Confirm new image
=================

The new image is now loaded into slot-0, but it will be swapped back into slot-1 on the next
reset unless the image is confirmed. Confirm the image using the tool of your choice.

Note that if you try to send the very same image that is already flashed in
slot-0 then the procedure will not complete successfully since the hash values
for both slots will be identical.

Download file from File System
******************************

SMP server supports downloading files from File System on device via
:file:`mcumgr`. This is useful with FS log backend, when files are stored in
non-volatile memory. Build and flash both MCUboot and smp_svr applications and
then use the tool of your choice to download files from the file system.
