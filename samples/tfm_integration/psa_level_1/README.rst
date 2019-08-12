.. _tfm_psa_level_1:

TF-M PSA Level 1
################

Overview
********
This TF-M integration example can be used with an Arm v8-M supported board. It
implements the RTOS vendor requirements for a PSA Certified Level 1 product.

Trusted Firmware (TF-M) Platform Security Architecture (PSA) APIs
are used for the secure processing environment, with Zephyr running in the
non-secure processing environment.

As part of the standard build process, the secure bootloader (BL2) is built, as
well as the secure TF-M binary and non-secure Zephyr binary images. The two
application images are then merged and signed using the public/private key pair
stored in the secure bootloader, so that they will be accepted during the
image validation process at startup.

All TF-M dependencies will be automatically cloned in the `/ext` folder, and
built if appropriate build artefacts are not detected.

.. _PSA Certified Level 1:
  https://www.psacertified.org/security-certification/psa-certified-level-1/

.. note:: This example is based upon trusted-firmware-m commit 0d822496e809132e040c0a5cd556b2d5dad01ea6

Building and Running
********************

This project outputs startup status and info to the console. It can be built and
executed on an MPS2+ configured for AN521 (dual-core ARM Cortex M33).

At present, this sample is only compatible with Linux and OS X based systems,
and has been tested on the following setups:

- OS X 10.14.5 natively with gcc-arm-none-eabi-7-2018-q2-update
- Ubuntu 18.04 using Zephyr SDK 0.10.1

On MPS2+ AN521:
===============

1. Build Zephyr with a non-secure configuration (``-DBOARD=mps2_an521_nonsecure``). This will also build TF-M in the ``ext/tfm`` folder if necessary.

Using ``west``

.. code-block:: bash

   cd <ZEPHYR_ROOT>/samples/tfm_integration/psa_level_1/
   west build -p -b mps2_an521_nonsecure ./

Using ``cmake``

.. code-block:: bash

   cd <ZEPHYR_ROOT>/samples/tfm_integration/psa_level_1/
   mkdir build
   cd build
   cmake -GNinja -DBOARD=mps2_an521_nonsecure ..
   ninja -v

2. Merge the TF-M secure and Zephyr non-secure images, and sign the final image

.. code-block:: bash

   ./merge.sh

3. Copy application binary files (mcuboot.bin and tfm_sign.bin) to ``<MPS2 device name>/SOFTWARE/``.
4. Open ``<MPS2 device name>/MB/HBI0263C/AN521/images.txt``.
5. Update the ``AN521/images.txt`` file as follows:

.. code-block:: bash

   TITLE: Versatile Express Images Configuration File

   [IMAGES]
   TOTALIMAGES: 2 ;Number of Images (Max: 32)

   IMAGE0ADDRESS: 0x10000000
   IMAGE0FILE: \SOFTWARE\mcuboot.bin  ; BL2 bootloader

   IMAGE1ADDRESS: 0x10080000
   IMAGE1FILE: \SOFTWARE\tfm_sign.bin ; TF-M with application binary blob

8. Reset MPS2+ board.

Sample Output
=============

.. code-block:: console

   [INF] Starting bootloader
   [INF] Swap type: none
   [INF] Bootloader chainload address offset: 0x80000
   [INF] Jumping to the first image slot
   [Sec Thread] Secure image initializing!
   ***** Booting Zephyr OS zephyr-v1.14.0-1616-g3013993758d4 *****
   ...


.. _TF-M build instruction:
   https://git.trustedfirmware.org/trusted-firmware-m.git/tree/docs/user_guides/tfm_build_instruction.rst

.. _TF-M secure boot:
   https://git.trustedfirmware.org/trusted-firmware-m.git/tree/docs/user_guides/tfm_secure_boot.rst

Signing Images
==============

TF-M uses a secure bootloader (BL2) and firmware images must be signed
with a private key before execution can be handed off by the bootloader. The
firmware image is validated by the bootloader at startup using the public key,
which is built into the secure bootloader.

By default, `tfm/bl2/ext/mcuboot/root-rsa-3072.pem` is used to sign images.
`merge.sh` signs the TF-M + Zephyr binary using the .pem private key,
calling `imgtool.py` to perform the actual signing operation.

In order to satisfy PSA Level 1 certification requirements, **You MUST replace
the default .pem file with a new key pair!**

To generate a new public/private key pair, run the following commands from
the sample folder:

.. code-block:: bash

  $ chmod +x ../../../ext/tfm/tfm/bl2/ext/mcuboot/scripts/imgtool.py
  $ ../../../ext/tfm/tfm/bl2/ext/mcuboot/scripts/imgtool.py keygen -k root-rsa-3072.pem -t rsa-3072
  $ cp root-

You can then replace the .pem file in `/ext/tfm/tfm/bl2/ext/mcuboot/` with
the newly generated .pem file, and rebuild the bootloader so that it uses the
public key extracted from this new key file when validating firmware images.

.. code-block:: bash

  $ west build -p -b mps2_an521_nonsecure ./
  $ ./merge.sh

.. warning::

  Be sure to keep your private key file in a safe, reliable location! If you
  lose this key file, you will be unable to sign any future firmware images,
  and it will no longer be possible to update your devices in the field!
