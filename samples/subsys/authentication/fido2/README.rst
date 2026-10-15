.. zephyr:code-sample:: fido2
   :name: FIDO2 Authenticator
   :relevant-api: fido2

   Implement a FIDO2/CTAP2 hardware security key.

Overview
********

This sample turns a Zephyr-supported board into a FIDO2 hardware security
key. It implements the CTAP2.1 protocol over USB HID (CTAPHID), with optional
Bluetooth Low Energy (Bluetooth LE) transport support.

The nRF54LM20 board configuration enables both USB HID and Bluetooth LE
transports, which are available at the same time. The other supported board
configurations use USB HID only.

The sample can be used for passwordless authentication on
websites that support WebAuthn, such as `webauthn.io <https://webauthn.io>`_.

Supported operations:

- ``authenticatorMakeCredential``
- ``authenticatorGetAssertion``
- ``authenticatorGetInfo``
- ``authenticatorClientPIN``
- ``authenticatorGetNextAssertion``
- ``authenticatorSelection``

Bluetooth Low Energy
====================

When using Bluetooth LE transport, the client must pair with the authenticator
before using FIDO2 operations.

For simplicity, the sample remains discoverable and pairable while running.
Production applications should restrict new pairing to an explicit
user-initiated pairing mode.

During testing with WebAuthn clients, Bluetooth LE operations can take longer than
USB HID operations because the Bluetooth LE connection may be disconnected and
re-established between operations.

Requirements
************

For USB HID, a board with USB device support is required. The sample has been tested with:

- Black Pill STM32H523 (``blackpill_h523ce``)
- STM32WB55 Core Board (``weact_stm32wb55_core``)
- ESP32-S3-B (``weact_esp32s3_b/esp32s3/procpu``)
- nRF54LM20DK (``nrf54lm20dk/nrf54lm20a/cpuapp``)

For Bluetooth LE, a board with Bluetooth peripheral support is required. The sample has
been tested with:

- nRF54LM20DK (``nrf54lm20dk/nrf54lm20a/cpuapp``)

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
Open `webauthn.io <https://webauthn.io>`_ in any WebAuthn-compatible
browser (e.g., Chrome, Edge, Firefox, Safari) and follow these steps:

1. Enter a username and click **Register**.
2. The browser prompts for a security key. Press the user-presence button on the board.
3. Setup or enter a PIN if prompted.
4. Registration should succeed.
5. Click **Authenticate**, press the button again, and enter pin to log in.
