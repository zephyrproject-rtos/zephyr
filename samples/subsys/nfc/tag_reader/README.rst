.. zephyr:code-sample:: nfc-tag-reader
   :name: NFC tag reader
   :relevant-api: nfc_tag nfc_poller nfc_ndef

   Read the NDEF message from a tag presented to an NFC reader.

Overview
********

Polls for an NFC-A tag, connects to it, and prints the records of its NDEF
message. The tag type is detected during :c:func:`nfc_tag_connect`, so the same
code reads a Type 2 Tag and a Type 4 Tag.

Requirements
************

A board with an NFC controller assigned to the ``nfc0`` devicetree alias, and a
tag to present to it. ``native_sim`` builds against the emulated controller,
which answers nothing, so the sample only reports that it is waiting.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/nfc/tag_reader
   :board: native_sim
   :goals: build run
   :compact:

Sample Output
*************

.. code-block:: console

   *** Booting Zephyr OS build v4.4.0 ***
   [00:00:00.000,000] <inf> nfc_tag_reader: Present a tag to pn532@24
   [00:00:03.412,000] <inf> nfc_tag_reader: UID
                                            04 8a 2b c1 5e 71 80          |..+..q.
   [00:00:03.418,000] <inf> nfc_tag_reader: Type 2 Tag
   [00:00:03.441,000] <inf> nfc_tag_reader:   record 0: TNF 1, 19 byte payload
   [00:00:03.441,000] <inf> nfc_tag_reader:   type
                                            55                            |U
   [00:00:03.441,000] <inf> nfc_tag_reader:   payload
                                            04 7a 65 70 68 79 72 70  72 6f 6a 65 63 74 2e 6f |.zephyrp rojects.o
                                            72 67                                            |rg
