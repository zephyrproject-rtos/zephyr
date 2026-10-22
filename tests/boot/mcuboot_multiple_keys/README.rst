MCUboot Multiple Signing Keys
#############################

Runtime acceptance test for an MCUboot bootloader built with more than one
verification key: a single bootloader embeds two keys and boots an application
signed by *either* one, exercised entirely under QEMU.

:kconfig:option:`SB_CONFIG_BOOT_SIGNATURE_KEY_FILE` takes a comma-separated list
of keys. MCUboot embeds the public half of each; the application is signed with
the first. This test models a *development* bootloader that trusts both a
development and a production key, using keys shipped with MCUboot:

* ``key_id 0`` -- ``root-ed25519.pem``, the development key whose private half
  signs the application in the default build;
* ``key_id 1`` -- ``root-ed25519-2-pub.pem``, the production key's public half
  only: the bootloader trusts production-signed images without ever holding the
  production private key.

The two scenarios differ only in how the application is signed:

* **key0** boots the application as built -- signed with the development key.
* **key1** re-signs the built application with the production private key
  (``root-ed25519-2.pem``) *after* the build and boots that, modelling a
  production custodian who signs the release binary out-of-band. The re-sign
  reuses the build's own imgtool invocation; see ``pytest/test_multiple_keys.py``.

How the QEMU Test Works
*******************************

A ``harness: pytest`` test (``pytest/test_multiple_keys.py``) starts QEMU with
two ``-device loader`` entries -- MCUboot's hex and the signed application
preloaded into slot0 -- and reads the console. With
:kconfig:option:`CONFIG_MCUBOOT_LOG_LEVEL_DBG` MCUboot prints
``bootutil_verify_sig: ED25519 key_id N``, the runtime proof of which embedded
key validated the image; the test asserts the expected ``key_id`` and that the
application banner then prints from the primary slot.
