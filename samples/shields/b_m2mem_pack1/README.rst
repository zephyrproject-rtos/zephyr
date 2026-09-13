.. zephyr:code-sample:: b-m2mem-pack1
   :name: B-M2MEM-PACK1 shield
   :relevant-api: flash_interface eeprom_interface gpio_interface

   Check a B-M2MEM-PACK1 memory module against the data stored in its
   identification EEPROM.

Overview
********

The :ref:`st_b_m2mem_pack1_shield` carries two devices: a serial NOR flash and
an I2C EEPROM that describes the module. This sample reads the EEPROM, prints
what the module claims to be, and then runs three tests on the flash to check
that the claim holds and that the memory is usable.

The tests are:

#. **Compare JEDEC IDs.** Read the JEDEC ID from the flash at runtime and
   compare it with the value the identification EEPROM reports for the memory
   position. A mismatch means the module is not what its EEPROM says it is.
#. **Erase.** Erase one sector and check that the operation succeeds. The
   remaining test depends on this one, so the sample stops if it fails.
#. **Write and read back.** Write a known pattern into the erased sector and
   read it back.

The first test needs the ``jedec-id`` property on the flash node. Shield
variants that do not carry it skip that test and run the other two.

Results go to the console and to the two shield LEDs, so the sample is also
usable without a serial terminal:

=================  ==========================================================
LED behaviour      Meaning
=================  ==========================================================
Alternating blink  A test is about to start
Green for 2 s      The test passed
Red for 2 s        The test failed
Both steady on     All tests completed
=================  ==========================================================

Requirements
************

A Nucleo-144 board that exposes the ST M.2 Key A serial memory connector, with
a B-M2MEM-PACK1 module fitted. The tests write to the flash at offset
``0xff000``, so any data stored there is lost.

Building and Running
********************

Build for the shield variant that matches the fitted module:

.. zephyr-app-commands::
   :zephyr-app: samples/shields/b_m2mem_pack1
   :board: nucleo_u3c5zi_q
   :shield: b_m2mem_pack1_mb1928_33lb
   :goals: build flash
   :compact:

The other variants are ``b_m2mem_pack1_mb1927_18ba``,
``b_m2mem_pack1_mb1927_33ba`` and ``b_m2mem_pack1_mb1928_33la``.

Sample Output
*************

The board information read from the EEPROM is printed first, followed by one
section per test:

.. code-block:: console

   ==============================================
    B-M2MEM-PACK1 shield test
   ==============================================
   ID EEPROM size: 16384 bytes
   Board: nucleo_u3c5zi_q

   EEPROM board information
   ----------------------------------------------
   Card part number:           MB1928-33LB-C01
   ...

   ----------------------------------------------
   TEST 1: Compare JEDEC ID's:
   ----------------------------------------------
   JEDEC ID - devicetree:       ef 40 15
   JEDEC ID - eeprom:           ef 70 15
   jedec id stored in eeprom doesn't match one in devicetree.
   TEST 1: FAIL
   ----------------------------------------------

   ----------------------------------------------
   TEST 2: Erase flash section and verify erased:
   ----------------------------------------------
   flash_erase success.
   TEST 2: PASS
   ----------------------------------------------

   ----------------------------------------------
   TEST 3: Write flash section and verify written:
   ----------------------------------------------
   Data written matches data read.
   TEST 3: PASS
   ----------------------------------------------

   ==============================================
    All tests completed: 2/3 passed
   ==============================================

Known result on MB1928-33LB
***************************

The run shown above is the expected one for this module. Its EEPROM records the
JEDEC ID ``ef 70 15``, but the fitted W25Q16JV answers ``ef 40 15``. The second
byte is the memory type and separates two ordering options of the same part:
``40`` is the plain SPI variant that is actually fitted, ``70`` is the DTR one.

The devicetree value is the correct one, and it is verified on every boot: the
flash driver reads the JEDEC ID and compares it against the ``jedec-id``
property before switching to quad mode, so the device would not become ready if
they disagreed.

The error is in the reference EEPROM record published by ST, not in a single
mis-programmed module, so TEST 1 is left strict. It reports a real
inconsistency, and the two flash tests still run and pass.
