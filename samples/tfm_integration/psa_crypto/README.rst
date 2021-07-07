.. _tfm_psa_crypto:

TF-M PSA crypto
################

Overview
********
This TF-M integration example demonstrates how to use the PSA crypto API in
Zephyr for cryptography. In addition, this example also demonstrates certain
TF-M features that are covered as part of the RTOS vendor requirements for a
`PSA Certified Level 1`_ product, such as secure storage for config data,
initial attestation for device verification.

Trusted Firmware (TF-M) Platform Security Architecture (PSA) APIs
are used for the secure processing environment, with Zephyr running in the
non-secure processing environment.

It uses **IPC Mode** for communication, where an IPC mechanism is inserted to
handle secure TF-M API calls and responses.

The sample prints test info to the console either as a single-thread or
multi-thread application.

.. _PSA Certified Level 1:
  https://www.psacertified.org/security-certification/psa-certified-level-1/

Key Files
*********

``psa_crypto.c``
================

Demonstrates the following workflow:

- Generate a persistent key: secp256r1 (usage: ecdsa-with-SHA256)
- Display the public key based on the private key data above
- Calculate the SHA256 hash of a payload
- Sign the hash with the persistent key
- Verify the signature using the public key
- Destroy the key

``psa_attestation.c``
=====================

Demonstrates how to request an initial attestation token (IAT) from the TF-M
secure processing environment (SPE).

Building and Running
********************

This project outputs startup status and info to the console. It can be built and
executed on an ARM Cortex M33 target board or QEMU.

This sample will only build on a Linux or macOS development system
(not Windows), and has been tested on the following setups:

- macOS Mojave using QEMU 4.2.0 with gcc-arm-none-eabi-7-2018-q2-update
- macOS Mojave with gcc-arm-none-eabi-7-2018-q2-update
- Ubuntu 18.04 using Zephyr SDK 0.11.2

TF-M BL2 logs
=============
Add the following to ``prj.conf`` to see the logs from TF-M BL2:
   .. code-block:: bash

      CONFIG_TFM_BL2=y
      CONFIG_TFM_CMAKE_BUILD_TYPE_DEBUG=y

On MPS2+ AN521:
===============

1. Build Zephyr with a non-secure configuration
   (``-DBOARD=mps2_an521_ns``).

   Using ``west``

   .. code-block:: bash

      cd <ZEPHYR_ROOT>
      west build -p -b mps2_an521_ns samples/tfm_integration/psa_crypto

   Using ``cmake`` and ``ninja``

   .. code-block:: bash

      cd <ZEPHYR_ROOT>/samples/tfm_integration/psa_crypto/
      rm -rf build
      mkdir build && cd build
      cmake -GNinja -DBOARD=mps2_an521_ns ..
      ninja

   Using ``cmake`` and ``make``

   .. code-block:: bash

      cd <ZEPHYR_ROOT>/samples/tfm_integration/psa_crypto/
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
      west build -p -b mps2_an521_ns samples/tfm_integration/psa_crypto -t run

   Using ``cmake`` and ``ninja``

   .. code-block:: bash

      cd <ZEPHYR_ROOT>/samples/tfm_integration/psa_crypto/
      rm -rf build
      mkdir build && cd build
      cmake -GNinja -DBOARD=mps2_an521_ns ..
      ninja run

   Using ``cmake`` and ``make``

   .. code-block:: bash

      cd <ZEPHYR_ROOT>/samples/tfm_integration/psa_crypto/
      rm -rf build
      mkdir build && cd build
      cmake -DBOARD=mps2_an521_ns ..
      make run

On LPCxpresso55S69:
======================

Build Zephyr with a non-secure configuration:

   .. code-block:: bash

      $ west build -p -b lpcxpresso55s69_ns samples/tfm_integration/psa_crypto/ --

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
(``-DBOARD=nrf5340dk_nrf5340_cpuapp_ns`` or ``-DBOARD=nrf9160dk_nrf9160ns``).

   Example, for nRF9160, using ``cmake`` and ``ninja``

   .. code-block:: bash

      cd <ZEPHYR_ROOT>/samples/tfm_integration/psa_crypto/
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

On BL5340:
==========

Build Zephyr with a non-secure configuration
(``-DBOARD=bl5340_dvk_cpuapp_ns``).

   Example using ``cmake`` and ``ninja``

   .. code-block:: bash

      cd <ZEPHYR_ROOT>/samples/tfm_integration/psa_crypto/
      rm -rf build
      mkdir build && cd build
      cmake -GNinja -DBOARD=bl5340_dvk_cpuapp_ns ..

Flash the concatenated TF-M + Zephyr binary.

   Example using ``west``

   .. code-block:: bash

      west flash --hex-file tfm_merged.hex

Sample Output
=============

   .. code-block:: console

      [INF] Starting bootloader
      [INF] Swap type: none
      [INF] Swap type: none
      [INF] Bootloader chainload address offset: 0x80000
      [INF] Jumping to the first image slot
      [Sec Thread] Secure image initializing!
      TF-M isolation level is: 0x00000001
      Booting TFM v1.3.0
      Jumping to non-secure code...
      *** Booting Zephyr OS build v2.6.0-rc2-1-g77259223c716  ***
      [00:00:00.037,000] <inf> app: app_cfg: Creating new config file with UID 0x1055CFDA7A
      [00:00:03.968,000] <inf> app: att: System IAT size is: 545 bytes.
      [00:00:03.968,000] <inf> app: att: Requesting IAT with 64 byte challenge.
      [00:00:05.961,000] <inf> app: att: IAT data received: 545 bytes.

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
      00000110 41 32 35 36 02 58 20 AE AA BE 88 46 21 BA 4F ED A256.X ....F!.O.
      00000120 E9 68 26 05 08 42 FC D0 1E AE 31 EB A9 47 5B D7 .h&..B....1..G[.
      00000130 5E C0 7F 75 C8 0A 0A A5 01 64 4E 53 50 45 04 65 ^..u.....dNSPE.e
      00000140 30 2E 30 2E 30 05 58 20 B3 60 CA F5 C9 8C 6B 94 0.0.0.X .`....k.
      00000150 2A 48 82 FA 9D 48 23 EF B1 66 A9 EF 6A 6E 4A A3 *H...H#..f..jnJ.
      00000160 7C 19 19 ED 1F CC C0 49 06 66 53 48 41 32 35 36 |......I.fSHA256
      00000170 02 58 20 FC 36 15 76 EE 01 5C FC 2A 2E 23 C6 43 .X .6.v..\.*.#.C
      00000180 DD 3C C4 5A 68 A7 1A CC 14 7A BF 3F B1 9B E2 D7 .<.Zh....z.?....
      00000190 E3 74 88 3A 00 01 25 01 77 77 77 77 2E 74 72 75 .t.:..%.wwww.tru
      000001A0 73 74 65 64 66 69 72 6D 77 61 72 65 2E 6F 72 67 stedfirmware.org
      000001B0 3A 00 01 24 F7 71 50 53 41 5F 49 4F 54 5F 50 52 :..$.qPSA_IOT_PR
      000001C0 4F 46 49 4C 45 5F 31 3A 00 01 24 FC 72 30 36 30 OFILE_1:..$.r060
      000001D0 34 35 36 35 32 37 32 38 32 39 31 30 30 31 30 58 456527282910010X
      000001E0 40 53 A1 B7 9B 18 45 D4 15 4D 84 8C A6 D6 0C 10 @S....E..M......
      000001F0 A3 88 17 E7 E7 C9 39 72 DC 32 ED A0 DB FB EA 06 ......9r.2......
      00000200 19 AF AF 6C 88 55 22 84 4E 1B 2F DF 9E 57 C3 12 ...l.U".N./..W..
      00000210 7E 96 39 DB DC F8 A3 7F C1 BC 6D C2 9B 42 16 40 ~.9.......m..B.@
      00000220 49                                              I

      [00:00:06.025,000] <inf> app: Persisting SECP256R1 key as #1
      [00:00:06.035,000] <inf> app: Retrieving public key for key #1

               0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
      00000000 04 2E 36 AC C3 55 DC 17 A5 D8 0C 9B 70 F5 C6 C2 ..6..U......p...
      00000010 F0 10 67 8E C5 21 D7 D7 43 79 2C CF 41 32 C1 15 ..g..!..Cy,.A2..
      00000020 33 CC A8 F4 1E ED FB 45 CA 1C E7 C0 FD 07 B2 85 3......E........
      00000030 B3 AD CC C3 7C 08 81 9B 44 64 E4 EA 9A 2A 38 46 ....|...Dd...*8F
      00000040 D5                                              .

      [00:00:07.935,000] <inf> app: Calculating SHA-256 hash of value

               0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
      00000000 50 6C 65 61 73 65 20 68 61 73 68 20 61 6E 64 20 Please hash and
      00000010 73 69 67 6E 20 74 68 69 73 20 6D 65 73 73 61 67 sign this messag
      00000020 65 2E                                           e.


               0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
      00000000 9D 08 E3 E6 DB 1C 12 39 C0 9B 9A 83 84 83 72 7A .......9......rz
      00000010 EA 96 9E 1D 13 72 1E 4D 35 75 CC D4 C8 01 41 9C .....r.M5u....A.

      [00:00:07.945,000] <inf> app: Signing SHA-256 hash

               0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
      00000000 E8 59 8C C1 A1 D7 0C 00 34 60 D7 D7 1D 82 DA 26 .Y......4`.....&
      00000010 5D EC 2A 40 26 8F 20 A3 4B B8 B4 8D 44 25 1D F1 ].*@&. .K...D%..
      00000020 78 FF CA CB 96 0B B3 31 F0 68 AB BF F3 57 FF A8 x......1.h...W..
      00000030 DB E6 02 01 59 22 5D 53 13 81 63 31 3C 75 61 92 ....Y"]S..c1<ua.

      [00:00:09.919,000] <inf> app: Verifying signature for SHA-256 hash
      [00:00:14.559,000] <inf> app: Signature verified.
      [00:00:14.570,000] <inf> app: Destroyed persistent key #1
      [00:00:14.574,000] <inf> app: Generating 256 bytes of random data.

               0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
      00000000 30 13 B1 67 10 2E 2B 7A 45 A7 89 32 80 89 DB 05 0..g..+zE..2....
      00000010 30 93 CF F0 03 9A BA 92 0C A4 54 46 96 A4 C2 A9 0.........TF....
      00000020 11 A2 0B F6 3A C5 5A FB 55 51 4F CB C5 7D 02 71 ....:.Z.UQO..}.q
      00000030 19 AA A0 62 36 AA 69 5F 8E 93 A8 9B DB 8C AF 7C ...b6.i_.......|
      00000040 A0 68 C7 60 48 1C 30 51 20 2E AD B6 91 22 38 14 .h.`H.0Q ...."8.
      00000050 87 00 F6 59 18 81 DB 6B E0 67 95 0C FF 67 B2 1D ...Y...k.g...g..
      00000060 9E 15 B6 46 94 F0 08 15 5F C8 B7 61 72 34 28 18 ...F...._..ar4(.
      00000070 BA D1 41 2B D3 5B C7 72 87 89 70 E4 34 6D 40 B7 ..A+.[.r..p.4m@.
      00000080 B2 38 77 C9 A9 C3 81 18 3C 67 AD 30 CC B4 CE 77 .8w.....<g.0...w
      00000090 54 11 D6 8B FC 18 D1 7B 26 D3 45 00 67 23 E7 F2 T......{&.E.g#..
      000000A0 5C 59 CB 63 8F C5 8C 2F 01 CC 09 CE 06 85 4D DC \Y.c.../......M.
      000000B0 33 41 48 F8 01 8D DA 39 F9 DB 71 0D 80 E6 53 42 3AH....9..q...SB
      000000C0 58 B0 A8 50 6D 5E 11 B1 EC 53 5E FA 23 AC 7A 0D X..Pm^...S^.#.z.
      000000D0 EF AC 98 76 68 82 4C 48 8E B4 51 D4 31 78 AE 52 ...vh.LH..Q.1x.R
      000000E0 7F F2 19 0D 57 6B C7 5B 77 77 36 E7 87 E2 DA 74 ....Wk.[ww6....t
      000000F0 BF BB 83 5F 8F 94 83 21 28 3A A6 B9 5A 73 18 E2 ..._...!(:..Zs..
