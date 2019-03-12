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

``smp_svr`` enables support for the following command groups:

    * ``fs_mgmt``
    * ``img_mgmt``
    * ``os_mgmt``
    * ``stat_mgmt``

Caveats
*******

* The Zephyr port of ``smp_svr`` is configured to run on a Nordic nRF52x MCU. The
  application should build and run for other platforms without modification, but
  the file system management commands will not work.  To enable file system
  management for a different platform, adjust the
  :option:`CONFIG_FS_NFFS_FLASH_DEV_NAME` setting in :file:`prj.conf` accordingly.

* The MCUboot bootloader is required for ``img_mgmt`` to function
  properly. More information about the Device Firmware Upgrade subsystem and
  MCUboot can be found in :ref:`mcuboot`.

* The :file:`mcumgr` command-line tool only works with Bluetooth Low Energy (BLE)
  on Linux and macOS. On Windows there is no support for Device Firmware
  Upgrade over BLE yet.

Building a BLE Controller (optional)
************************************

.. note::
   This section is only relevant for Linux users

If you want to try out Device Firmware Upgrade (DFU) over the air using
Bluetooth Low Energy (BLE) and do not have a built-in or pluggable BLE radio,
you can build one and use it following the instructions in
:ref:`bluetooth-hci-uart-bluez`.

Building and Running
********************

The below steps describe how to build and run the ``smp_svr`` sample in
Zephyr. Where examples are given, they assume the sample is being built for
the Nordic nRF52 Development Kit (``BOARD=nrf52_pca10040``).

If you would like to use a more constrained platform, such as the nRF51 DK, you
should use the :file:`prj_tiny.conf` configuration file rather than the default
:file:`prj.conf`.

Step 1: Build MCUboot
=====================

Build MCUboot by following the instructions in the :ref:`mcuboot`
documentation page.

Step 2: Flash MCUboot
======================

Flash the resulting image file to address 0x0 of flash memory.
This can be done in multiple ways.

Using make or ninja:

.. code-block:: console

   make flash
   # or
   ninja flash

Using GDB:

.. code-block:: console

   restore <path-to-mcuboot-zephyr.bin> binary 0

Step 3: Build smp_svr
=====================

``smp_svr`` can be built for the nRF52 as follows:

.. zephyr-app-commands::
    :zephyr-app: samples/subsys/mgmt/mcumgr/smp_svr
    :board: nrf52_pca10040
    :build-dir: nrf52_pca10040
    :goals: build

.. _smp_svr_sample_sign:

Step 4: Sign the image
======================

.. note::
   From this section onwards you can use either a binary (``.bin``) or an
   Intel Hex (``.hex``) image format. This is written as ``(bin|hex)`` in this
   document.

Using MCUboot's :file:`imgtool.py` script, sign the :file:`zephyr.(bin|hex)`
file you built in Step 3. In the below example, the MCUboot repo is located at
:file:`~/src/mcuboot`.

.. code-block:: console

   ~/src/mcuboot/scripts/imgtool.py sign \
        --key ~/src/mcuboot/root-rsa-2048.pem \
        --header-size 0x200 \
        --align 8 \
        --version 1.0 \
        --slot-size <image-slot-size> \
        <path-to-zephyr.(bin|hex)> signed.(bin|hex)

The above command creates an image file called :file:`signed.(bin|hex)` in the
current directory.

Step 5: Flash the smp_svr image
===============================

Upload the :file:`signed.(bin|hex)` file from Step 4 to image slot-0 of your
board.  The location of image slot-0 varies by board, as described in
:ref:`mcuboot_partitions`.  For the nRF52 DK, slot-0 is located at address
``0xc000``.

Using :file:`nrfjprog` you don't need to specify the slot-0 starting address,
since :file:`.hex` files already contain that information:

.. code-block:: console

    nrfjprog --program <path-to-signed.hex>

Using GDB:

.. code-block:: console

    restore <path-to-signed.bin> binary 0xc000

Step 6: Run it!
===============

.. note::
   If you haven't installed :file:`mcumgr` yet, then do so by following the
   instructions in the :ref:`mcumgr_cli` section of the Management subsystem
   documentation.

.. note::
   The :file:`mcumgr` command-line tool requires a connection string in order
   to identify the remote target device. In this sample we use a BLE-based
   connection string, and you might need to modify it depending on the
   BLE controller you are using.


The ``smp_svr`` app is ready to run.  Just reset your board and test the app
with the :file:`mcumgr` command-line tool's ``echo`` functionality, which will
send a string to the remote target device and have it echo it back:

.. code-block:: console

   sudo mcumgr --conntype ble --connstring ctlr_name=hci0,peer_name='Zephyr' echo hello
   hello


Step 7: Device Firmware Upgrade
===============================

Now that the SMP server is running on your board and you are able to communicate
with it using :file:`mcumgr`, you might want to test what is commonly called
"OTA DFU", or Over-The-Air Device Firmware Upgrade.

To do this, build a second sample (following the steps below) to verify
it is sent over the air and properly flashed into slot-1, and then
swapped into slot-0 by MCUboot.

Build a second sample
---------------------

Perhaps the easiest sample to test with is the :zephyr_file:`samples/hello_world`
sample provided by Zephyr, documented in the :ref:`hello_world` section.

Edit :zephyr_file:`samples/hello_world/prj.conf` and enable the required MCUboot
Kconfig option as described in :ref:`mcuboot` by adding the following line to
it:

.. code-block:: console

   CONFIG_BOOTLOADER_MCUBOOT=y

Then build the sample as usual (see :ref:`hello_world`).

Sign the second sample
----------------------

Next you will need to sign the sample just like you did for :file:`smp_svr`,
since it needs to be loaded by MCUboot.
Follow the same instructions described in :ref:`smp_svr_sample_sign`,
but this time you must use a :file:`.bin` image, since :file:`mcumgr` does not
yet support :file:`.hex` files.

Upload the image over BLE
-------------------------

Now we are ready to send or upload the image over BLE to the target remote
device.

.. code-block:: console

   sudo mcumgr --conntype ble --connstring ctlr_name=hci0,peer_name='Zephyr' image upload signed.bin

If all goes well the image will now be stored in slot-1, ready to be swapped
into slot-0 and executed.

.. note::

   At the beginning of the upload process, the target might start erasing
   the image slot, taking several dozen seconds for some targets.  This might
   cause an NMP timeout in the management protocol tool. Use the
   ``-t <timeout-in-seconds`` option to increase the response timeout for the
   ``mcumgr`` command line tool if this occurs.

List the images
---------------

We can now obtain a list of images (slot-0 and slot-1) present in the remote
target device by issuing the following command:

.. code-block:: console

   sudo mcumgr --conntype ble --connstring ctlr_name=hci0,peer_name='Zephyr' image list

This should print the status and hash values of each of the images present.

Test the image
--------------

In order to instruct MCUboot to swap the images we need to test the image first,
making sure it boots:

.. code-block:: console

   sudo mcumgr --conntype ble --connstring ctlr_name=hci0,peer_name='Zephyr' image test <hash of slot-1 image>

Now MCUBoot will swap the image on the next reset.

Reset remotely
--------------

We can reset the device remotely to observe (use the console output) how
MCUboot swaps the images:

.. code-block:: console

   sudo mcumgr --conntype ble --connstring ctlr_name=hci0,peer_name='Zephyr' reset

Upon reset MCUboot will swap slot-0 and slot-1.

The new image is the basic ``hello_world`` sample that does not contain
SMP or BLE functionality, so we cannot communicate with it using
:file:`mcumgr`. Instead simply reset the board manually to force MCUboot
to revert (i.e. swap back the images) due to the fact that the new image has
not been confirmed.

If you had instead built and uploaded a new image based on ``smp_svr``
(or another BLE and SMP enabled sample), you could confirm the
new image and make the swap permanent by using this command:

.. code-block:: console

   sudo mcumgr --conntype ble --connstring ctlr_name=hci0,peer_name='Zephyr' image confirm

Note that if you try to send the very same image that is already flashed in
slot-0 then the procedure will not complete successfully since the hash values
for both slots will be identical.

