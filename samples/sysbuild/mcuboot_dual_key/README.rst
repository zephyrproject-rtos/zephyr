.. zephyr:code-sample:: mcuboot_dual_key
   :name: MCUboot dual signing key with sysbuild

   Build a development bootloader that accepts images signed with
   either a local development key or a remote production public key.

Overview
********

This sample demonstrates the canonical real-world use of
``SB_CONFIG_BOOT_SIGNATURE_KEY_FILE_2``: a **development bootloader**
that accepts images signed with either the team's development key or
the production key, while production bootloaders (built separately)
accept only the production key.

The security-critical property modelled here is that the development
workstation **never holds the production private key**. The production
team signs release images on a locked-down build server and
distributes only the public half of their signing key to development
workstations. That public half is embedded into development
bootloaders via ``SB_CONFIG_BOOT_SIGNATURE_KEY_FILE_2`` — no private
material is required or present.

Keys used by this sample
========================

- **Primary key** — represents the developer's own signing key. This
  sample inherits the ED25519 Kconfig default
  (``$ZEPHYR_MCUBOOT_MODULE_DIR/root-ed25519.pem``), a publicly-known
  MCUboot test key standing in for a private dev key.
- **Second key** — :file:`keys/prod_pubkey.pem`, a PEM file containing
  **only the public half** of a keypair. It was extracted from
  MCUboot's ``root-ed25519-2.pem`` test key with
  ``imgtool getpub --encoding pem``, simulating what a dev workstation
  would receive from a production team.

:file:`sysbuild.conf` references :file:`keys/prod_pubkey.pem` with a
plain relative path. Sysbuild resolves application-owned signing-key
paths against the application source directory before forwarding them
to the MCUboot child image.

All keys here derive from MCUboot's publicly-available test keys and
are insecure. Substitute your own ED25519 keypair when adopting this
pattern for production.

Files
=====

- :file:`sysbuild.conf` — enables MCUboot with ED25519 signing,
  inherits the primary key from the Kconfig default, and points the
  second key at the sample-shipped public-only PEM.
- :file:`keys/prod_pubkey.pem` — public-only PEM shipped with the
  sample.
- :file:`sysbuild/mcuboot.conf` — MCUboot-specific Kconfig
  adjustments.

Building and running
********************

.. zephyr-app-commands::
   :tool: west
   :zephyr-app: samples/sysbuild/mcuboot_dual_key
   :board: nrf52840dk/nrf52840
   :goals: build flash
   :west-args: --sysbuild
   :compact:

The default build signs the application with the primary (development)
key. Expected console output:

.. code-block:: console

   *** Booting MCUboot ***
   *** Booting Zephyr OS ***
   Address of sample 0xc000
   Hello mcuboot dual key! nrf52840dk/nrf52840

What the build produces
***********************

After ``west build``, two flashable artifacts exist side by side:

- :file:`build/mcuboot/zephyr/zephyr.hex` — the MCUboot bootloader
  with **both public keys embedded** (the primary from
  ``root-ed25519.pem`` and the secondary from
  :file:`keys/prod_pubkey.pem`). Inspect
  :file:`build/mcuboot/zephyr/autogen-pubkey.c` and
  :file:`autogen-pubkey2.c` to see the embedded byte arrays. Neither
  file contains private key material — both are pure public-key data.
- :file:`build/mcuboot_dual_key/zephyr/zephyr.signed.hex` — the
  application, signed by imgtool with the **primary (development)
  private key only**. The production private key is not on this
  workstation and was not used.

``west flash`` chains both to the target automatically (the flash
order is recorded in :file:`build/domains.yaml`). For a single
combined hex, enable :kconfig:option:`SB_CONFIG_MERGED_HEX_FILES` in
:file:`sysbuild.conf` and sysbuild will emit
:file:`build/merged_<board_target>.hex`. See
:ref:`sysbuild_merged_hex_files`.

The security property this build demonstrates
=============================================

A developer with only :file:`keys/prod_pubkey.pem` (no production
private key) can produce a bootloader that accepts production-signed
images. They cannot themselves produce such an image — only the
production team, who holds the private half, can. This is the
separation of duties a dual-key bootloader is designed to enforce.

Which key signs the application?
********************************

The application is signed at build time by imgtool, using the
**primary** key (``SB_CONFIG_BOOT_SIGNATURE_KEY_FILE``). The second
key is never used for signing — it exists only for bootloader-side
verification. This matches the security model: a development
workstation should only hold the private key it is authorized to
sign with.

To change which key signs the application, point
``SB_CONFIG_BOOT_SIGNATURE_KEY_FILE`` in :file:`sysbuild.conf` at the
desired private key and rebuild. On a development workstation that is
typically the local dev key; on a production build server it would be
the production private key. Both types of workstation would use the
same ``SB_CONFIG_BOOT_SIGNATURE_KEY_FILE_2`` value (embedding the
*other* role's public key), producing bootloader binaries that
accept either side's signed images.

Verifying second-key acceptance
*******************************

To confirm the bootloader accepts images signed with the production
key, re-sign the application slot with MCUboot's
``root-ed25519-2.pem`` — the *private* half of the same keypair whose
public half ships as :file:`keys/prod_pubkey.pem` — and reflash only
the application:

.. code-block:: console

   west sign -t imgtool -d build/mcuboot_dual_key \
       -- --key ${ZEPHYR_MCUBOOT_MODULE_DIR}/root-ed25519-2.pem

The bootloader should verify and chain-load the image identically.
This simulates a release-signed image arriving at a development
workstation for testing — the workstation runs it without ever
possessing the production signing key.

In practice the release team would sign production images on their own
build server and hand the dev team a signed binary; the
``root-ed25519-2.pem`` invocation above only stands in for "here is
what a production-signed image looks like" so the full loop can be
demonstrated from one machine.

As a negative check, signing with ``root-ed25519-unknown.pem`` (also
shipped in MCUboot for testing) should cause MCUboot to reject the
image at boot.

How the sample proves the public-only property
**********************************************

After a default build, inspect :file:`build/mcuboot/zephyr/.config`
and :file:`build/mcuboot/zephyr/autogen-pubkey2.c`:

- ``CONFIG_BOOT_SIGNATURE_KEY_FILE_2`` points at
  :file:`keys/prod_pubkey.pem` — a PEM file whose only content is
  ``-----BEGIN PUBLIC KEY-----``.
- ``autogen-pubkey2.c`` contains a valid ED25519 public key byte
  array. No private-key material was involved at any point.

Attempting to ``imgtool sign`` with the sample's
:file:`prod_pubkey.pem` fails (as it should — signing requires a
private key).
