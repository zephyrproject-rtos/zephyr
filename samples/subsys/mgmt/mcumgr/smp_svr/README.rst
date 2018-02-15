.. _smp_svr_sample:

SMP Server Sample
################################

Overview
********

An application that implements a Simple Management Protocol (SMP) server.   SMP
is a basic transfer encoding for use with the mcumgr management protocol.  For
more information about mcumgr and SMP, please see
:file:`ext/lib/mgmt/mcumgr/README.md` in the Zephyr tree.

This sample application supports the following mcumgr transports by default:
    * Shell
    * Bluetooth

`smp_svr` enables support for the following command groups:
    * fs_mgmt
    * img_mgmt
    * os_mgmt
    * stat_mgmt

Caveats
*******

The Zephyr port of `smp_svr` is configured to run on an nRF52 MCU.  The
application should build and run for other platforms without modification, but
the file system management commands will not work.  To enable file system
management for a different platform, adjust the `CONFIG_FS_NFFS_FLASH_DEV_NAME`
setting in `prj.conf` accordingly.

In addition, the MCUboot boot loader (https://github.com/runtimeco/mcuboot) is
required for img_mgmt to function properly.

Building and Running
********************

The below steps describe how to build and run the `smp_svr` sample app in
Zephyr.  Where examples are given, they assume the following setup:

* BSP: Nordic nRF52dk
* MCU: PCA10040

If you would like to use a more constrained platform, such as the nRF51, you
should use the `prj.conf.tiny` configuration file rather than the default
`prj.conf`.

Step 1: Build MCUboot
=====================

Build MCUboot by following the instructions in its `docs/readme-zephyr.md`
file.

Step 2: Upload MCUboot
======================

Upload the resulting `zephyr.bin` file to address 0 of your board.  This can be
done in gdb as follows:

```
restore <path-to-mcuboot-zephyr.bin> binary 0
```

Step 3: Build smp_svr
=====================

`smp_svr` can be built for the nRF52 as follows:

.. zephyr-app-commands::
    :zephyr-app: samples/mgmt/mcumgr/smp_svr
    :board: nrf52_pca10040
    :build-dir: nrf52_pca10040
    :goals: build

Step 4: Create an MCUboot-compatible image
==========================================

Using MCUboot's `imgtool.py` script, convert the `zephyr.bin` file from step 3
into an image file.  In the below example, the MCUboot repo is located at
`~/repos/mcuboot`.

.. code-block:: console

    ~/repos/mcuboot/scripts/imgtool.py sign \
        --header-size 0x200 \
        --align 8 \
        --version 1.0 \
        --included-header \
        --key ~/repos/mcuboot/root-rsa-2048.pem \
        <path-to-smp_svr-zephyr.bin> signed.img

The above command creates an image file called `signed.img` in the current
directory.

Step 5: Upload the smp_svr image
================================

Upload the `signed.img` file from step 4 to image slot 0 of your board.  The
location of image slot 0 varies by BSP.  For the nRF52dk, slot 0 is located at
address 0xc000.

The following gdb command uploads the image to 0xc000:
.. code-block:: console

    restore <path-to-signed.img> binary 0xc000

Step 7: Run it!
===============

The `smp_svr` app is ready to run.  Just reset your board and test the app with
the mcumgr CLI tool:

.. code-block:: console

    $ mcumgr --conntype ble --connstring peer_name=Zephyr echo hello
    hello
