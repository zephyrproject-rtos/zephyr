.. zephyr:code-sample:: mec172xevb_assy6906-rom_api
   :name: MEC172x ROM API
   :relevant-api: crypto

   Perform hash computations using the MEC172x ROM API through the Zephyr crypto API.

Overview
********

This sample demonstrates the use of the MEC172x ROM API for
hardware-accelerated cryptographic hash computations via the Zephyr crypto API.

It verifies SHA-224, SHA-256, SHA-384, and SHA-512 hash operations using
known test vectors with multiple message sizes.

The sample tests:

* Multi-block hash computation with block-aligned updates plus a remainder
* Arbitrary chunk-size hash computation
* Digest comparison against pre-computed expected values

Requirements
************

This sample runs on Microchip MEC172x EVB (:zephyr:board:`mec172xevb_assy6906`)

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/microchip/mec172xevb_assy6906/rom_api
   :board: mec172xevb_assy6906
   :goals: build flash
   :compact:

Sample Output
*************

.. code-block:: console

   It lives! mec172xevb_assy6906
   ROM API say GIVE MEMORY, MORE MEMORY!
   Test Zephyr crypto hash API for multiblock plus remainder
   Test Zephyr crypto hash API for multiblock plus remainder returned 0
   Test Zephyr crypto arbitrary chunk size = 0
   Test Zephyr crypto arbitrary chunk size returned 0
   Test Zephyr crypto arbitrary chunk size = 8
   Test Zephyr crypto arbitrary chunk size returned 0
   Test Zephyr crypto arbitrary chunk size = 23
   Test Zephyr crypto arbitrary chunk size returned 0
   Application done

