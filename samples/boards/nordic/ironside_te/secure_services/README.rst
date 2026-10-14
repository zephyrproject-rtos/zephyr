.. zephyr:code-sample:: ironside_te_secure_services
   :name: Nordic IronSide TE secure services

   Call the IronSide TE boot report, PSA Crypto and PSA Initial Attestation services.

Overview
********

IronSide TE exposes all of its services behind a single secure-gateway entry.
The sample exercises three of them from a non-secure application:

* **The boot report**, which says which image IronSide TE booted and the device's lifecycle state, the version of every image in the boot chain, and whether an update candidate was applied this boot.
  The sample logs the booted image, the lifecycle state and the versions.
* **PSA Crypto**, which IronSide TE runs on the application's behalf.
  The application uses TF-PSA-Crypto's standard ``<psa/crypto.h>`` while the IronSide TE client dispatches the supported functions to IronSide TE, so ordinary PSA code needs no change.
  The sample draws random bytes with ``psa_generate_random()``.
* **PSA Initial Attestation**, which returns a signed token over a challenge.
  The sample uses random bytes as the challenge and checks the token against the size the service announced.
  It does not decode the token: the claims are found by label with a CBOR parser, which the IronSide Integration Kit's ``psa_initial_attestation`` sample shows in full.

The remaining device-management service, ``ironside_devmgmt_ap_protect()``, is callable but deliberately not called here.
It closes the non-secure access port permanently, which is a production step rather than something to run on a development kit.

Requirements
************

The sample runs on the nRF54LM20 DK as ``nrf54lm20dk/nrf54lm20a/cpuapp/ite``.

The kit must already have IronSide TE installed, and the build needs the keyset that the device was provisioned from.
See :file:`ironsdk/README.md` for that bring-up.

Building and running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nordic/ironside_te/secure_services
   :board: nrf54lm20dk/nrf54lm20a/cpuapp/ite
   :goals: flash

The application is signed with the version in :kconfig:option:`CONFIG_IRONSIDE_TE_SIGN_VERSION`, and IronSide TE derives a rollback counter from it.
Once the kit has booted a given version, it refuses anything below it, so rebuilding at a lower version leaves the kit in DFU mode until you either sign higher or reinstall IronSide TE with ``ironside_install.py <bundle> --force``.
