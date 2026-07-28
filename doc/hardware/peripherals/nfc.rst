.. _nfc_api:

Near Field Communication (NFC)
##############################

Overview
********

NFC is a short-range wireless technology operating at 13.56 MHz, standardised
across ISO/IEC 14443, ISO/IEC 15693 and JIS X 6319-4, and profiled by the NFC
Forum. A device takes a role on the RF interface:

* A **poller** (also called initiator or reader) generates the RF field and
  starts every exchange.
* A **listener** (also called target or card) draws power from another device's
  field and only answers.

Two devices can also talk as peers over NFC-DEP. The API reserves
``NFC_MODE_P2P`` for it, but no in-tree driver implements it.

The NFC driver API describes what a controller can do on that interface. It
does not implement tag types or NDEF; those belong above the driver class.

Controller classes
******************

NFC hardware is not uniform, and the API is deliberately shaped so that a
controller only implements the operations it actually has. Three classes are
covered, and a driver is recognised by which operations it provides:

.. list-table::
   :header-rows: 1

   * - Class
     - Provides
     - In-tree driver
   * - Offloading controller
     - :c:func:`nfc_offload_poll_start`, :c:func:`nfc_offload_poll_stop`,
       :c:func:`nfc_offload_exchange`, :c:func:`nfc_offload_release`
     - NXP PN532
   * - Host-driven frontend, poller
     - :c:func:`nfc_initiator_transceive`, :c:func:`nfc_load_protocol`,
       :c:func:`nfc_claim`
     - NXP MFRC522
   * - Host-driven frontend, listener
     - :c:func:`nfc_target_start`, :c:func:`nfc_target_stop`,
       :c:func:`nfc_target_send`
     - None

An offloading controller resolves collisions and activates targets in its own
firmware and reports the result through a callback, so the host never sees
individual frames. A frontend exposes the raw RF interface, and the host drives
anticollision and framing itself.

Most operations a controller does not provide report ``-ENOSYS``, so a caller
can tell at runtime what a device supports. The exceptions are the ones with a
meaningful empty answer: :c:func:`nfc_claim` and :c:func:`nfc_release` succeed
as a no-op on a controller that has no host-driven sequence to serialise, and
:c:func:`nfc_supported_protocols` and :c:func:`nfc_supported_modes` report
none.

Emulated controller
*******************

A build without NFC hardware can still exercise the API through the emulated
controller, which impersonates one of the three classes above and answers from
a script the test supplies. See :ref:`nfc_emul_api` and the tests under
:zephyr_file:`tests/drivers/nfc/api`.

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_NFC_DRIVERS`
* :kconfig:option:`CONFIG_NFC_EMUL`

API Reference
*************

.. doxygengroup:: nfc_interface

.. _nfc_emul_api:

Emulated NFC controller API
===========================

.. doxygengroup:: nfc_emul
