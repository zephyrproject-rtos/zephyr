.. _smp_svr_sample:

SMP Server Sample
#################

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

* The :file:`mcumgr` command-line tool only works with Bluetooth Low Energy (BLE)
  on Linux and macOS. On Windows there is no support for Device Firmware
  Upgrade over BLE yet.

Prerequisites
*************

Installing the mcumgr cli
=========================

To interact remotely with the management subsystem on a device, we need to have the
:file:`mcumgr` installed. Follow the instructions in the :ref:`mcumgr_cli` section
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
we can build and flash it like normal using ``west``, like so:

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

      The sample application comes in two bluetooth flavours: a normal one and a tiny one
      for resource constrained bluetooth devices.

      To build the normal bluetooth sample:

      .. code-block:: console

         west build \
            -b nrf52dk_nrf52832 \
            samples/subsys/mgmt/mcumgr/smp_svr \
            -- \
            -DOVERLAY_CONFIG=overlay-bt.conf

      And to build the tiny bluetooth sample:

      .. code-block:: console

         west build \
            -b nrf51dk_nrf51422 \
            samples/subsys/mgmt/mcumgr/smp_svr \
            -- \
            -DOVERLAY_CONFIG=overlay-bt-tiny.conf

   .. group-tab:: Serial

      To build the serial sample with file-system and shell management support:

      .. code-block:: console

         west build \
            -b frdm_k64f \
            samples/subsys/mgmt/mcumgr/smp_svr \
            -- \
            -DOVERLAY_CONFIG='overlay-serial.conf;overlay-fs.conf;overlay-shell-mgmt.conf'

   .. group-tab:: USB CDC_ACM

      To build the serial sample with USB CDC_ACM backend:

      .. code-block:: console

         west build \
            -b nrf52840dk_nrf52840 \
            samples/subsys/mgmt/mcumgr/smp_svr \
            -- \
            -DOVERLAY_CONFIG=overlay-cdc.conf

   .. group-tab:: Shell

      To build the shell sample:

      .. code-block:: console

         west build \
            -b frdm_k64f \
            samples/subsys/mgmt/mcumgr/smp_svr \
            -- \
            -DOVERLAY_CONFIG='overlay-shell.conf'

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
            -DOVERLAY_CONFIG=overlay-udp.conf

.. _smp_svr_sample_sign:

Signing the sample image
************************

A key feature of MCUboot is that images must be signed before they can be successfully
uploaded and run on a target. To sign images, the MCUboot tool :file:`imgtool` can be used.

To sign the sample image we built in a previous step:

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

To upload the initial image file to an empty slot-0, we simply use ``west flash``
like normal. ``west flash`` will automatically detect slot-0 address and confirm
the image.

.. code-block:: console

    west flash --bin-file build/zephyr/zephyr.signed.bin

We need to explicity specify the *signed* image file, otherwise the non-signed version
will be used and the image wont be runnable.

Sample image: hello world!
==========================

The ``smp_svr`` app is ready to run.  Just reset your board and test the app
with the :file:`mcumgr` command-line tool's ``echo`` functionality, which will
send a string to the remote target device and have it echo it back:

.. tabs::

   .. group-tab:: Bluetooth

      .. code-block:: console

         sudo mcumgr --conntype ble --connstring ctlr_name=hci0,peer_name='Zephyr' echo hello
         hello

   .. group-tab:: Shell

      .. code-block:: console

         mcumgr --conntype serial --connstring "/dev/ttyACM0,baud=115200" echo hello
         hello

   .. group-tab:: UDP

      Using IPv4:

      .. code-block:: console

         mcumgr --conntype udp --connstring=[192.168.1.1]:1337 echo hello
         hello

      And using IPv6

      .. code-block:: console

         mcumgr --conntype udp --connstring=[2001:db8::1]:1337 echo hello
         hello

.. note::
   The :file:`mcumgr` command-line tool requires a connection string in order
   to identify the remote target device. In the BT sample we use a BLE-based
   connection string, and you might need to modify it depending on the
   BLE controller you are using.

.. note::
   In the following sections, examples will use ``<connection string>`` to represent
   the ``--conntype <type>`` and ``--connstring=<string>`` :file:`mcumgr` parameters.

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

Upload the signed image
=======================

To upload the signed image, use the following command:

.. code-block:: console

   sudo mcumgr <connection string> image upload build/zephyr/zephyr.signed.bin

.. note::

   At the beginning of the upload process, the target might start erasing
   the image slot, taking several dozen seconds for some targets.  This might
   cause an NMP timeout in the management protocol tool. Use the
   ``-t <timeout-in-seconds`` option to increase the response timeout for the
   ``mcumgr`` command line tool if this occurs.

List the images
===============

We can now obtain a list of images (slot-0 and slot-1) present in the remote
target device by issuing the following command:

.. code-block:: console

   sudo mcumgr <connection string> image list

This should print the status and hash values of each of the images present.

Test the image
==============

In order to instruct MCUboot to swap the images we need to test the image first,
making sure it boots:

.. code-block:: console

   sudo mcumgr <connection string> image test <hash of slot-1 image>

Now MCUBoot will swap the image on the next reset.

.. note::
   There is not yet any way of getting the image hash without actually uploading the
   image and getting the hash by using the ``image list`` command of :file:`mcumgr`.

Reset remotely
==============

We can reset the device remotely to observe (use the console output) how
MCUboot swaps the images:

.. code-block:: console

   sudo mcumgr <connection string> reset

Upon reset MCUboot will swap slot-0 and slot-1.

Confirm new image
=================

The new image is now loaded into slot-0, but it will be swapped back into slot-1
on the next reset unless the image is confirmed. To confirm the new image:

.. code-block:: console

   sudo mcumgr <connection string> image confirm

Note that if you try to send the very same image that is already flashed in
slot-0 then the procedure will not complete successfully since the hash values
for both slots will be identical.
