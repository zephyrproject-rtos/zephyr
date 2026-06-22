.. zephyr:code-sample:: psa_protected_storage
   :name: TF-M PSA Protected Storage

   Use the Protected Storage (PS) API to store encrypted data.

Overview
********

This sample demonstrates how the Protected Storage (PS) API can be used for storing data.

Protected storage provides a key/value storage interface where data is (by default) encrypted, with
optional authentication and rollback protection. The default crypto algorithm is ``AES-128-GCM``.
The encryption key is derived from the Hardware Unique Key (HUK), which is often set via device
fuses, etc.

Using the PS API, this sample stores data to non-volatile storage. The sample shows how data can
be stored to and read from UIDs, and how overwrite protection can be enabled using flags.

TF-M includes a maximum number of PS records, set via ``PS_NUM_ASSETS`` (default 10 as of
TF-M 1.3.0), and a maximum record size, set via ``PS_MAX_ASSET_SIZE`` (default of 2048 as of
TF-M 1.3.0). These defaults may be different depending on the platform being used, the current
value will be printed by the build system during the TF-M compilation step.

More information about Protected Storage can be found in the Platform Security Architecture (PSA)
Secure Storage API: https://developer.arm.com/architectures/architecture-security-features/platform-security

This sample is available for platforms that are supported in the trusted-firmware-m repo:
https://git.trustedfirmware.org/TF-M/trusted-firmware-m.git/
See sample.yaml for a list of supported platforms.

Building and Running
********************

On Target
=========

Note that the board needs to be completely erased before programming the sample, as the flash area
used might contain data from before. The board must also be erased between each time the sample is
run as the overwrite protection will not be removed with a power reset.

On QEMU
========

Refer to :zephyr:code-sample:`tfm_ipc` for detailed instructions.
Following is an example based on ``west build``

   .. code-block:: bash

      $ west build samples/tfm_integration/psa_protected_storage/ -p -b mps2/an521/cpu0/ns -t run

Sample Output
=============

.. code-block:: console

    *** Booting Zephyr OS build zephyr-v2.5.0-2791-g5585355dde0c  ***
    TF-M Protected Storage sample started. PSA Protected Storage API Version 1.0
    Writing data to UID1: The quick brown fox jumps over the lazy dog
    Info on data stored in UID1:
    - Size: 16
    - Capacity: 0x42
    - Flags: 0x 0
    Read and compare data stored in UID1
    Data stored in UID1: The quick brown fox jumps over the lazy dog
    Overwriting data stored in UID1 with: Lorem ipsum dolor sit amet
    Writing data to UID2 with overwrite protection: The quick brown fox jumps over the lazy dog
    Attempting to write 'The quick brown fox jumps over the lazy dog' to UID2
    Got expected error (PSA_ERROR_NOT_PERMITTED) when writing to protected UID
    Removing UID1
