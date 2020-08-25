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

Building and Running
********************

This project outputs startup status and info to the console. It can be built and
executed on an ARM Cortex M33 target board or QEMU.

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

On LPCxpresso55S69:
======================

Build Zephyr with a non-secure configuration:

   .. code-block:: bash

      $ west build -p -b lpcxpresso55s69_ns samples/tfm_integration/psa_level_1/ --

Next we need to manually flash the secure (``tfm_s.hex``)
and non-secure (``zephyr.hex``) images wth a J-Link as follows:

   .. code-block:: console

      JLinkExe -device lpc55s69 -if swd -speed 2000 -autoconnect 1
      J-Link>loadfile build/tfm/install/outputs/LPC55S69/tfm_s.hex
      J-Link>loadfile build/zephyr/zephyr.hex

NOTE: At present, the LPC55S69 doesn't include support for the MCUBoot bootloader.

We need to reset the board manually after flashing the image to run this code.

Sample Output
=============

   .. code-block:: console

      [INF] Starting bootloader
      [INF] Swap type: none
      [INF] Swap type: none
      [INF] Bootloader chainload address offset: 0x80000

      [INF] Jumping to the first image slot
      [Sec Thread] Secure image initializing!
      TF-M isolation level is: 1
      Booting TFM v1.0
      *** Booting Zephyr OS build v1.12.0-rc1-19787-g7bf29820769f  ***
      [00:00:00.003,000] <inf> app: app_cfg: Creating new config file with UID 0x155cfda7a
      [00:00:03.517,000] <inf> app: att: System IAT size is: 545 bytes.
      [00:00:03.517,000] <inf> app: att: Requesting IAT with 64 byte challenge.
      [00:00:06.925,000] <inf> app: att: IAT data received: 545 bytes.
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
      [00:00:06.982,000] <inf> app: Generating 256 bytes of random data.
                0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
      00000000 0C 90 D8 0C FA 0F 97 00 29 B2 AE 5C 90 48 3D 39 ........)..\.H=9
      00000010 00 14 6C A3 84 E2 C0 C9 82 F5 8B A6 E9 38 66 16 ..l..........8f.
      00000020 EA B7 E7 78 91 0D 6D 87 5B B8 04 0B 8B E0 74 23 ...x..m.[.....t#
      00000030 7D 11 E2 17 32 34 1A 01 71 24 29 D5 7C 05 B1 11 }...24..q$).|...
      00000040 A0 97 20 82 03 FF D6 76 9D 6F D5 52 45 C9 E1 17 .. ....v.o.RE...
      00000050 69 DF 18 B6 8E 0C AA 3B 74 B4 EF 97 D9 0E 82 25 i......;t......%
      00000060 E1 97 0E 6E 4F 0F DE B9 20 60 34 A4 EA 0D 9A B3 ...nO... `4.....
      00000070 3F C4 9A CF F3 5E F2 2C 78 96 6F 0E DD E3 E6 CB ?....^.,x.o.....
      00000080 DC 19 26 A3 E8 8E 07 0E 1E 5B DB 59 B0 05 41 E2 ..&......[.Y..A.
      00000090 A4 ED 90 35 8B AB 1C B8 00 7E BB 2D 22 FE 7A EA ...5.....~.-".z.
      000000A0 CF A0 BB DF 4F 2B 32 55 C9 07 0D 3D CE B8 43 78 ....O+2U...=..Cx
      000000B0 63 33 6C 79 CA 43 3A 4F 0B 93 33 2B B1 D2 B0 A7 c3ly.C:O..3+....
      000000C0 44 A0 E9 E8 BF FB FD 89 2A 44 7A 60 2D 9B 0F 9E D.......*Dz`-...
      000000D0 0D B1 0E 9D 5C 60 5D E6 92 78 36 79 68 37 24 C5 ....\`]..x6yh7$.
      000000E0 57 7F 2E DF 53 D2 7B 3F EE 56 9B 9E BB 39 2C B6 W...S.{?.V...9,.
      000000F0 AA FF B5 3B 59 4E 40 1D E0 34 50 05 D0 E0 95 12 ...;YN@..4P.....
      [00:00:07.004,000] <inf> app: Calculating SHA-256 hash of value.
                0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
      00000000 E3 B0 C4 42 98 FC 1C 14 9A FB F4 C8 99 6F B9 24
      00000010 27 AE 41 E4 64 9B 93 4C A4 95 99 1B 78 52 B8 55
