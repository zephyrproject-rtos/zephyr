.. _tfm_integration-samples:

TFM Integration Samples
#######################

.. toctree::
   :maxdepth: 1
   :glob:

   */*

Trusted Firmware-M (TF-M)
#########################

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

Trusted Firmware M source code is available at
`git.trustedfirmware.org <https://git.trustedfirmware.org>`_, although a fork
of this source code is maintained by the Zephyr Project as a module for
convenience sake at
`<https://github.com/zephyrproject-rtos/trusted-firmware-m>`_.

For further information consult the official `TF-M documentation`_

.. _TF-M documentation:
   https://ci.trustedfirmware.org/job/tf-m-build-test-nightly/lastSuccessfulBuild/artifact/build-docs/tf-m_documents/install/doc/user_guide/html/index.html

TF-M Requirements
*****************

The following Python modules are required when building TF-M binaries:

* cryptography
* pyasn1
* pyyaml
* cbor>=1.0.0
* imgtool>=1.6.0
* jinja2
* click

You can install them via:

   .. code-block:: bash

      $ pip3 install --user cryptography pyasn1 pyyaml cbor>=1.0.0 imgtool>=1.6.0 jinja2 click

They are used by TF-M's signing utility to prepare firmware images for
validation by the bootloader.

Part of the process of generating binaries for QEMU and merging signed
secure and non-secure binaries on certain platforms also requires the use of
the ``srec_cat`` utility.

This can be installed on Linux via:

   .. code-block:: bash

      $ sudo apt-get install srecord

And on OS X via:

   .. code-block:: bash

      $ brew install srecord

For Windows-based systems, please make sure you have a copy of the utility
available on your system path. See, for example:
`SRecord for Windows <http://srecord.sourceforge.net/windows.html>`_

Images Created by the TF-M Build
================================

The TF-M build system creates executable files:

* tfm_s - the secure firmware
* tfm_ns - a nonsecure app which is discarded in favor of the Zephyr app
* bl2 - mcuboot, if enabled

For each of these, it creates .bin, .hex, .elf, and .axf files.

The TF-M build system also creates signed variants of tfm_s and tfm_ns, and a file which combines them:

* tfm_s_signed
* tfm_ns_signed
* tfm_s_ns_signed

For each of these, only .bin files are created.
The Zephyr build system usually signs both tfm_s and the Zephyr app itself, see below.

The 'tfm' target contains properties for all these paths.
For example, the following will resolve to ``<path>/tfm_s.hex``:

   .. code-block::

      $<TARGET_PROPERTY:tfm,TFM_S_HEX_FILE>

See the top level CMakeLists.txt file in the tfm module for an overview of all the properties.

Signing Images
==============

TF-M uses a secure bootloader (BL2) and firmware images must be signed with a
private key. The firmware image is validated by the bootloader at startup using
the corresponding public key, which is stored inside the secure bootloader
firmware image.

By default, ``tfm/bl2/ext/mcuboot/root-rsa-3072.pem`` is used to sign secure
images, and ``tfm/bl2/ext/mcuboot/root-rsa-3072_1.pem`` is used to sign
non-secure images. Theses default .pem keys keys can be overridden using the
``CONFIG_TFM_KEY_FILE_S`` and ``CONFIG_TFM_KEY_FILE_NS`` values.

The ``wrapper.py`` script from TF-M signs the TF-M + Zephyr binary using the
.pem private key..

To satisfy `PSA Certified Level 1`_ requirements, **You MUST replace
the default .pem file with a new key pair!**

To generate a new public/private key pair, run the following commands:

   .. code-block:: bash

     $ cd $ZEPHYR_BASE/../modules/tee/tfm/trusted-firmware-m/bl2/ext/mcuboot/scripts
     $ chmod +x imgtool.py
     $ ./imgtool.py keygen -k root-rsa-3072.pem -t rsa-3072
     $ ./imgtool.py keygen -k root-rsa-3072_1.pem -t rsa-3072

You can then replace the .pem file in ``[TF-M_PATH]/bl2/ext/mcuboot/`` with
the newly generated .pem files, and rebuild the bootloader so that it uses the
public key extracted from this new key file when validating firmware images.

Alternatively, place the new .pem files in an alternate location, such as your
Zephyr application folder, and reference them in the ``prj.conf`` file via the
``CONFIG_TFM_KEY_FILE_S`` and ``CONFIG_TFM_KEY_FILE_NS`` config values.

   .. warning::

     Be sure to keep your private key file in a safe, reliable location! If you
     lose this key file, you will be unable to sign any future firmware images,
     and it will no longer be possible to update your devices in the field!

.. _PSA Certified Level 1:
  https://www.psacertified.org/security-certification/psa-certified-level-1/
