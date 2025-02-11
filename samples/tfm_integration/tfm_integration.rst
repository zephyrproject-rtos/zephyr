.. _tfm_integration-samples:

TF-M Integration Samples
########################

.. toctree::
   :maxdepth: 1
   :glob:

   */*

Overview
********
These TF-M integration examples can be used with a supported Armv8-M board, and
demonstrate how the TF-M APIs can be used with Zephyr.

Trusted Firmware Platform Security Architecture (PSA) APIs are used for the
secure processing environment (S), with Zephyr running in the non-secure
processing environment (NS).

As part of the standard build process, the secure bootloader (BL2) is normally
built, in addition to the TF-M S and Zephyr NS binary images. The S and NS
images are then merged and signed using the private signing keys, whose public
key values are stored in the secure bootloader. This allows the application
images to be verified and either accepted or rejected during the image
verification process at startup, based on a pair of private keys that you
control.

What is Trusted Firmware-M (TF-M)?
**********************************

Trusted Firmware-M (TF-M) is the reference implementation of `Platform Security
Architecture (PSA) <https://pages.arm.com/psa-resources.html>`_.

TF-M provides a highly configurable set of software components to create a
Trusted Execution Environment. This is achieved by a set of secure run time
services such as Secure Storage, Cryptography, Audit Logs and Attestation.
Additionally, secure boot in TF-M ensures integrity of run time software and
supports firmware upgrade.

The current TF-M implementation specifically targets TrustZone for ARMv8-M.

Trusted Firmware-M source code is available at
`git.trustedfirmware.org <https://git.trustedfirmware.org>`_, although a fork
of this source code is maintained by the Zephyr Project as a module for
convenience sake at
`<https://github.com/zephyrproject-rtos/trusted-firmware-m>`_.

For further information consult the official `TF-M documentation`_

.. _TF-M documentation:
   https://tf-m-user-guide.trustedfirmware.org/
