.. zephyr:code-sample:: fido2
   :name: FIDO2 Authenticator
   :relevant-api: fido2

   Implement a FIDO2/CTAP2 hardware security key.

Overview
********

This sample turns a Zephyr-supported board into a FIDO2 hardware security
key. It implements the CTAP2.1 protocol. Currently only USB HID (CTAPHID)
is supported. The sample can be used for passwordless authentication on
websites that support WebAuthn, such as `webauthn.io <https://webauthn.io>`_.

Supported operations:

- ``authenticatorMakeCredential``
- ``authenticatorGetAssertion`` and ``authenticatorGetNextAssertion``
- ``authenticatorGetInfo``
- ``authenticatorSelection``

Requirements
************

A board with USB device support. The sample has been tested with:

- Black Pill STM32H523 (``blackpill_h523ce``)
- STM32WB55 Core Board (``weact_stm32wb55_core``)
- ESP32-S3-B (``weact_esp32s3_b/esp32s3/procpu``)

User presence is confirmed by pressing either the chosen button ``zephyr,fido2-up-button``
or the button aliased to ``sw0`` as a fallback.

The FIDO2 runtime state can be monitored via the LED aliased to ``led0``. The
subsystem exposes the runtime state that the sample uses to control the LED:

- LED off: FIDO2 idle or stopped
- LED blinking: waiting for user presence
- LED on: processing a request

Building and Running
********************
For the Black Pill STM32H523 board:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/authentication/fido2
   :board: blackpill_h523ce
   :goals: build flash

After flashing, connect the board to your computer via its USB port.
Open `webauthn.io <https://webauthn.io>`_ in Chrome or Firefox:

1. Enter a username and click **Register**.
2. The browser prompts for a security key. Press the user-presence button
   on the board.
3. Registration should succeed.
4. Click **Authenticate** and press the button again to log in.

.. note::

   This sample does not implement clientPin. Chromium-based browsers may
   require clientPin for discoverable credentials, even though it is not
   enforced by the FIDO2 specification. Use non-discoverable credentials
   on Chrome, or use Firefox or Safari.
