.. _nfc_subsystem:

Near Field Communication (NFC)
##############################

Overview
********

The NFC subsystem reads and writes NFC Forum tags. It drives a controller from
the NFC driver class described in :ref:`nfc_api` as a poller, and provides NFC-A
activation, ISO-DEP, Type 2 and Type 4 tag access and an NDEF codec.

The same application code drives every supported controller. Where controllers
differ, the subsystem adapts; see `Controller classes`_.

Poller session
**************

A :c:struct:`nfc_poller` binds one NFC device. :c:func:`nfc_poller_start`
selects the protocols to run and resolves once, for the whole session, how the
device is driven.

The poller owns a session lock. A sequence that spans several frames, such as
reading an NDEF message, holds it for its whole duration, so a second thread
cannot interleave frames and leave the target in an undefined state.

.. code-block:: c

   NFC_POLLER_DEFINE(poller, DEVICE_DT_GET(DT_ALIAS(nfc0)));

   nfc_poller_start(&poller, NFC_PROTO_ISO14443A);

Discovery
*********

:c:func:`nfc_discover` returns one activated target, or ``-EAGAIN`` if none
answered before the deadline. The result identifies its own technology, so a
caller that only reads NDEF can pass it straight to :c:func:`nfc_tag_connect`.

The RF field is removed between poll cycles, as the NFC Forum Digital
specification requires, so a target still on the antenna is found again on the
next cycle. A reader that must act once per presentation compares
:c:func:`nfc_target_uid` against the previous result.

A target that is discovered but not connected to is let go with
:c:func:`nfc_target_release`, which leaves it in a state anticollision passes
over so the other targets in the field are reached.

Tag access
**********

:c:func:`nfc_tag_connect` resolves the tag type from the NFC-A ``SEL_RES``
(SAK) and runs the connect sequence for that type. A MIFARE Classic is not an
NFC Forum tag and is rejected with ``-ENOTSUP``.

.. code-block:: c

   struct nfc_tag tag;
   uint8_t buf[256];
   uint16_t len = sizeof(buf);

   nfc_tag_connect(&poller, &target, &tag, K_SECONDS(2));
   nfc_tag_read_ndef(&tag, buf, &len, K_SECONDS(1));
   nfc_tag_close(&tag, K_MSEC(100));

:c:func:`nfc_tag_close` ends the tag-type protocol and releases the target.
:c:func:`nfc_tag_read_ndef` reports ``-ENOENT`` when the tag holds no NDEF
message, which is distinct from a tag that cannot hold one.

Raw block access to a Type 2 tag is available through :c:func:`nfc_t2t_read_block`
and :c:func:`nfc_t2t_write_block` for callers that need the memory layout rather
than NDEF.

Controller classes
******************

The driver class covers controllers that differ in which side owns the protocol.
The subsystem resolves this once per session and branches there, not per frame:

* On a **host-driven frontend**, the subsystem runs anticollision itself, sends
  RATS and parses the answer to select (ATS). Where the controller has no
  hardware CRC or timeout, the subsystem substitutes software.

* On an **offloading controller**, the controller has already activated the
  target and reports the ATS in the activation result. The subsystem feeds that
  to the same parser, sends no RATS, and passes application blocks to the
  controller instead of framing them.

Callers see no difference.

Configuration
*************

* :kconfig:option:`CONFIG_NFC` enables the subsystem.
* :kconfig:option:`CONFIG_NFC_T2T` and :kconfig:option:`CONFIG_NFC_T4T` select
  which tag types are built.
* :kconfig:option:`CONFIG_NFC_ISO14443_FSD_MAX` is the largest frame the reader
  accepts, set from the FSD choice. Raising it costs stack in the ISO-DEP
  exchange.

Samples
*******

* :zephyr:code-sample:`nfc-tag-reader`

API Reference
*************

.. doxygengroup:: nfc_poller

.. doxygengroup:: nfc_tag

.. doxygengroup:: nfc_t2t

.. doxygengroup:: nfc_iso_dep

.. doxygengroup:: nfc_ndef
