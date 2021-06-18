.. psa_custom_service:

PSA Custom Service
####################

Overview
********

PSA specification defines following security services:

- Cryptography API
- Secure Storage API
- Attestation API
- Firmware Update API

Security service developers can create custom service that are not part of PSA.
This sample demonstrates the use of PSA custom service API.

An implementation of example secure service is available at https://github.com/zephyrproject-rtos/trusted-firmware-m/tree/master/trusted-firmware-m/secure_fw/partitions/example_partition

For more information refer to https://ci-builds.trustedfirmware.org/static-files/gFvFxDUNlMBpcBlMUJapUCfmj1_IRkRiCOu_eU685ogxNjI0MDE4Njk4NjE4Ojk6YW5vbnltb3VzOmpvYi90Zi1tLWJ1aWxkLWRvY3MtbmlnaHRseS9sYXN0U3RhYmxlQnVpbGQvYXJ0aWZhY3Q=/trusted-firmware-m/build/docs/user_guide/html/docs/integration_guide/services/tfm_secure_partition_addition.html

Building and Running
********************

On Target
=========

Refer to :ref:`tfm_psa_crypto` for detailed instructions.

On QEMU
========

Refer to :ref:`tfm_ipc` for detailed instructions.
Following is an example based on ``west build``

   .. code-block:: bash

      $ west build samples/tfm_integration/psa_custom_service/ -p -b mps2_an521_nonsecure -t run

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
      *** Booting Zephyr OS build zephyr-v2.6.0-343-gc7a4679e1540  ***
      [Example partition] Service called! arg=0xff
      [Example partition] Timer started...
      [00:00:00.003,000] <inf> app: Hashing the message

               0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
      00000000 50 6C 65 61 73 65 20 68 61 73 68 20 74 68 69 73 Please hash this
      00000010 20 6D 65 73 73 61 67 65 2E                       message.

      [Example partition] Calculating SHA-256!
      [Example partition] Done!
      [00:00:00.015,000] <inf> app: Size of SHA-256 hash is 32 bytes

               0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
      00000000 56 B3 E0 03 97 58 BD CE D2 E2 AA 5B 3A 04 8E 2D V....X.....[:..-
      00000010 FE 90 53 BE 87 38 FB 4A 31 F7 1D 56 6A 7E FF 24 ..S..8.J1..Vj~.$
