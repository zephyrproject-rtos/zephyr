Trusted Firmware-M Overview
###########################

`Trusted Firmware-M (TF-M) <https://tf-m-user-guide.trustedfirmware.org/>`__
is a reference implementation of the Platform Security Architecture (PSA)
`IoT Security Framework <https://www.psacertified.org/what-is-psa-certified/>`__.
It defines and implements an architecture and a set of software components
that aim to address some of the main security concerns in IoT products.

Zephyr RTOS has been PSA Certified since Zephyr 2.0.0 with TF-M 1.0, and
is currently integrated with TF-M 1.4.1.

What Does TF-M Offer?
*********************

Through a set of secure services and by design, TF-M provides:

* Isolation of secure and non-secure resources
* Embedded-appropriate crypto
* Management of device secrets (keys, etc.)
* Firmware verification (and encryption)
* Protected off-chip data storage and retrieval
* Proof of device identity (device attestation)
* Audit logging

Build System Integration
************************

When using TF-M with a supported platform, TF-M will be automatically built and
link in the background as part of the standard Zephyr build process. This
build process makes a number of assumptions about how TF-M is being used, and
has certain implications about what the Zephyr application image can and can
not do:

* The secure processing environment (secure boot and TF-M) starts first
* Resource allocation for Zephyr relies on choices made in the secure image.

Architecture Overview
*********************

A TF-M application will, generally, have the following three parts, from most
to least trusted, left-to-right, with code execution happening in the same
order (secure boot > secure image > ns image).

While the secure bootloader is optional, it is enabled by default, and secure
boot is an important part of providing a secure solution:

::

    +-------------------------------------+           +--------------+
    | Secure Processing Environment (SPE) |           |     NSPE     |
    | +----------++---------------------+ |           | +----------+ |
    | |          ||                     | |           | |          | |
    | | bl2.bin  ||  tfm_s_signed.bin   | |           | |zephyr.bin| |
    | |          ||                     | | <- PSA -> | |          | |
    | |  Secure  || Trusted Firmware-M  | |    APIs   | |  Zephyr  | |
    | |   Boot   ||   (Secure Image)    | |           | |(NS Image)| |
    | |          ||                     | |           | |          | |
    | +----------++---------------------+ |           | +----------+ |
    +-------------------------------------+           +--------------+

Communication between the (Zephyr) Non-Secure Processing Environment (NSPE) and
the (TF-M) Secure Processing Environment image happens based on a set of PSA
APIs, and normally makes use of an IPC mechanism that is included as part of
the TF-M build, and implemented in Zephyr
(see :zephyr_file:`modules/trusted-firmware-m/interface`).

Root of Trust (RoT) Architecture
================================

TF-M is based upon a **Root of Trust (RoT)** architecture. This allows for
hierarchies of trust from most, to less, to least trusted, providing a sound
foundation upon which to build or access trusted services and resources.

The benefit of this approach is that less trusted components are prevented from
accessing or compromising more critical parts of the system, and error
conditions in less trusted environments won't corrupt more trusted, isolated
resources.

The following RoT hierarchy is defined for TF-M, from most to least trusted:

* PSA Root of Trust (**PRoT**), which consists of:

  * PSA Immutable Root of Trust: secure boot
  * PSA Updateable Root of Trust: most trusted secure services
* Application Root of Trust (**ARoT**): isolated secure services

The **PSA Immutable Root of Trust** is the most trusted piece of code in the
system, to which subsequent Roots of Trust are anchored. In TF-M, this is the
secure boot image, which verifies that the secure and non-secure images are
valid, have not been tampered with, and come from a reliable source. The
secure bootloader also verifies new images during the firmware update process,
thanks to the public signing key(s) built into it. As the name implies,
this image is **immutable**.

The **PSA Updateable Root of Trust** implements the most trusted secure
services and components in TF-M, such as the Secure Partition Manager (SPM),
and shared secure services like PSA Crypto, Internal Trusted Storage (ITS),
etc. Services in the PSA Updateable Root of Trust have access to other
resources in the same Root of Trust.

The **Application Root of Trust** is a reduced-privilege area in the secure
processing environment which, depending on the isolation level chosen when
building TF-M, has limited access to the PRoT, or even other ARoT services at
the highest isolation levels. Some standard services exist in the ARoT, such as
Protected Storage (PS), and generally custom secure services that you implement
should be placed in the ARoT, unless a compelling reason is present to place
them in the PRoT.

These divisions are distinct from the **untrusted code**, which runs in the
non-secure environment, and has the least privilege in the system. This is the
Zephyr application image in this case.

Isolation Levels
----------------

At present, there are three distinct **isolation levels** defined in TF-M,
with increasingly rigid boundaries between regions. The isolation level used
will depend on your security requirements, and the system resources available
to you.

* **Isolation Level 1** is the lowest isolation level, and the only major
  boundary is between the secure and non-secure processing environment,
  usually by means of Arm TrustZone on Armv8-M processors. There is no
  distinction here between the PSA Updateable Root of Trust (PRoT) and the
  Application Root of Trust (ARoT). They execute at the same privilege level.
  This isolation level will lead to the smallest combined application images.
* **Isolation Level 2** builds upon level one by introducing a distinction
  between the PSA Updateable Root of Trust and the Application Root of Trust,
  where ARoT services have limited access to PRoT services, and can only
  communicate with them through public APIs exposed by the PRoT services.
  ARoT services, however, are not strictly isolated from one another.
* **Isolation Level 3** is the highest isolation level, and builds upon level
  2 by isolating ARoT services from each other, so that each ARoT is
  essentially silo'ed from other services. This provides the highest level of
  isolation, but also comes at the cost of additional overhead and code
  duplication between services.

The current isolation level can be checked via
:kconfig:option:`CONFIG_TFM_ISOLATION_LEVEL`.

Secure Boot
===========

The default secure bootloader in TF-M is based on
`MCUBoot <https://www.mcuboot.com/>`__, and is referred to as ``BL2`` in TF-M
(for the second-stage bootloader, potentially after a HW-based bootloader on
the secure MCU, etc.).

All images in TF-M are hashed and signed, with the hash and signature verified
by MCUBoot during the firmware update process.

Some key features of MCUBoot as used in TF-M are:

* Public signing key(s) are baked into the bootloader
* S and NS images can be signed using different keys
* Firmware images can optionally be encrypted
* Client software is responsible for writing a new image to the secondary slot
* By default, uses static flash layout of two identically-sized memory regions
* Optional security counter for rollback protection

When dealing with (optionally) encrypted images:

* Only the payload is encrypted (header, TLVs are plain text)
* Hashing and signing are applied over the un-encrypted data
* Uses ``AES-CTR-128`` or ``AES-CTR-256`` for encryption
* Encryption key randomized every encryption cycle (via ``imgtool``)
* The ``AES-CTR`` key is included in the image and can be encrypted using:

  * ``RSA-OAEP``
  * ``AES-KW`` (128 or 256 bits depending on the ``AES-CTR`` key length)
  * ``ECIES-P256``
  * ``ECIES-X25519``

Key config properties to control secure boot in Zephyr are:

* :kconfig:option:`CONFIG_TFM_BL2` toggles the bootloader (default = ``y``).
* :kconfig:option:`CONFIG_TFM_KEY_FILE_S` overrides the secure signing key.
* :kconfig:option:`CONFIG_TFM_KEY_FILE_NS` overrides the non-secure signing key.

Secure Processing Environment
=============================

Once the secure bootloader has finished executing, a TF-M based secure image
will begin execution in the **secure processing environment**. This is where
our device will be initially configured, and any secure services will be
initialised.

Note that the starting state of our device is controlled by the secure firmware,
meaning that when the non-secure Zephyr application starts, peripherals may
not be in the HW-default reset state. In case of doubts, be sure to consult
the board support packages in TF-M, available in the ``platform/ext/target/``
folder of the TF-M module (which is in ``modules/tee/tf-m/trusted-firmware-m/``
within a default Zephyr west workspace.)

Secure Services
---------------

As of TF-M 1.4.1, the following secure services are generally available (although vendor support may vary):

* Audit Logging (Audit)
* Crypto (Crypto)
* Firmware Update (FWU)
* Initial Attestation (IAS)
* Platform (Platform)
* Secure Storage, which has two parts:

  * Internal Trusted Storage (ITS)
  * Protected Storage (PS)

A template also exists for creating your own custom services.

For full details on these services, and their exposed APIs, please consult the
`TF-M Documentation <https://tf-m-user-guide.trustedfirmware.org/>`__.

Key Management and Derivation
-----------------------------

Key and secret management is a critical part of any secure device. You need to
ensure that key material is available to regions that require it, but not to
anything else, and that it is stored securely in a way that makes it difficult
to tamper with or maliciously access.

The **Internal Trusted Storage** service in TF-M is used by the **PSA Crypto**
service (which itself makes use of mbedtls) to store keys, and ensure that
private keys are only ever accessible to the secure processing environment.
Crypto operations that make use of key material, such as when signing payloads
or when decrypting sensitive data, all take place via key handles. At no point
should the key material ever be exposed to the NS environment.

One exception is that private keys can be provisioned into the secure
processing environment as a one-way operation, such as during a factory
provisioning process, but even this should be avoided where possible, and a
request should be made to the SPE (via the PSA Crypto service) to generate a
new private key itself, and the public key for that can be requested during
provisioning and logged in the factory. This ensures the private key
material is never exposed, or even known during the provisioning phase.

TF-M also makes extensive use of the **Hardware Unique Key (HUK)**, which
every TF-M device must provide. This device-unique key is used by the
**Protected Storage** service, for example, to encrypt information stored in
external memory. For example, this ensures that the contents of flash memory
can't be decrypted if they are removed and placed on a new device, since each
device has its own unique HUK used while encrypting the memory contents
the first time.

HUKs provide an additional advantage for developers, in that they can be used
to derive new keys, and the **derived keys** don't need to be stored since
they can be regenerated from the HUK at startup, using an additional salt/seed
value (depending on the key derivation algorithm used). This removes the
storage issue and a frequent attack vector. The HUK itself it usually highly
protected in secure devices, and inaccessible directly by users.

``TFM_CRYPTO_ALG_HUK_DERIVATION`` identifies the default key derivation
algorithm used if a software implementation is used. The current default
algorithm is ``HKDF`` (RFC 5869) with a SHA-256 hash. Other hardware
implementations may be available on some platforms.

Non-Secure Processing Environment
=================================

Zephyr is used for the NSPE, using a board that is supported by TF-M where the
:kconfig:option:`CONFIG_BUILD_WITH_TFM` flag has been enabled.

Generally, you simply need to select the ``*_ns`` variant of a valid target
(for example ``mps2_an521_ns``), which will configure your Zephyr application
to run in the NSPE, correctly build and link it with the TF-M secure images,
sign the secure and non-secure images, and merge the three binaries into a
single ``tfm_merged.hex`` file. The :ref:`west flash <west-flashing>` command
will flash ``tfm_merged.hex`` by default in this configuration.

At present, Zephyr can not be configured to be used as the secure processing
environment.
