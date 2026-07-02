.. _fido2_api:

FIDO2 Authenticator
###################

Overview
********

The FIDO2 authenticator subsystem implements the `FIDO2 CTAP2 Specification`_
(Client to Authenticator Protocol), allowing a Zephyr device to act as a
hardware security key for passwordless authentication. The subsystem can be
enabled with the :kconfig:option:`CONFIG_FIDO2` option.

FIDO2 security keys are used with the `WebAuthn Specification`_ web standard.
A relying party (website or service) interacts with the authenticator through
a client (browser or OS platform) to register and verify user credentials. The
authenticator performs cryptographic operations using on-device keys that
never leave the hardware.

The subsystem currently supports the following CTAP2 commands:

- ``authenticatorMakeCredential``
- ``authenticatorGetAssertion``
- ``authenticatorGetInfo``
- ``authenticatorGetNextAssertion``
- ``authenticatorSelection``

Architecture
************

The subsystem is organized into pluggable backend components, each selectable
at build time via Kconfig:

Transport
   Handles wire-protocol communication between the host and the authenticator.
   Transports are registered using the :c:macro:`FIDO2_TRANSPORT_DEFINE` macro
   and are iterated at startup.
   Available transports:

   - **USB HID (CTAPHID)** — :kconfig:option:`CONFIG_FIDO2_TRANSPORT_USB_HID`

User Presence (UP)
   Confirms that a human is physically present. Backends are selected via
   :kconfig:option:`CONFIG_FIDO2_UP_BACKEND`:

   - **Input device** — :kconfig:option:`CONFIG_FIDO2_UP_INPUT`
   - **Always approve** — :kconfig:option:`CONFIG_FIDO2_UP_ALWAYS`
   - **Custom** — :kconfig:option:`CONFIG_FIDO2_UP_CUSTOM` (application-provided)

Credential Storage
   Persists discoverable (resident) credentials. Backends are
   selected via :kconfig:option:`CONFIG_FIDO2_STORAGE_BACKEND`:

   - **Settings subsystem** — :kconfig:option:`CONFIG_FIDO2_STORAGE_SETTINGS`
   - **None** — :kconfig:option:`CONFIG_FIDO2_STORAGE_NONE` (non-discoverable credentials only)

Attestation
   Signs newly created credentials to prove their origin. Backends are selected
   via :kconfig:option:`CONFIG_FIDO2_ATTESTATION_BACKEND`:

   - **Self attestation** — :kconfig:option:`CONFIG_FIDO2_ATTESTATION_SELF` (default)
   - **None** — :kconfig:option:`CONFIG_FIDO2_ATTESTATION_NONE`
   - **Custom** — :kconfig:option:`CONFIG_FIDO2_ATTESTATION_CUSTOM` (application-provided)

Usage
*****

To use the FIDO2 subsystem, include the main header:

.. code-block:: c

   #include <zephyr/authentication/fido2/fido2.h>

Basic Initialization
====================

At least one transport must be enabled for the authenticator to communicate
with a host.

See :zephyr:code-sample:`fido2` for a complete initialization sequence.

Runtime State Monitoring
========================

The subsystem exposes a runtime state callback that applications can use to
drive status indicators such as LEDs:

.. code-block:: c

   #include <zephyr/authentication/fido2/fido2.h>

   static void on_state_change(enum fido2_runtime_state state, void *user_data)
   {
       switch (state) {
       case FIDO2_RUNTIME_STATE_IDLE:
           /* LED off */
           break;
       case FIDO2_RUNTIME_STATE_WAITING_USER_PRESENCE:
           /* Blink LED */
           break;
       case FIDO2_RUNTIME_STATE_PROCESSING:
           /* LED on solid */
           break;
       default:
           break;
       }
   }

   fido2_set_state_callback(on_state_change, NULL);

Extensions
**********

CTAP2 extensions are not implemented yet. The following Kconfig options
exist for future implementation:

- **credProtect** — :kconfig:option:`CONFIG_FIDO2_EXT_CRED_PROTECT`
- **hmac-secret** — :kconfig:option:`CONFIG_FIDO2_EXT_HMAC_SECRET`
- **largeBlobKey** — :kconfig:option:`CONFIG_FIDO2_EXT_LARGE_BLOB_KEY`
- **credBlob** — :kconfig:option:`CONFIG_FIDO2_EXT_CRED_BLOB`
- **thirdPartyPayment** — :kconfig:option:`CONFIG_FIDO2_EXT_THIRD_PARTY_PAYMENT`

References
**********

* `FIDO2 CTAP2 Specification`_

.. _FIDO2 CTAP2 Specification:
   https://fidoalliance.org/specs/fido-v2.2-rd-20230321/fido-client-to-authenticator-protocol-v2.2-rd-20230321.html

* `WebAuthn Specification`_

.. _WebAuthn Specification:
   https://www.w3.org/TR/webauthn-2/

* `FIDO Alliance`_

.. _FIDO Alliance:
   https://fidoalliance.org/

API Reference
*************

.. doxygengroup:: fido2
