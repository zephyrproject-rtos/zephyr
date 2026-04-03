.. zephyr:code-sample:: flash-ipm
   :name: Flash IPM on ESP32
   :relevant-api: ipm_interface

   Sample of flash API sharing between CPU cores using inter-processor mailbox (IPM).

Overview
********

This sample demonstrates how to perform Flash API operations on
CPU0 (procpu, hpcore) from a program running on CPU1 (appcpu, lpcore),
using the Inter-Processor Messaging (IPM) for communication.
It showcases how flash operations can be delegated to a supervising core
when the remote core (CPU1) lacks direct access to flash hardware.

Execution Flow:
CPU0 initializes the IPM service and flash subsystem.
CPU1 starts after CPU0 is ready and connects to it using the IPM channel.

Test Procedure (on CPU1)
* Erases the first page (4096 bytes) of the storage_partition.
* Verifies that the page is blank.
* Writes a simple incremental pattern (0x00–0x7F).
* Verifies the written pattern by reading back the same data.

CPU1 prints a success message and terminates.
CPU0 continues running as the flash supervisor, ready for further requests.

Requirements
************

The ``esp32s3_devkitc`` board with flash chip is required to run this sample.

Building
********

Build the ESP32 Flash IPM sample code as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/espressif/flash_ipm/
   :board: esp32s3_devkitc/esp32s3/procpu
   :west-args: --sysbuild
   :goals: build
   :compact:

Sample Output
*************

To check the output of this sample, run ``west espressif monitor`` or any other serial
console program (e.g., minicom, putty, screen, etc).

.. code-block:: console

  *** Booting Zephyr OS build v4.3.0-rc1-14-gd840aed36685 ***
  uart:~$
  Flash IPM sample program (remote CPU)

  This program is running on a remote CPU.
  Partition storage_partition will be used to perform flash API.

  To inspect the test area manually, run on HOST CPU console:
   $ flash read 3b0000 1000

  Step 1: Erase
  * Erasing first 4096 bytes of partition.. OK
  * Verifing page erased.. OK
  Step 2: Write
  * Write ascii pattern..OK
  Step 3: Verify
  * Verifing pattern.. OK

  uart:~$
  uart:~$ flash read 3b0000 100
  003B0000: 00 01 02 03 04 05 06 07  08 09 0a 0b 0c 0d 0e 0f |........ ........|
  003B0010: 10 11 12 13 14 15 16 17  18 19 1a 1b 1c 1d 1e 1f |........ ........|
  003B0020: 20 21 22 23 24 25 26 27  28 29 2a 2b 2c 2d 2e 2f | !"#$%&' ()*+,-./|
  003B0030: 30 31 32 33 34 35 36 37  38 39 3a 3b 3c 3d 3e 3f |01234567 89:;<=>?|
  003B0040: 40 41 42 43 44 45 46 47  48 49 4a 4b 4c 4d 4e 4f |@ABCDEFG HIJKLMNO|
  003B0050: 50 51 52 53 54 55 56 57  58 59 5a 5b 5c 5d 5e 5f |PQRSTUVW XYZ[\]^_|
  003B0060: 60 61 62 63 64 65 66 67  68 69 6a 6b 6c 6d 6e 6f |`abcdefg hijklmno|
  003B0070: 70 71 72 73 74 75 76 77  78 79 7a 7b 7c 7d 7e 7f |pqrstuvw xyz{|}~.|
  003B0080: 00 01 02 03 04 05 06 07  08 09 0a 0b 0c 0d 0e 0f |........ ........|
  003B0090: 10 11 12 13 14 15 16 17  18 19 1a 1b 1c 1d 1e 1f |........ ........|
  003B00A0: 20 21 22 23 24 25 26 27  28 29 2a 2b 2c 2d 2e 2f | !"#$%&' ()*+,-./|
  003B00B0: 30 31 32 33 34 35 36 37  38 39 3a 3b 3c 3d 3e 3f |01234567 89:;<=>?|
  003B00C0: 40 41 42 43 44 45 46 47  48 49 4a 4b 4c 4d 4e 4f |@ABCDEFG HIJKLMNO|
  003B00D0: 50 51 52 53 54 55 56 57  58 59 5a 5b 5c 5d 5e 5f |PQRSTUVW XYZ[\]^_|
  003B00E0: 60 61 62 63 64 65 66 67  68 69 6a 6b 6c 6d 6e 6f |`abcdefg hijklmno|
  003B00F0: 70 71 72 73 74 75 76 77  78 79 7a 7b 7c 7d 7e 7f |pqrstuvw xyz{|}~.|
