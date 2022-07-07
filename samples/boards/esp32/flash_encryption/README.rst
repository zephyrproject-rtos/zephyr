.. _flash_encryption_test:

Espressif ESP32 Flash Encryption Test
#####################################

Overview
********

Flash encryption is intended for encrypting the contents of the ESP32's off-chip flash memory.
Once this feature is enabled, firmware is flashed as plaintext and then the data is encrypted
in place on the first boot. As a result, physical readout of flash will not be sufficient to
recover most flash contents. This is a hardware feature that can be enabled in MCUboot build process
and is an additional security measure beyond MCUboot existent features.

The following flash encryption modes are available:

* Development Mode

   Recommended for use ONLY DURING DEVELOPMENT, as it does not prevent modification and
   readout of encrypted flash contents.

* Release Mode

   Recommended for manufacturing and production to prevent physical readout of encrypted flash
   contents. When release mode is selected..

With flash encryption enabled, the following types of data are encrypted by default:

* Bootloader area
* Application area
* Storage area

For more details, check `ESP32 Flash Encryption`_ and `MCUBoot Readme`_.

Software Requirements
*********************

The following Python modules are required when building flash encryption sample:

* cryptography
* imgtool>=1.9.0

Setup
*****

This sample code isn't enough to enable flash encryption as it only consists on displaying
encrypted/decrypted data. It requires MCUBoot bootloader previously configured to enable the
feature. Follow the instructions provided at `MCUBoot Readme`_ prior to running this sample.

.. warning::
  When enabling the Flash Encryption, user can encrypt the content either using a device
  generated key (remains unknown and unreadable) or a host generated key (owner is responsible
  for keeping the key private and safe as .bin file). After the flash encryption gets enabled
  through eFuse burning on the device, all read and write operations are decrypted/encrypted
  in runtime.


Supported SoCs
**************

The following SoCs are supported by this sample code so far:

* ESP32

Building and Running
********************

Make sure you have your board connected over USB port.

.. code-block:: console

   west build -b esp32 samples/boards/esp32/flash_encryption
   west flash

Sample Output
=============

To check the output of this sample, run ``west espressif monitor`` or any other serial
console program (e.g., minicom, putty, screen, etc).
This example uses ``west espressif monitor``, which automatically detects the serial
port at ``/dev/ttyUSB0``:

.. code-block:: console

   $ west espressif monitor

The sample code writes a known buffer into the storage area defined in DTS file.
Then, it dumps 32-bytes of the same memory area in plaintext mode. The content is encrypted, meaning
that a reading attack to the off-chip memory is safe. Last step is to perform the
memory reading using proper spi_flash call, which decrypts the content as expected.

.. code-block:: console

   *** Booting Zephyr OS build v2.7.99-2729-geb08584393d6  ***
   [00:00:00.453,000] <inf> flash_encryption: Found flash controller FLASH_CTRL.

   [00:00:00.453,000] <inf> flash_encryption: BUFFER CONTENT
                                           00 01 02 03 04 05 06 07  08 09 0a 0b 0c 0d 0e 0f |........ ........
                                           10 11 12 13 14 15 16 17  18 19 1a 1b 1c 1d 1e 1f |........ ........
   [00:00:00.482,000] <inf> flash_encryption: FLASH RAW DATA (Encrypted)
                                           9a 06 93 76 12 cb 0f 7e  ec c5 12 6f 64 db d1 ff |...v...~ ...od...
                                           08 e6 be 0c cd 06 6d ad  7d 55 3d 57 bf b7 be 0a |......m. }U=W....
   [00:00:00.482,000] <inf> flash_encryption: FLASH DECRYPTED DATA
                                           00 01 02 03 04 05 06 07  08 09 0a 0b 0c 0d 0e 0f |........ ........
                                           10 11 12 13 14 15 16 17  18 19 1a 1b 1c 1d 1e 1f |........ ........


.. _ESP32 Flash Encryption:
   https://docs.espressif.com/projects/esp-idf/en/latest/esp32/security/flash-encryption.html

.. _MCUBoot Readme:
   https://www.mcuboot.com/documentation/readme-espressif/
