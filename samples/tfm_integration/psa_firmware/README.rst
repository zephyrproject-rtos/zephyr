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

The sample prints test info to the console either as a single-thread or
multi-thread application.

Building and Running
********************

This project needs another firmware as the update payload. It must use another
example's hex file, and should be specified on the command line
as ZEPHYR_FIRMWARE_UPDATE_SAMPLE_BUILD. For example, to use the sample
`tfm_integration/tfm_ipc` as the payload:

.. code-block:: bash

   cd <ZEPHYR_ROOT>
   west build -p -b lpcxpresso5s69_ns samples/tfm_integration/psa_firmware \
       -d build/lpcxpresso55s69_ns/tfm_integration/psa_firmware
       -- -DCONFIG_FIRMWARE_UPDATE_IMAGE=`realpath build/lpcxpresso55s69_ns/tfm_integration/tfm_ipc/zephyr_ns_signed.hex`


This project outputs startup status and info to the console. It can be built and
executed on an ARM Cortex M33 target board or QEMU.

This sample will only build on a Linux or macOS development system
(not Windows), and has been tested on the following setups:

- macOS Big Sur using QEMU 6.0.0 with gcc-arm-none-eabi-9-2020-q2-update
- Linux (NixOS) using QEMU 6.2.50 with gcc from Zephyr SDK 0.13.2

On MPS3 AN547:
===============

1. Build Zephyr with a non-secure configuration
   (``-DBOARD=mps3_an547_ns``).
   Using ``west``

.. zephyr-app-commands::
  :zephyr-app: samples/tfm_integration/psa_firmware
  :host-os: unix
  :board: mps3_an547_ns
  :goals: run
  :compact:

2. Copy application binary files (mcuboot.bin and tfm_sign.bin) to
   ``<MPS3 device name>/SOFTWARE/``.

3. Edit (e.g., with vim) the ``<MPS3 device name>/MB/HBI0263C/AN547/images.txt``
   file, and update it as shown below:

   .. code-block:: bash

      TITLE: Versatile Express Images Configuration File

      [IMAGES]
      TOTALIMAGES: 2 ;Number of Images (Max: 32)

      IMAGE0ADDRESS: 0x10000000
      IMAGE0FILE: \SOFTWARE\mcuboot.bin  ; BL2 bootloader

      IMAGE1ADDRESS: 0x10080000
      IMAGE1FILE: \SOFTWARE\tfm_sign.bin ; TF-M with application binary blob

4. Save the file, exit the editor, and reset the MPS3 board.

On QEMU:
========

Build Zephyr with a non-secure configuration (``-DBOARD=mps3_an547_ns``)
and run it in qemu via the ``run`` command.

Using ``west``

.. zephyr-app-commands::
  :zephyr-app: samples/tfm_integration/psa_firmware
  :host-os: unix
  :board: mps3_an547_ns
  :goals: run
  :compact:

On LPCxpresso55S69:
======================

Build Zephyr with a non-secure configuration:

   .. code-block:: bash

      $ west build -p -b lpcxpresso55s69_ns samples/tfm_integration/psa_firmware/ --

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
      TF-M FP mode: Software
      Booting TFM v1.5.0
      Creating an empty ITS flash layout.
      Creating an empty PS flash layout.
      *** Booting Zephyr OS build v3.0.0-rc1-321-gbe26b6a260d6  ***
      PSA Firmware API test
      Active NS image version: 0.0.0-0
      Starting FWU; Writing Firmware from 21000000 size 58466 bytes
      Wrote Firmware; Writing Header from 2100e462 size   432 bytes
      Wrote Header; Installing Image
      Installed New Firmware; Reboot Needed; Rebooting
      [WRN] This device was provisioned with dummy keys. This device is NOT SECURE
      [Sec Thread] Secure image initializing!
      TF-M FP mode: Software
      Booting TFM v1.5.0
      *** Booting Zephyr OS build v3.0.0-rc1-35-g03f2993ef07b  ***
      Hello World from UserSpace! mps3_an547
