.. _tfm_psa_crypto:

TF-M PSA crypto
################

Overview
********
This TF-M integration example demonstrates how to use the PSA crypto API in
Zephyr for cryptography and device certificate signing request. In addition,
this example also demonstrates certain TF-M features that are covered as part
of the RTOS vendor requirements for a `PSA Certified Level 1`_ product, such
as secure storage for config data, initial attestation for device
verification.

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

Demonstrates hash, sign/verify workflow:

- Generate/import a persistent key: secp256r1 (usage: ecdsa-with-SHA256)
- Display the public key based on the private key data above
- Calculate the SHA256 hash of a payload
- Sign the hash with the persistent key
- Verify the signature using the public key
- Destroy the key

Also demonstrates device certificate signing request (CSR) workflow:

- Generate/import a persistent key: secp256r1 (usage: ecdsa-with-SHA256)
- Set subject name in device CSR
- Generate device CSR in PEM format
- Encode device CSR as JSON

Importing/generating the persistent key is based on config option
``PSA_IMPORT_KEY``. When ``PSA_IMPORT_KEY`` is enabled,
the key data can be static if ``PRIVATE_KEY_STATIC`` is set or key data
is generated using ``psa_generate_random`` if ``PRIVATE_KEY_RANDOM``
is set.

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

.. code-block:: cfg

   CONFIG_TFM_BL2=y
   CONFIG_TFM_CMAKE_BUILD_TYPE_DEBUG=y

On MPS2+ AN521:
===============

1. Build Zephyr with a non-secure configuration
   (``-DBOARD=mps2/an521/cpu0/ns``).

   Using ``west``

   .. code-block:: bash

      cd <ZEPHYR_ROOT>
      west build -p -b mps2/an521/cpu0/ns samples/tfm_integration/psa_crypto

   Using ``cmake`` and ``ninja``

   .. code-block:: bash

      cd <ZEPHYR_ROOT>/samples/tfm_integration/psa_crypto/
      rm -rf build
      mkdir build && cd build
      cmake -GNinja -DBOARD=mps2/an521/cpu0/ns ..
      ninja

   Using ``cmake`` and ``make``

   .. code-block:: bash

      cd <ZEPHYR_ROOT>/samples/tfm_integration/psa_crypto/
      rm -rf build
      mkdir build && cd build
      cmake -DBOARD=mps2/an521/cpu0/ns ..
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

Build Zephyr with a non-secure configuration (``-DBOARD=mps2/an521/cpu0/ns``)
and run it in qemu via the ``run`` command.

   Using ``west``

   .. code-block:: bash

      cd <ZEPHYR_ROOT>
      west build -p -b mps2/an521/cpu0/ns samples/tfm_integration/psa_crypto -t run

   Using ``cmake`` and ``ninja``

   .. code-block:: bash

      cd <ZEPHYR_ROOT>/samples/tfm_integration/psa_crypto/
      rm -rf build
      mkdir build && cd build
      cmake -GNinja -DBOARD=mps2/an521/cpu0/ns ..
      ninja run

   Using ``cmake`` and ``make``

   .. code-block:: bash

      cd <ZEPHYR_ROOT>/samples/tfm_integration/psa_crypto/
      rm -rf build
      mkdir build && cd build
      cmake -DBOARD=mps2/an521/cpu0/ns ..
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
(``-DBOARD=nrf5340dk/nrf5340/cpuapp/ns`` or ``-DBOARD=nrf9160dk/nrf9160/ns``).

   Example, for nRF9160, using ``cmake`` and ``ninja``

   .. code-block:: bash

      cd <ZEPHYR_ROOT>/samples/tfm_integration/psa_crypto/
      rm -rf build
      mkdir build && cd build
      cmake -GNinja -DBOARD=nrf9160dk/nrf9160/ns ..

If building with BL2 (MCUboot bootloader) enabled, manually flash
the MCUboot bootloader image binary (``bl2.hex``).

   Example, using ``nrfjprog`` on nRF9160:

   .. code-block:: bash

      nrfjprog -f NRF91 --program tfm/bin/bl2.hex --sectorerase

Finally, flash the concatenated TF-M + Zephyr binary.

   Example, for nRF9160, using ``cmake`` and ``ninja``

   .. code-block:: bash

      ninja flash

On BL5340:
==========

Build Zephyr with a non-secure configuration
(``-DBOARD=bl5340_dvk/nrf5340/cpuapp/ns``).

   Example using ``cmake`` and ``ninja``

   .. code-block:: bash

      cd <ZEPHYR_ROOT>/samples/tfm_integration/psa_crypto/
      rm -rf build
      mkdir build && cd build
      cmake -GNinja -DBOARD=bl5340_dvk/nrf5340/cpuapp/ns ..

Flash the concatenated TF-M + Zephyr binary.

   Example using ``west``

   .. code-block:: bash

      west flash --hex-file tfm_merged.hex

Sample Output
=============

   .. code-block:: console

      [Sec Thread] Secure image initializing!
      Booting TFM v1.4.1
      [Crypto] Dummy Entropy NV Seed is not suitable for production!
      *** Booting Zephyr OS build v2.7.99-1102-gf503ba9f1ab3  ***
      [00:00:00.014,000] <inf> app: app_cfg: Creating new config file with UID 0x1055CFDA7A
      [00:00:01.215,000] <inf> app: att: System IAT size is: 545 bytes.
      [00:00:01.215,000] <inf> app: att: Requesting IAT with 64 byte challenge.
      [00:00:01.836,000] <inf> app: att: IAT data received: 545 bytes.

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
      00000110 41 32 35 36 02 58 20 6D E1 0F 82 E0 CF FC 84 5A A256.X m.......Z
      00000120 24 25 2B EB 70 D7 2C 6B FC 92 CD BE 5B 65 9E C7 $%+.p.,k....[e..
      00000130 34 1E 1C D2 80 5D A3 A5 01 64 4E 53 50 45 04 65 4....]...dNSPE.e
      00000140 30 2E 30 2E 30 05 58 20 B3 60 CA F5 C9 8C 6B 94 0.0.0.X .`....k.
      00000150 2A 48 82 FA 9D 48 23 EF B1 66 A9 EF 6A 6E 4A A3 *H...H#..f..jnJ.
      00000160 7C 19 19 ED 1F CC C0 49 06 66 53 48 41 32 35 36 |......I.fSHA256
      00000170 02 58 20 01 4C F2 64 0D 49 F8 23 69 57 FE F3 73 .X .L.d.I.#iW..s
      00000180 97 7E 73 C2 2C 4F D2 95 25 D8 BE 29 32 14 23 5D .~s.,O..%..)2.#]
      00000190 A9 22 AD 3A 00 01 25 01 77 77 77 77 2E 74 72 75 .".:..%.wwww.tru
      000001A0 73 74 65 64 66 69 72 6D 77 61 72 65 2E 6F 72 67 stedfirmware.org
      000001B0 3A 00 01 24 F7 71 50 53 41 5F 49 4F 54 5F 50 52 :..$.qPSA_IOT_PR
      000001C0 4F 46 49 4C 45 5F 31 3A 00 01 24 FC 72 30 36 30 OFILE_1:..$.r060
      000001D0 34 35 36 35 32 37 32 38 32 39 31 30 30 31 30 58 456527282910010X
      000001E0 40 59 23 3E 80 5E E0 9F FA E3 F4 14 62 D3 15 A5 @Y#>.^......b...
      000001F0 B0 95 B5 E5 CB 79 92 F8 F1 A0 FE 14 0C 6C 84 2A .....y.......l.*
      00000200 41 97 BC 6F C6 7D 9C A5 21 BB 4C 2C D1 2C F3 66 A..o.}..!.L,.,.f
      00000210 4E D4 85 D2 57 15 72 11 E8 9E 06 4F C4 46 D0 58 N...W.r....O.F.X
      00000220 26                                              &

      [00:00:01.905,000] <inf> app: Persisting SECP256R1 key as #1
      [00:00:02.458,000] <inf> app: Retrieving public key for key #1

               0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
      00000000 04 07 93 39 CD 42 53 7B 18 8C 8A F1 05 7F 49 D1 ...9.BS{......I.
      00000010 6B 30 D5 39 0D 1A 6E 95 BA 0C CD FE DB 59 A3 03 k0.9..n......Y..
      00000020 02 61 B4 CF 13 CC 70 15 67 30 83 FE A0 D4 2A 19 .a....p.g0....*.
      00000030 72 82 3E 3F 90 00 91 C6 5E 43 DC E9 B4 C4 0E F3 r.>?....^C......
      00000040 79                                              y

      [00:00:03.020,000] <inf> app: Calculating SHA-256 hash of value

               0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
      00000000 50 6C 65 61 73 65 20 68 61 73 68 20 61 6E 64 20 Please hash and
      00000010 73 69 67 6E 20 74 68 69 73 20 6D 65 73 73 61 67 sign this messag
      00000020 65 2E                                           e.


               0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
      00000000 9D 08 E3 E6 DB 1C 12 39 C0 9B 9A 83 84 83 72 7A .......9......rz
      00000010 EA 96 9E 1D 13 72 1E 4D 35 75 CC D4 C8 01 41 9C .....r.M5u....A.

      [00:00:03.032,000] <inf> app: Signing SHA-256 hash

               0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
      00000000 EE F1 FE A6 A8 41 5F CC A6 3A 73 A7 C1 33 B4 78 .....A_..:s..3.x
      00000010 BF B7 38 78 2A 91 C8 82 32 F8 73 85 56 08 D2 A0 ..8x*...2.s.V...
      00000020 A6 22 2C 64 7A C7 E4 0A FB 99 D1 8B 67 37 F7 13 .",dz.......g7..
      00000030 E6 6C 54 7B 29 1D 3B A2 D8 E3 C4 79 17 BA 34 A8 .lT{).;....y..4.

      [00:00:03.658,000] <inf> app: Verifying signature for SHA-256 hash
      [00:00:06.339,000] <inf> app: Signature verified.
      [00:00:06.349,000] <inf> app: Destroyed persistent key #1
      [00:00:06.354,000] <inf> app: Generating 256 bytes of random data.

               0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
      00000000 24 5C B3 EB 88 D2 80 76 23 B3 07 CA 16 92 8F 3D $\.....v#......=
      00000010 27 AC C2 42 59 15 5E 3C EB 11 20 3C 14 A6 EB 60 '..BY.^<.. <...`
      00000020 C0 92 12 97 4D D7 62 BC A0 0A 34 A7 CE A8 78 18 ....M.b...4...x.
      00000030 1B 30 6E 3C DA 80 F2 55 F7 FA 10 8B F5 78 CE 92 .0n<...U.....x..
      00000040 92 FF F2 A3 22 4D 2D F6 62 39 6D A5 DD E1 E1 C4 ...."M-.b9m.....
      00000050 67 67 30 19 98 D7 E4 AD A2 6A 27 1C A4 C2 A2 C6 gg0......j'.....
      00000060 8A B5 98 26 D3 1A 84 75 55 52 4F E1 6D 4B 84 99 ...&...uURO.mK..
      00000070 0F C2 5E 88 D5 8B E6 AA 2F 61 DC 63 79 5B 69 3F ..^...../a.cy[i?
      00000080 19 79 5A 78 49 29 22 92 9D F5 F3 FD 16 60 E2 72 .yZxI)"......`.r
      00000090 EA F8 8E 32 7D 81 A0 21 0C 82 4A A8 4C EE 9C 0E ...2}..!..J.L...
      000000A0 D7 BF 50 60 6C 65 8A 7C A6 CD C5 98 8B 15 EA F0 ..P`le.|........
      000000B0 26 D0 15 F4 EB DE A0 FD 88 2F 72 8B ED 07 44 5C &......../r...D\
      000000C0 91 46 17 8C 26 46 F2 7C BF 6B 45 63 B6 71 E7 51 .F..&F.|.kEc.q.Q
      000000D0 E4 34 A2 5A 01 F4 6E FF A2 67 82 7B F3 36 34 54 .4.Z..n..g.{.64T
      000000E0 80 ED 7E 9D 0A 21 09 9C 9C 55 A9 14 AF A2 66 65 ..~..!...U....fe
      000000F0 DE 8D BE C2 8B 31 B8 ED 06 AE A9 0B 7E 62 75 87 .....1......~bu.

      [00:00:06.385,000] <inf> app: Initialising PSA crypto
      [00:00:06.386,000] <inf> app: PSA crypto init completed
      [00:00:06.387,000] <inf> app: Persisting SECP256R1 key as #1
      [00:00:06.938,000] <inf> app: Retrieving public key for key #1

               0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
      00000000 04 34 B7 2F D5 EC 41 71 B1 04 D9 BE 1C E7 DD F7 .4./..Aq........
      00000010 C4 C0 B1 E9 64 CB 45 1F E3 4A 95 52 A8 75 B2 8C ....d.E..J.R.u..
      00000020 4D F1 CB 4F C2 26 2C 90 C9 05 B2 E4 4C 2A E9 9D M..O.&,.....L*..
      00000030 11 DF 35 1B 0E 86 D5 9C A1 1F FC FA ED 21 9A B5 ..5..........!..
      00000040 28                                              (

      [00:00:07.495,000] <inf> app: Adding subject name to CSR
      [00:00:07.496,000] <inf> app: Adding subject name to CSR completed
      [00:00:07.497,000] <inf> app: Adding EC key to PK container
      [00:00:07.499,000] <inf> app: Adding EC key to PK container completed
      [00:00:07.500,000] <inf> app: Create device Certificate Signing Request
      [00:00:08.692,000] <inf> app: Create device Certificate Signing Request completed
      [00:00:08.693,000] <inf> app: Certificate Signing Request:

      -----BEGIN CERTIFICATE REQUEST-----
      MIHrMIGQAgEAMC4xDzANBgNVBAoMBkxpbmFybzEbMBkGA1UEAwwSRGV2aWNlIENl
      cnRpZmljYXRlMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAENLcv1exBcbEE2b4c
      5933xMCx6WTLRR/jSpVSqHWyjE3xy0/CJiyQyQWy5Ewq6Z0R3zUbDobVnKEf/Prt
      IZq1KKAAMAwGCCqGSM49BAMCBQADSAAwRQIgaAlTPmrIaRO7myM2Qr+LNk9sagdO
      jPGUqbz4oUWhUsICIQCuHADW6F2l4czv78BO5Nf+FHZEpjbI1+fA2aLzglOaiA==
      -----END CERTIFICATE REQUEST-----

      [00:00:08.696,000] <inf> app: Encoding CSR as json
      [00:00:08.699,000] <inf> app: Encoding CSR as json completed
      [00:00:08.700,000] <inf> app: Certificate Signing Request in JSON:

      {"CSR":"-----BEGIN CERTIFICATE REQUEST-----\nMIHrMIGQAgEAMC4xDzANBgNVBAoMBkxpbmFybzEbMBkGA1UEAwwSRGV2aWNlIENl\ncnRpZmljYXRlMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAENLcv1exBcbEE2b4c\n5933xMCx6WTLRR/jSpVSqHWyjE3xy0/CJiyQyQWy5Ewq6Z0R3zUbDobVnKEf/Prt\nIZq1KKAAMAwGCCqGSM49BAMCBQADSAAwRQIgaAlTPmrIaRO7myM2Qr+LNk9sagdO\njPGUqbz4oUWhUsICIQCuHADW6F2l4czv78BO5Nf+FHZEpjbI1+fA2aLzglOaiA==\n-----END CERTIFICATE REQUEST-----\n"}
