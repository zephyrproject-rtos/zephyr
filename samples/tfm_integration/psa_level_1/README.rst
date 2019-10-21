.. _tfm_psa_level_1:

TF-M PSA Level 1
################

Overview
********
This TF-M integration example can be used with an Arm v8-M supported board. It
implements the RTOS vendor requirements for a `PSA Certified Level 1`_ product.

Trusted Firmware (TF-M) Platform Security Architecture (PSA) APIs
are used for the secure processing environment, with Zephyr running in the
non-secure processing environment.

As part of the standard build process, the secure bootloader (BL2) is built, as
well as the secure TF-M binary and non-secure Zephyr binary images. The two
application images are then merged and signed using the public/private key pair
stored in the secure bootloader, so that they will be accepted during the
image validation process at startup.

All TF-M dependencies will be automatically cloned in the ``/ext`` folder, and
built if appropriate build artefacts are not detected.

.. _PSA Certified Level 1:
  https://www.psacertified.org/security-certification/psa-certified-level-1/

.. note:: This example is based upon trusted-firmware-m commit 0d822496e809132e040c0a5cd556b2d5dad01ea6

Building and Running
********************

This project outputs startup status and info to the console. It can be built and
executed on an MPS2+ configured for AN521 (dual-core ARM Cortex M33).

This sample will only build on a Linux or macOS development system
(not Windows), and has been tested on the following setups:

- macOS Mojave using QEMU 4.1.0 with gcc-arm-none-eabi-7-2018-q2-update
- macOS Mojave with gcc-arm-none-eabi-7-2018-q2-update
- Ubuntu 18.04 using Zephyr SDK 0.10.1

On MPS2+ AN521:
===============

1. Build Zephyr with a non-secure configuration
   (``-DBOARD=mps2_an521_nonsecure``). This will also build TF-M in the
   ``ext/tfm`` folder if necessary.

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

3. Copy application binary files (mcuboot.bin and tfm_sign.bin) to
   ``<MPS2 device name>/SOFTWARE/``.

4. Edit (e.g., with vim) the ``<MPS2 device name>/MB/HBI0263C/AN521/images.txt``
   file, and update it as shown below:

   .. code-block:: bash

      TITLE: Versatile Express Images Configuration File

      [IMAGES]
      TOTALIMAGES: 2 ;Number of Images (Max: 32)

      IMAGE0ADDRESS: 0x10000000
      IMAGE0FILE: \SOFTWARE\mcuboot.bin  ; BL2 bootloader

      IMAGE1ADDRESS: 0x10080000
      IMAGE1FILE: \SOFTWARE\tfm_sign.bin ; TF-M with application binary blob

5. Save the file, exit the editor, and reset the MPS2+ board.

On QEMU:
========

1. Build Zephyr with a non-secure configuration
   (``-DBOARD=mps2_an521_nonsecure``). This will also build TF-M in the
   ``ext/tfm`` folder if necessary.

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

3. Run the qemu startup script, which will merge the key binaries and start
   execution of QEMU using the AN521 build target:

.. code-block:: bash

   ./qemu.sh

Sample Output
=============

.. code-block:: console

   [INF] Starting bootloader
   [INF] Swap type: none
   [INF] Bootloader chainload address offset: 0x80000
   [INF] Jumping to the first image slot
   [Sec Thread] Secure image initializing!
   TFM level is: 1
   [Sec Thread] Jumping to non-secure code...
   ***** Booting Zephyr OS build zephyr-v1.14.0-2726-g611526e98102 *****
   [00:00:00.000,000] <inf> app: app_cfg: Creating new config file with UID 0x155cfda7a
   [00:00:00.010,000] <inf> app: att: System IAT size is: 495 bytes.
   [00:00:00.010,000] <inf> app: att: Requesting IAT with 64 byte challenge.
   [00:00:00.100,000] <inf> app: att: IAT data received: 495 bytes.
             0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
   00000000 D2 84 43 A1 01 26 A1 04 58 20 07 8C 18 F1 10 F4 ..C..&..X ......
   00000010 32 FF 78 0C D8 DA E5 80 69 A2 A0 D8 22 77 CB C6 2.x.....i..."w..
   00000020 64 50 C8 58 1D D4 7D 96 A2 2E 59 01 80 AA 3A 00 dP.X..}...Y...:.
   00000030 01 24 FF 58 40 00 11 22 33 44 55 66 77 88 99 AA .$.X@.."3DUfw...
   00000040 BB CC DD EE FF 00 11 22 33 44 55 66 77 88 99 AA ......."3DUfw...
   00000050 BB CC DD EE FF 00 11 22 33 44 55 66 77 88 99 AA ......."3DUfw...
   00000060 BB CC DD EE FF 00 11 22 33 44 55 66 77 88 99 AA ......."3DUfw...
   00000070 BB CC DD EE FF 3A 00 01 24 FB 58 20 A0 A1 A2 A3 .....:..$.X ....
   00000080 A4 A5 A6 A7 A8 A9 AA AB AC AD AE AF B0 B1 B2 B3 ................
   00000090 B4 B5 B6 B7 B8 B9 BA BB BC BD BE BF 3A 00 01 25 ............:..%
   000000A0 01 77 77 77 77 2E 74 72 75 73 74 65 64 66 69 72 .wwww.trustedfir
   000000B0 6D 77 61 72 65 2E 6F 72 67 3A 00 01 24 F7 71 50 mware.org:..$.qP
   000000C0 53 41 5F 49 4F 54 5F 50 52 4F 46 49 4C 45 5F 31 SA_IOT_PROFILE_1
   000000D0 3A 00 01 25 00 58 21 01 FA 58 75 5F 65 86 27 CE :..%.X!..Xu_e.'.
   000000E0 54 60 F2 9B 75 29 67 13 24 8C AE 7A D9 E2 98 4B T`..u)g.$..z...K
   000000F0 90 28 0E FC BC B5 02 48 3A 00 01 24 FC 72 30 36 .(.....H:..$.r06
   00000100 30 34 35 36 35 32 37 32 38 32 39 31 30 30 31 30 0456527282910010
   00000110 3A 00 01 24 FA 58 20 AA AA AA AA AA AA AA AA BB :..$.X .........
   00000120 BB BB BB BB BB BB BB CC CC CC CC CC CC CC CC DD ................
   00000130 DD DD DD DD DD DD DD 3A 00 01 24 F8 20 3A 00 01 .......:..$. :..
   00000140 24 F9 19 30 00 3A 00 01 24 FD 81 A6 01 68 4E 53 $..0.:..$....hNS
   00000150 50 45 5F 53 50 45 04 65 30 2E 30 2E 30 03 00 02 PE_SPE.e0.0.0...
   00000160 58 20 52 ED 0E 2C F2 D2 D2 36 E0 CF 76 FD C2 64 X R..,...6..v..d
   00000170 1F E0 28 2E AA EF 14 A7 FB AE 92 52 C0 D1 5F 61 ..(........R.._a
   00000180 81 8A 06 66 53 48 41 32 35 36 05 58 20 BF E6 D8 ...fSHA256.X ...
   00000190 6F 88 26 F4 FF 97 FB 96 C4 E6 FB C4 99 3E 46 19 o.&..........>F.
   000001A0 FC 56 5D A2 6A DF 34 C3 29 48 9A DC 38 58 40 D9 .V].j.4.)H..8X@.
   000001B0 49 32 21 DB 84 16 89 A7 43 33 E4 9C DF EF 55 07 I2!.....C3....U.
   000001C0 C2 81 85 C7 AE 54 77 D9 A1 66 6A B0 76 77 7A 0E .....Tw..fj.vwz.
   000001D0 15 08 49 13 B5 2D CC C8 53 EC D0 01 40 C2 63 84 ..I..-..S...@.c.
   000001E0 A4 70 68 71 0A 71 BB BC 37 43 CD E5 0B DB A4    .phq.q..7C.....

Signing Images
==============

TF-M uses a secure bootloader (BL2) and firmware images must be signed
with a private key before execution can be handed off by the bootloader. The
firmware image is validated by the bootloader at startup using the public key,
which is built into the secure bootloader.

By default, ``tfm/bl2/ext/mcuboot/root-rsa-3072.pem`` is used to sign images.
``merge.sh`` signs the TF-M + Zephyr binary using the .pem private key,
calling `imgtool.py` to perform the actual signing operation.

To satisfy PSA Level 1 certification requirements, **You MUST replace
the default .pem file with a new key pair!**

To generate a new public/private key pair, run the following commands from
the sample folder:

.. code-block:: bash

  $ chmod +x ../../../ext/tfm/tfm/bl2/ext/mcuboot/scripts/imgtool.py
  $ ../../../ext/tfm/tfm/bl2/ext/mcuboot/scripts/imgtool.py keygen \
    -k root-rsa-3072.pem -t rsa-3072

You can then replace the .pem file in ``/ext/tfm/tfm/bl2/ext/mcuboot/`` with
the newly generated .pem file, and rebuild the bootloader so that it uses the
public key extracted from this new key file when validating firmware images.

.. code-block:: bash

  $ west build -p -b mps2_an521_nonsecure ./
  $ ./merge.sh

.. warning::

  Be sure to keep your private key file in a safe, reliable location! If you
  lose this key file, you will be unable to sign any future firmware images,
  and it will no longer be possible to update your devices in the field!
