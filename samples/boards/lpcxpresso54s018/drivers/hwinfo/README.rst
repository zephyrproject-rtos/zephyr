.. _hwinfo_sample:

LPC54S018 Hardware Info Sample
###############################

Overview
********

This sample demonstrates how to read hardware information from the LPC54S018,
including the unique device ID.

The LPC54S018 provides a 128-bit unique identification number that is 
programmed during manufacturing and cannot be changed. This ID can be used for:

- Device authentication
- License management
- Inventory tracking
- Secure key derivation

Building and Running
********************

.. code-block:: console

   west build -b lpcxpresso54s018 samples/drivers/hwinfo
   west flash

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build v3.5.0 ***
   [00:00:00.000,000] <inf> hwinfo_lpc: LPC54S018 Unique ID: 0123456789abcdef0123456789abcdef
   [00:00:00.000,000] <inf> hwinfo_sample: LPC54S018 Hardware Info Sample
   [00:00:00.000,000] <inf> hwinfo_sample: Device ID (16 bytes):
   [00:00:00.000,000] <inf> hwinfo_sample:   01 23 45 67 89 ab cd ef
   [00:00:00.000,000] <inf> hwinfo_sample:   01 23 45 67 89 ab cd ef
   [00:00:00.000,000] <inf> hwinfo_sample: EUI64: 03:23:45:67:89:ab:cd:ef
   [00:00:00.000,000] <inf> hwinfo_sample: Serial Number: 0123456789abcdef0123456789abcdef
   [00:00:00.000,000] <inf> hwinfo_sample: Unique ID (as words):
   [00:00:00.000,000] <inf> hwinfo_sample:   Word 0: 0x67452301
   [00:00:00.000,000] <inf> hwinfo_sample:   Word 1: 0xefcdab89
   [00:00:00.000,000] <inf> hwinfo_sample:   Word 2: 0x67452301
   [00:00:00.000,000] <inf> hwinfo_sample:   Word 3: 0xefcdab89

API Usage
*********

Standard Zephyr hwinfo API:

- ``hwinfo_get_device_id()``: Get the unique device ID
- ``hwinfo_get_device_eui64()``: Get EUI64 derived from unique ID

LPC-specific extensions:

- ``lpc_get_unique_id()``: Get the raw 128-bit unique ID
- ``lpc_get_serial_number()``: Get unique ID as hex string

Implementation Details
**********************

The unique ID is read using the LPC54S018 ROM IAP (In-Application Programming)
interface. The IAP command 58 (Read UID) returns the 128-bit unique ID as
four 32-bit words.