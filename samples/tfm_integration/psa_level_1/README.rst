.. _tfm_psa_level_1:

TF-M PSA Level 1
################

Overview
********
This TF-M integration example demonstrates how to use certain TF-M features
that are covered as part of the RTOS vendor requirements for a
`PSA Certified Level 1`_ product, such as secure storage for config data,
initial attestation for device verification, and the PSA crypto API for
cryptography.

Trusted Firmware (TF-M) Platform Security Architecture (PSA) APIs
are used for the secure processing environment, with Zephyr running in the
non-secure processing environment.

It uses **IPC Mode** for communication, where an IPC mechanism is inserted to
handle secure TF-M API calls and responses. The OS-specific code to handle
the IPC calls is in ``tfm_ipc.c``.

The sample prints test info to the console either as a single-thread or
multi-thread application.

.. _PSA Certified Level 1:
  https://www.psacertified.org/security-certification/psa-certified-level-1/

Key Files
*********

``psa_crypto.c``
================

Demonstrates the following workflow:

- Generate a permanent persistent key (prime256v1 or ecdsa-with-SHA256)
- Display the public key based on the private key data above
- Calculates the SHA256 hash of a payload
- Signs the hash with the persistent key
- Verifies the signature using the public key
- Destroys the key

``psa_attestation.c``
=====================

Demonstrates how to request an initial attestation token (IAT) from the TF-M
secure processing environment (SPE).

``tfm_ipc.c``
=============

Mutex handler required to enable safe use of the TF-M IPC mechanism in Zephyr.

Building and Running
********************

This project outputs startup status and info to the console. It can be built and
executed on an MPS2+ configured for AN521 (dual-core ARM Cortex M33), or using
the ``mps2_an521_nonsecure`` target with QEMU.

This sample will only build on a Linux or macOS development system
(not Windows), and has been tested on the following setups:

- macOS Mojave using QEMU 4.2.0 with gcc-arm-none-eabi-7-2018-q2-update
- macOS Mojave with gcc-arm-none-eabi-7-2018-q2-update
- Ubuntu 18.04 using Zephyr SDK 0.11.2

On MPS2+ AN521:
===============

1. Build Zephyr with a non-secure configuration
   (``-DBOARD=mps2_an521_nonsecure``).

   Using ``west``

   .. code-block:: bash

      cd <ZEPHYR_ROOT>
      west build -p -b mps2_an521_nonsecure samples/tfm_integration/psa_level_1

   Using ``cmake`` and ``ninja``

   .. code-block:: bash

      cd <ZEPHYR_ROOT>/samples/tfm_integration/psa_level_1/
      rm -rf build
      mkdir build && cd build
      cmake -GNinja -DBOARD=mps2_an521_nonsecure ..
      ninja

   Using ``cmake`` and ``make``

   .. code-block:: bash

      cd <ZEPHYR_ROOT>/samples/tfm_integration/psa_level_1/
      rm -rf build
      mkdir build && cd build
      cmake -DBOARD=mps2_an521_nonsecure ..
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

Build Zephyr with a non-secure configuration (``-DBOARD=mps2_an521_nonsecure``)
and run it in qemu via the ``run`` command.

   Using ``west``

   .. code-block:: bash

      cd <ZEPHYR_ROOT>
      west build -p -b mps2_an521_nonsecure samples/tfm_integration/psa_level_1 -t run

   Using ``cmake`` and ``ninja``

   .. code-block:: bash

      cd <ZEPHYR_ROOT>/samples/tfm_integration/psa_level_1/
      rm -rf build
      mkdir build && cd build
      cmake -GNinja -DBOARD=mps2_an521_nonsecure ..
      ninja run

   Using ``cmake`` and ``make``

   .. code-block:: bash

      cd <ZEPHYR_ROOT>/samples/tfm_integration/psa_level_1/
      rm -rf build
      mkdir build && cd build
      cmake -DBOARD=mps2_an521_nonsecure ..
      make run

Sample Output
=============

   .. code-block:: console

      [INF] Starting bootloader
      [INF] Image 0: version=0.0.0+1, magic= good, image_ok=0x3
      [INF] Image 1: No valid image
      [INF] Booting image from the primary slot
      [INF] Bootloader chainload address offset: 0x80000
      [INF] Jumping to the first image slot
      [Sec Thread] Secure image initializing!
      TF-M isolation level is: 1
      Booting TFM v1.0
      *** Booting Zephyr OS build v2.3.0-rc1  ***
      [00:00:00.003,000] <inf> app: app_cfg: Creating new config file with UID 0x155cfda7a
      [00:00:03.515,000] <inf> app: att: System IAT size is: 453 bytes.
      [00:00:03.515,000] <inf> app: att: Requesting IAT with 64 byte challenge.
      [00:00:06.920,000] <inf> app: att: IAT data received: 453 bytes.
                0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
      00000000 D2 84 43 A1 01 26 A0 59 01 D5 AA 3A 00 01 24 FF ..C..&.Y...:..$.
      00000010 58 40 00 11 22 33 44 55 66 77 88 99 AA BB CC DD X@.."3DUfw......
      00000020 EE FF 00 11 22 33 44 55 66 77 88 99 AA BB CC DD ...."3DUfw......
      00000030 EE FF 00 11 22 33 44 55 66 77 88 99 AA BB CC DD ...."3DUfw......
      00000040 EE FF 00 11 22 33 44 55 66 77 88 99 AA BB CC DD ...."3DUfw......
      00000050 EE FF 3A 00 01 24 FB 58 20 A0 A1 A2 A3 A4 A5 A6 ..:..$.X .......
      00000060 A7 A8 A9 AA AB AC AD AE AF B0 B1 B2 B3 B4 B5 B6 ................
      00000070 B7 B8 B9 BA BB BC BD BE BF 3A 00 01 25 00 58 21 .........:..%.X!
      00000080 01 FA 58 75 5F 65 86 27 CE 54 60 F2 9B 75 29 67 ..Xu_e.'.T`..u)g
      00000090 13 24 8C AE 7A D9 E2 98 4B 90 28 0E FC BC B5 02 .$..z...K.(.....
      000000A0 48 3A 00 01 24 FA 58 20 AA AA AA AA AA AA AA AA H:..$.X ........
      000000B0 BB BB BB BB BB BB BB BB CC CC CC CC CC CC CC CC ................
      000000C0 DD DD DD DD DD DD DD DD 3A 00 01 24 F8 20 3A 00 ........:..$. :.
      000000D0 01 24 F9 19 30 00 3A 00 01 24 FD 82 A5 01 63 53 .$..0.:..$....cS
      000000E0 50 45 04 65 30 2E 30 2E 30 05 58 20 BF E6 D8 6F PE.e0.0.0.X ...o
      000000F0 88 26 F4 FF 97 FB 96 C4 E6 FB C4 99 3E 46 19 FC .&..........>F..
      00000100 56 5D A2 6A DF 34 C3 29 48 9A DC 38 06 66 53 48 V].j.4.)H..8.fSH
      00000110 41 32 35 36 02 58 20 EF FC 32 08 03 06 CA 5A 8C A256.X ..2....Z.
      00000120 D2 93 C8 46 04 DD 45 3F CA 41 20 47 A8 F7 D4 09 ...F..E?.A G....
      00000130 24 16 94 38 05 68 B6 A5 01 64 4E 53 50 45 04 65 $..8.h...dNSPE.e
      00000140 30 2E 30 2E 30 05 58 20 B3 60 CA F5 C9 8C 6B 94 0.0.0.X .`....k.
      00000150 2A 48 82 FA 9D 48 23 EF B1 66 A9 EF 6A 6E 4A A3 *H...H#..f..jnJ.
      00000160 7C 19 19 ED 1F CC C0 49 06 66 53 48 41 32 35 36 |......I.fSHA256
      00000170 02 58 20 D5 3F 25 8F AA 5A 05 33 36 F4 D9 2C D6 .X .?%..Z.36..,.
      00000180 11 DF 6E 1B 18 B9 03 09 37 01 9D A7 5E FC 57 32 ..n.....7...^.W2
      00000190 B3 1A 94 3A 00 01 25 01 77 77 77 77 2E 74 72 75 ...:..%.wwww.tru
      000001A0 73 74 65 64 66 69 72 6D 77 61 72 65 2E 6F 72 67 stedfirmware.org
      000001B0 3A 00 01 24 F7 71 50 53 41 5F 49 4F 54 5F 50 52 :..$.qPSA_IOT_PR
      000001C0 4F 46 49 4C 45 5F 31 3A 00 01 24 FC 72 30 36 30 OFILE_1:..$.r060
      000001D0 34 35 36 35 32 37 32 38 32 39 31 30 30 31 30 58 456527282910010X
      000001E0 40 51 33 D9 87 96 A9 91 55 18 9E BF 14 7A E1 76 @Q3.....U....z.v
      000001F0 F5 0F A6 3C 7B F2 3A 1B 59 24 5B 2E 67 A8 F8 AB ...<{.:.Y$[.g...
      00000200 12 B4 2E 09 13 5B BF 35 1F ED 66 E3 36 CF DA CE .....[.5..f.6...
      00000210 06 03 69 DF C0 DC 4D 2F 17 33 D7 5E BE 73 B9 0E ..i...M/.3.^.s..
      00000220 08                                              .
      [00:00:06.962,000] <inf> app: Persisting SECP256R1 key as #1
      [00:00:09.400,000] <inf> app: Retrieving public key for key #1
                0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
      00000000 04 47 EA AE D9 D6 6D 2E 1D 65 05 F5 04 FE CC 21 .G....m..e.....!
      00000010 99 BE 5E 5A 56 6B 4F 1E 0C 43 E2 5B CE 1B 7D 06 ..^ZVkO..C.[..}.
      00000020 D7 B3 71 E2 0A 3C 47 ED 84 9F 65 0E DB F9 3D D2 ..q..<G...e...=.

      00000030 07 BB 81 A6 73 E6 3B 16 95 19 AC 01 02 CB 1C F5 ....s.;.........
      00000040 35                                              5
      [00:00:11.831,000] <inf> app: Calculating SHA-256 hash of value
                0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
      00000000 50 6C 65 61 73 65 20 68 61 73 68 20 61 6E 64 20 Please hash and
      00000010 73 69 67 6E 20 74 68 69 73 20 6D 65 73 73 61 67 sign this messag
      00000020 65 2E                                           e.
                0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
      00000000 9D 08 E3 E6 DB 1C 12 39 C0 9B 9A 83 84 83 72 7A .......9......rz
      00000010 EA 96 9E 1D 13 72 1E 4D 35 75 CC D4 C8 01 41 9C .....r.M5u....A.
      [00:00:11.851,000] <inf> app: Signing SHA-256 hash
                0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
      00000000 81 FC CE C2 02 96 79 E0 60 A8 0C 53 22 58 F3 17 ......y.`..S"X..
      00000010 7A AC 46 60 7E 30 7F 60 03 53 1C 43 CA 31 97 B8 z.F`~0.`.S.C.1..
      00000020 47 47 56 E9 19 45 F9 E2 DC 38 68 8D F1 A7 C7 48 GGV..E...8h....H
      00000030 96 26 F6 0C 0F 94 D8 E3 9E 66 82 76 A6 BC B4 FC .&.......f.v....
      [00:00:15.199,000] <inf> app: Verifying signature for SHA-256 hash
      [00:00:20.985,000] <inf> app: Signature verified.
      [00:00:23.439,000] <inf> app: Destroyed persistent key #1
