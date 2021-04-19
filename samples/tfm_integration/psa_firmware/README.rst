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

This project outputs startup status and info to the console. It can be built and
executed on an ARM Cortex M33 target board or QEMU.

This sample will only build on a Linux or macOS development system
(not Windows), and has been tested on the following setups:

- macOS Big Sur using QEMU 6.0.0 with gcc-arm-none-eabi-9-2020-q2-update

On MPS2+ AN521:
===============

1. Build Zephyr with a non-secure configuration
   (``-DBOARD=mps2_an521_ns``).

   Using ``west``

   .. code-block:: bash

      cd <ZEPHYR_ROOT>
      west build -p -b mps2_an521_ns samples/tfm_integration/psa_firmware

   Using ``cmake`` and ``ninja``

   .. code-block:: bash

      cd <ZEPHYR_ROOT>/samples/tfm_integration/psa_firmware/
      rm -rf build
      mkdir build && cd build
      cmake -GNinja -DBOARD=mps2_an521_ns ..
      ninja

   Using ``cmake`` and ``make``

   .. code-block:: bash

      cd <ZEPHYR_ROOT>/samples/tfm_integration/psa_firmware/
      rm -rf build
      mkdir build && cd build
      cmake -DBOARD=mps2_an521_ns ..
      make

2. Copy application binary files (mcuboot.bin and tfm_sign.bin) to
   ``<MPS2 device name>/SOFTWARE/``.

3. Edit (e.g., with vim) the ``<MPS2 device name>/MB/HBI0263C/AN521/images.txt``
   file, and update it as shown below:

   .. code-block:: bash

      TITLE: Versatile Express Images Configuration File

      [IMAGES]
      TOTALIMAGES: 2 ;Number of Images (Max: 32)

      IMAGE0ADDRESS: 0x10000000
      IMAGE0FILE: \SOFTWARE\mcuboot.bin  ; BL2 bootloader

      IMAGE1ADDRESS: 0x10080000
      IMAGE1FILE: \SOFTWARE\tfm_sign.bin ; TF-M with application binary blob

4. Save the file, exit the editor, and reset the MPS2+ board.

On QEMU:
========

Build Zephyr with a non-secure configuration (``-DBOARD=mps2_an521_ns``)
and run it in qemu via the ``run`` command.

   Using ``west``

   .. code-block:: bash

      cd <ZEPHYR_ROOT>
      west build -p -b mps2_an521_ns samples/tfm_integration/psa_firmware -t run

   Using ``cmake`` and ``ninja``

   .. code-block:: bash

      cd <ZEPHYR_ROOT>/samples/tfm_integration/psa_firmware/
      rm -rf build
      mkdir build && cd build
      cmake -GNinja -DBOARD=mps2_an521_ns ..
      ninja run

   Using ``cmake`` and ``make``

   .. code-block:: bash

      cd <ZEPHYR_ROOT>/samples/tfm_integration/psa_firmware/
      rm -rf build
      mkdir build && cd build
      cmake -DBOARD=mps2_an521_ns ..
      make run

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

On nRF5340 and nRF9160:
=======================

Build Zephyr with a non-secure configuration
(``-DBOARD=nrf5340dk_nrf5340_cpuappns`` or ``-DBOARD=nrf9160dk_nrf9160ns``).

   Example, for nRF9160, using ``cmake`` and ``ninja``

   .. code-block:: bash

      cd <ZEPHYR_ROOT>/samples/tfm_integration/psa_firmware/
      rm -rf build
      mkdir build && cd build
      cmake -GNinja -DBOARD=nrf9160dk_nrf9160ns ..

If building with BL2 (MCUboot bootloader) enabled, manually flash
the MCUboot bootloader image binary (``bl2.hex``).

   Example, using ``nrfjprog`` on nRF9160:

   .. code-block:: bash

      nrfjprg -f NRF91 --program tfm/bin/bl2.hex --sectorerase

Finally, flash the concatenated TF-M + Zephyr binary.

   Example, for nRF9160, using ``cmake`` and ``ninja``

   .. code-block:: bash

      ninja flash


Sample Output
=============

   .. code-block:: console

      TODO!
