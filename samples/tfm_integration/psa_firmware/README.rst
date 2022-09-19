.. _tfm_psa_firmware:

TF-M PSA Firmware
#################

Overview
********
This TF-M integration example demonstrates how to use the PSA Firmware API
to retrieve information about the current firmware, or implement a custom
firmware update process.

Trusted Firmware (TF-M) Platform Security Architecture (PSA) APIs
are used for the secure processing environment, with Zephyr running in the
non-secure processing environment.

It uses **IPC Mode** for communication, where an IPC mechanism is inserted to
handle secure TF-M API calls and responses. The OS-specific code to handle
the IPC calls is in ``tfm_ipc.c``.

TF-M supports three types of firmware upgrade mechanisms:
``https://tf-m-user-guide.trustedfirmware.org/docs/technical_references/design_docs/tfm_secure_boot.html#firmware-upgrade-operation``

This example uses the overwrite firmware upgrade mechanism, in particular, it showcases
upgrading the non-secure image which can be built from any other sample in the
``zephyr/samples`` directory.

The sample prints test info to the console either as a single-thread or
multi-thread application.


Building and Running
********************

This project needs another firmware as the update payload. It must use another
example's hex file, and should be specified on the command line
as ``CONFIG_APP_FIRMWARE_UPDATE_IMAGE``.

To use the ``tfm_integration/tfm_ipc`` sample as the NS firmware update
payload, follow the instructions below:

This sample will only build on a Linux or macOS development system
(not Windows), and has been tested on the following setups:

- macOS Big Sur using QEMU 6.0.0 with gcc-arm-none-eabi-9-2020-q2-update
- Linux (NixOS) using QEMU 6.2.0 with gcc from Zephyr SDK 0.14.1
- Targets ``MPS3 AN547`` and ``NXP LPCXPRESSO55S69``

On MPS3 AN547:
===============

Build:
======

1. Build the ``tfm_ipc`` sample with the non-secure board configuration, which will
generate the firmware image we'll use in ``psa_firmware`` during the update:

.. zephyr-app-commands::
   :zephyr-app: samples/tfm_integration/tfm_ipc
   :host-os: unix
   :board: mps3_an547_ns
   :goals: build
   :build-dir: build/tfm_ipc
   :compact:

2. Build psa_firmware

.. zephyr-app-commands::
   :zephyr-app: samples/tfm_integration/psa_firmware
   :host-os: unix
   :board: mps3_an547_ns
   :goals: build
   :build-dir: build/psa_firmware
   :gen-args: -DCONFIG_APP_FIRMWARE_UPDATE_IMAGE=\"full/path/to/zephyr/build/tfm_ipc/zephyr/zephyr.hex\"
   :compact:

Note:
This sample includes a pre-built firmware image (``hello-an547.hex``) in the ``boards``
directory. If you don't pass the ``CONFIG_APP_FIRMWARE_UPDATE_IMAGE`` command
line argument during step 2 (``-- -DCONFIG_APP_FIRMWARE_UPDATE_IMAGE=...``),
this sample will automatically uses the pre-built hex file.

Run in real target:
===================

1. Copy application binary files (mcuboot.bin and tfm_sign.bin) to
   ``<MPS3 device name>/SOFTWARE/``.

2. Edit (e.g., with vim) the ``<MPS3 device name>/MB/HBI0263C/AN547/images.txt``
   file, and update it as shown below:

   .. code-block:: bash

      TITLE: Versatile Express Images Configuration File

      [IMAGES]
      TOTALIMAGES: 2 ;Number of Images (Max: 32)

      IMAGE0ADDRESS: 0x10000000
      IMAGE0FILE: \SOFTWARE\mcuboot.bin  ; BL2 bootloader

      IMAGE1ADDRESS: 0x10080000
      IMAGE1FILE: \SOFTWARE\tfm_sign.bin ; TF-M with application binary blob

3. Save the file, exit the editor, and reset the MPS3 board.

Run in QEMU:
============

.. zephyr-app-commands::
   :zephyr-app: samples/tfm_integration/psa_firmware
   :host-os: unix
   :board: mps3_an547_ns
   :goals: run
   :build-dir: build/psa_firmware
   :gen-args: -DCONFIG_APP_FIRMWARE_UPDATE_IMAGE=\"full/path/to/zephyr/build/tfm_ipc/zephyr/zephyr.hex\"
   :compact:

On LPCxpresso55S69:
===================

1. Build the ``tfm_ipc`` sample with the non-secure board configuration, which will
generate the firmware image we'll use in ``psa_firmware`` during the update:

.. zephyr-app-commands::
   :zephyr-app: samples/tfm_integration/tfm_ipc
   :host-os: unix
   :board: lpcxpresso55s69_ns
   :goals: build
   :build-dir: build/tfm_ipc
   :compact:

2. Build psa_firmware:

.. zephyr-app-commands::
   :zephyr-app: samples/tfm_integration/psa_firmware
   :host-os: unix
   :board: lpcxpresso55s69_ns
   :goals: build
   :build-dir: build/psa_firmware
   :gen-args: -DCONFIG_APP_FIRMWARE_UPDATE_IMAGE=\"full/path/to/zephyr/build/tfm_ipc/zephyr/zephyr.hex\"
   :compact:

Make sure your board is set up with :ref:`lpclink2-jlink-onboard-debug-probe`,
since this isn't the debug interface boards ship with from the factory;

Next we need to manually flash the resulting image (``tfm_merged.bin``) with a
J-Link as follows:

   .. code-block:: console

      JLinkExe -device lpc55s69 -if swd -speed 2000 -autoconnect 1
      J-Link>r
      J-Link>erase
      J-Link>loadfile build/tfm_merged.bin

Resetting the board and erasing it will unlock the board, this is useful in case
it's in an unknown state and can't be flashed.

We need to reset the board manually after flashing the image to run this code.

Sample Output
=============

   .. code-block:: console

      [INF] Beginning TF-M provisioning
      [WRN] TFM_DUMMY_PROVISIONING is not suitable for production! This device is NOT SECURE
      [Sec Thread] Secure image initializing!
      Booting TF-M v1.6.0+8cffe127
      Creating an empty ITS flash layout.
      Creating an empty PS flash layout.
      *** Booting Zephyr OS build zephyr-v3.1.0-3851-g2bef8051b2fc  ***
      PSA Firmware API test
      Active S image version: 0.0.3-0
      Active NS image version: 0.0.1-0
      Starting FWU; Writing Firmware from 21000000 size 17802 bytes
      Wrote Firmware; Writing Header from 2100458a size    16 bytes
      Wrote Header; Installing Image
      Installed New Firmware; Reboot Needed; Rebooting
      [WRN] This device was provisioned with dummy keys. This device is NOT SECURE
      [Sec Thread] Secure image initializing!
      Booting TF-M v1.6.0+8cffe127
      *** Booting Zephyr OS build zephyr-v3.1.0-3851-g2bef8051b2fc  ***
      The version of the PSA Framework API is 257.
      The minor version is 1.
      Connect success!
      TF-M IPC on mps3_an547

Common Problems
***************

Compilation fails with ``Error: Header padding was not requested...``
=====================================================================

This error occurs when passing a signed image to ``CONFIG_APP_FIRMWARE_UPDATE_IMAGE``
on the command line, ex: ``zephyr_ns_signed.hex``.
Make sure you pass an unsigned, non-secure image (ex. ``zephyr.hex``) to ``CONFIG_APP_FIRMWARE_UPDATE_IMAGE``.
